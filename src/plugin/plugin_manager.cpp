#include "plugin_manager.hpp"
#include <cassert>

namespace PrismCascade {

PluginManager::PluginManager() : PluginManager(std::nullopt){ }

PluginManager::PluginManager(const std::optional<std::string>& path){
    const std::vector<std::string>& plugin_file_list = DynamicLibrary::list_plugin(path);
    for(auto&& plugin_file : plugin_file_list){
        try {
            const std::int64_t plugin_handler = handle_manager.fresh_handler();
            DynamicLibrary lib(plugin_file);
            dynamic_libraries.emplace(plugin_handler, lib);

            // メタデータ取得
            {
                // メタデータを受け渡すための一時変数
                std::shared_ptr<PluginMetaData> plugin_metadata_dll(new PluginMetaData, [this](PluginMetaData* ptr){
                    if(ptr){
                        dll_memory_manager.free_text(&ptr->name);
                        dll_memory_manager.free_text(&ptr->uuid);
                        delete ptr;
                    }
                });

                // メタ情報を取得
                reinterpret_cast<bool(*)(void*, std::int64_t, PluginMetaData*, void*, void*, void*, void*)>(lib["getMetaInfo"])(
                    reinterpret_cast<void*>(&dll_memory_manager),
                    plugin_handler,
                    plugin_metadata_dll.get(),
                    reinterpret_cast<void*>(&DllMemoryManager::allocate_param_static),
                    reinterpret_cast<void*>(&DllMemoryManager::assign_text_static),
                    reinterpret_cast<void*>(&DllMemoryManager::add_required_handler_static),
                    reinterpret_cast<void*>(&DllMemoryManager::add_handleable_effect_static)
                );

                // 取得したメタ情報をホスト用メモリに転送
                PluginMetaDataInternal plugin_metadata;
                plugin_metadata.name = std::string(plugin_metadata_dll->name.buffer);
                plugin_metadata.protocol_version = plugin_metadata_dll->protocol_version;
                plugin_metadata.type = plugin_metadata_dll->type;
                plugin_metadata.uuid = std::string(plugin_metadata_dll->uuid.buffer);

                // UUID重複チェック
                if(dll_memory_manager.plugin_uuid_to_handler.count(plugin_metadata.uuid))
                    throw std::runtime_error("[PluginManager::PluginManager] duplicated plugin UUID");
                else
                    dll_memory_manager.plugin_uuid_to_handler[plugin_metadata.uuid] = plugin_handler;
                dll_memory_manager.plugin_metadata_instances[plugin_handler] = std::move(plugin_metadata);
            }

            // プラグインのロードイベント発火
            reinterpret_cast<bool(*)()>(lib["onLoadPlugin"])();
        } catch (...){
            std::cerr << "error: loading plugin: " << plugin_file << std::endl;
        }
    }
}

PluginManager::~PluginManager(){
    for(auto&& [plugin_handler, lib] : dynamic_libraries){
        try{
            // プラグインの破棄イベント発火
            reinterpret_cast<void(*)()>(lib["onDestroyPlugin"])();
        }catch(...){
            std::string plugin_name = "(unknown plugin)";
            if(dll_memory_manager.plugin_metadata_instances.count(plugin_handler))
                plugin_name = dll_memory_manager.plugin_metadata_instances.at(plugin_handler).name;
            std::cerr << "failed to invoke onDestroyPlugin() of " << plugin_name << std::endl;
        }
    }
}

void PluginManager::assign_input(std::shared_ptr<AstNode> node, int index, AstNode::input_t input_value){
    // uuid が変だったら例外が飛ぶが，そもそも make_node で弾かれるはずなのでアンロードしない限り安全
    const std::uint64_t plugin_handler = dll_memory_manager.plugin_uuid_to_handler.at(node->plugin_uuid);
    const std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2>& type_info = dll_memory_manager.parameter_type_informations.at(plugin_handler);
    // 値を運ぶ先が範囲外なら落とす
    if(index < 0 || type_info[0].size() <= index)
        throw std::domain_error("[PluginManager::assign_input] index out of bounds");

    // 型検査
    VariableType input_type{}, input_type_inner{};
    if(std::holds_alternative<int>(input_value))
        input_type = VariableType::Int;
    else if(std::holds_alternative<bool>(input_value))
        input_type = VariableType::Bool;
    else if(std::holds_alternative<double>(input_value))
        input_type = VariableType::Float;
    else if(std::holds_alternative<std::string>(input_value))
        input_type = VariableType::Text;
    else if(std::holds_alternative<std::shared_ptr<AstNode>>(input_value)) {
        std::shared_ptr<AstNode> sub_function = std::get<std::shared_ptr<AstNode>>(input_value);
        const std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2>& type_info_sub = dll_memory_manager.parameter_type_informations.at(sub_function->plugin_handler);
        // [1][0][0] は， 出力 の 0番目 の メイン型（VectorとかIntとか）を指している
        const auto& [sub_main_output_name, sub_main_output_type] = type_info_sub[1].at(0);
        input_type = sub_main_output_type.at(0);
        if(input_type == VariableType::Vector)
            input_type_inner = sub_main_output_type.at(1);
        // 子ノードの親を更新
        if(std::shared_ptr<AstNode> old_parent = sub_function->parent.lock()){
            // 古い親から外し，デフォルト値を入れる
            for(std::size_t i = 0; i < old_parent->children.size(); ++i){
                const auto& old_parent_child = old_parent->children[i];
                if(std::holds_alternative<std::shared_ptr<AstNode>>(old_parent_child)
                && std::get<std::shared_ptr<AstNode>>(old_parent_child) == sub_function){
                    // 元の型検査に通っていれば落ちないはず
                    assign_input(old_parent, i, AstNode::make_empty_value(sub_main_output_type));
                }
            }
        }
        sub_function->parent = node;  // 新しい親を設定する
    } else if(std::holds_alternative<AstNode::SubEdge>(input_value)) {
        const AstNode::SubEdge& sub_edge = std::get<AstNode::SubEdge>(input_value);
        if(std::shared_ptr<AstNode> source_function = sub_edge.from_node.lock()){
            // type が effectの場合のみ接続可能
            // TODO: macroが副出力をするのは，設計上で本当に悪？ (要検討)
            if(source_function->plugin_type != PluginType::Effect)
                throw std::domain_error("[PluginManager::assign_input] target of subedge cannot be macro (for now)");
            const std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2>& type_info_sub = dll_memory_manager.parameter_type_informations.at(source_function->plugin_handler);
            // 0番目は主出力なので， sub_edge の参照先としては許容しない
            if(sub_edge.index <= 0 || type_info_sub[1].size() <= sub_edge.index)
                throw std::domain_error("[PluginManager::assign_input] index of subedge out of bounds");
            // [1][n][0] は， 出力 の n番目 の メイン型（VectorとかIntとか）を指している
            const auto& [sub_output_name, sub_output_type] = type_info_sub[1].at(sub_edge.index);
            input_type = sub_output_type.at(0);
            if(input_type == VariableType::Vector)
                input_type_inner = sub_output_type.at(1);
        } else
            throw std::domain_error("[PluginManager::assign_input] input sub_edge has been expired");
        input_type = VariableType::Text;
    }

    // 型検査に通ったら接続する
    if([&](){
        auto&& [name, types] = type_info[0].at(index);
        if(types.at(0) != input_type)
            return false;
        if(types.at(0) == VariableType::Vector && types.at(1) != input_type_inner)
            return false;
        return true;
    }()){
        // make_node でアロケーション済みのはずなので，基本的には at でエラーは出ないはず
        node->children.at(index) = input_value;
    } else
        throw std::domain_error("[PluginManager::assign_input] input type does not match");
}

std::shared_ptr<AstNode> PluginManager::make_node(std::string uuid, std::weak_ptr<AstNode> parent){
    if(dll_memory_manager.plugin_uuid_to_handler.count(uuid)) {
        const std::uint64_t plugin_handler = dll_memory_manager.plugin_uuid_to_handler.at(uuid);
        const std::int64_t plugin_instance_handler = handle_manager.fresh_handler();
        std::shared_ptr<AstNode> node(new AstNode, [this, plugin_instance_handler](AstNode* ptr){
            if(ptr){
                dll_memory_manager.free_plugin_parameters(plugin_instance_handler);
                delete ptr;
            }
        });
        // プラグイン情報のセット
        node->plugin_handler = plugin_handler;
        node->plugin_instance_handler = plugin_instance_handler;
        node->plugin_uuid = uuid;

        PluginMetaDataInternal metadata = dll_memory_manager.plugin_metadata_instances.at(plugin_handler);
        node->plugin_type = metadata.type;
        node->protocol_version = metadata.protocol_version;
        node->plugin_name = metadata.name;

        node->parent = parent;

        // 入力パラメータ接続情報の領域確保
        const std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2>& type_info = dll_memory_manager.parameter_type_informations.at(plugin_handler);
        node->children.resize(type_info[0].size());  // メモリ確保は一括で行う（焼け石に水）
        for(int i=0; i < type_info[0].size(); ++i){
            auto&& [input_name, input_type] = type_info[0][i];
            node->children[i] = AstNode::make_empty_value(input_type);
        }

        // 入出力パラメータのDLL受け渡し用領域確保
        std::tie(node->input_params, node->output_params) = dll_memory_manager.allocate_plugin_parameters(plugin_handler, plugin_instance_handler);
    } else
        throw std::runtime_error("[PluginManager::make_node] required plugin is not loaded");
}

bool PluginManager::invoke_start_rendering(std::shared_ptr<AstNode> node){
    auto lib = dynamic_libraries.at(node->plugin_handler);
    reinterpret_cast<void(*)()>(lib["onStartRendering"])();
    // レンダリング開始イベント発火 & アロケーション
    VideoMetaData video_clip_meta_data;
    AudioMetaData audio_clip_meta_data;
    bool ok = reinterpret_cast<bool(*)(void*, VideoMetaData*, AudioMetaData*, ParameterPack*, ParameterPack*, void*, void*, void*, void*, void*)>(lib["onStartRendering"])(
        reinterpret_cast<void*>(&dll_memory_manager),
        &video_clip_meta_data,
        &audio_clip_meta_data,
        &node->input_params,
        &node->output_params,
        reinterpret_cast<void*>(&DllMemoryManager::load_video_buffer_static),
        reinterpret_cast<void*>(&DllMemoryManager::assign_text_static),
        reinterpret_cast<void*>(&DllMemoryManager::allocate_vector_static),
        reinterpret_cast<void*>(&DllMemoryManager::allocate_video_static),
        reinterpret_cast<void*>(&DllMemoryManager::allocate_audio_static)
    );
    return ok;
}

bool PluginManager::invoke_render_frame(std::shared_ptr<AstNode> node, int frame){
    auto lib = dynamic_libraries.at(node->plugin_handler);
    VideoMetaData video_clip_meta_data;
    AudioMetaData audio_clip_meta_data;
    if(node->video_clip_meta_data)
        video_clip_meta_data = *node->video_clip_meta_data;
    if(node->audio_clip_meta_data)
        audio_clip_meta_data = *node->audio_clip_meta_data;

    // 入力パラメータの用意

    auto copy_parameter = [&](Parameter& dst, Parameter& src){
        assert(src.type == dst.type);
        switch(src.type){
            case VariableType::Int:
                *reinterpret_cast<int*>(dst.value) = *reinterpret_cast<int*>(src.value);
            break;
            case VariableType::Bool:
                *reinterpret_cast<bool*>(dst.value) = *reinterpret_cast<bool*>(src.value);
            break;
            case VariableType::Float:
                *reinterpret_cast<double*>(dst.value) = *reinterpret_cast<double*>(src.value);
            break;
            case VariableType::Text:
                dll_memory_manager.copy_text(reinterpret_cast<TextParam*>(dst.value), reinterpret_cast<TextParam*>(src.value));
            break;
            case VariableType::Vector:
                dll_memory_manager.copy_vector(reinterpret_cast<VectorParam*>(dst.value), reinterpret_cast<VectorParam*>(src.value));
            break;
            case VariableType::Video:
                dll_memory_manager.copy_video(reinterpret_cast<VideoFrame*>(dst.value), reinterpret_cast<VideoFrame*>(src.value));
            break;
            case VariableType::Audio:
                dll_memory_manager.copy_audio(reinterpret_cast<AudioParam*>(dst.value), reinterpret_cast<AudioParam*>(src.value));
            break;
            default:
                throw std::domain_error("[PluginManager::invoke_render_frame] unknown type");
        }
    };
    // parameter_pack_instances[plugin_instance_handler] = (plugin_handler, 2[([parameter_adapter], [parameter_buffer])])
    std::shared_ptr<Parameter> self_input_ptr = dll_memory_manager.parameter_pack_instances.at(node->plugin_instance_handler).second[false].first;
    for(std::size_t input_index = 0; input_index < node->children.size(); ++input_index){
        const AstNode::input_t& child = node->children[input_index];
        Parameter& input_parameter = self_input_ptr.get()[input_index];
        if(std::holds_alternative<int>(child)) {
            const auto value = std::get<int>(child);
            assert(input_parameter.type == VariableType::Int);
            *reinterpret_cast<int*>(input_parameter.value) = value;
        } else if(std::holds_alternative<bool>(child)) {
            const auto value = std::get<bool>(child);
            assert(input_parameter.type == VariableType::Bool);
            *reinterpret_cast<bool*>(input_parameter.value) = value;
        } else if(std::holds_alternative<double>(child)) {
            const auto value = std::get<double>(child);
            assert(input_parameter.type == VariableType::Float);
            *reinterpret_cast<double*>(input_parameter.value) = value;
        } else if(std::holds_alternative<std::string>(child)) {
            const auto value = std::get<std::string>(child);
            assert(input_parameter.type == VariableType::Text);
            *reinterpret_cast<std::string*>(input_parameter.value) = value.c_str();
        } else if(std::holds_alternative<std::shared_ptr<AstNode>>(child)) {
            // 子が先に呼ばれている前提
            std::shared_ptr<AstNode> sub_function = std::get<std::shared_ptr<AstNode>>(child);
            std::shared_ptr<Parameter> child_output_ptr = dll_memory_manager.parameter_pack_instances.at(sub_function->plugin_instance_handler).second[true].first;
            copy_parameter(input_parameter, child_output_ptr.get()[0]);
        } else if(std::holds_alternative<AstNode::SubEdge>(child)) {
            // トポロジカルソートされている前提
            AstNode::SubEdge sub_edge = std::get<AstNode::SubEdge>(child);
            if(std::shared_ptr<AstNode> sub_function = sub_edge.from_node.lock()){
                std::shared_ptr<Parameter> child_output_ptr = dll_memory_manager.parameter_pack_instances.at(sub_function->plugin_instance_handler).second[true].first;
                copy_parameter(input_parameter, child_output_ptr.get()[sub_edge.index]);
            } else
                throw std::runtime_error("[PluginManager::invoke_render_frame] dungling parameter");
        }
    }

    // プラグイン呼び出し
    bool is_ok = reinterpret_cast<bool(*)(void*, ParameterPack*, ParameterPack*, const VideoMetaData, const AudioMetaData, int, void*, void*)>(lib["renderFrame"])(
        reinterpret_cast<void*>(&dll_memory_manager),
        &node->input_params,
        &node->output_params,
        video_clip_meta_data,
        audio_clip_meta_data,
        frame,
        reinterpret_cast<void*>(&DllMemoryManager::load_video_buffer_static),
        reinterpret_cast<void*>(&DllMemoryManager::assign_text_static)
    );
    return is_ok;
}
void PluginManager::invoke_finish_rendering(std::shared_ptr<AstNode> node){
    auto lib = dynamic_libraries.at(node->plugin_handler);
    reinterpret_cast<void(*)()>(lib["onFinishRendering"])();
}

}
