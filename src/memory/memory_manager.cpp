#include <memory/memory_manager.hpp>
#include <cassert>
#include <stdexcept>
#include <stdarg.h>
#include <map>
#include <unordered_map>

namespace PrismCascade {

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool DllMemoryManager::allocate_param(std::int64_t plugin_handler, bool is_output, VariableType type, const char* name, std::optional<VariableType> type_inner){
        std::lock_guard<std::mutex> lock{ mutex_ };
        if(type_inner)
            parameter_type_informations[plugin_handler][is_output].push_back({std::string(name), {type, *type_inner}});
        else
            parameter_type_informations[plugin_handler][is_output].push_back({std::string(name), {type}});
        return true;
    }

    bool DllMemoryManager::allocate_param_static(void* ptr, std::int64_t plugin_handler, bool is_output, VariableType type, const char* name, ...){
        if(type == VariableType::Vector){
            // Cスタイルの可変長引数として取得
            va_list ap;
            va_start(ap, name);
            VariableType type_inner = va_arg(ap, VariableType);
            return reinterpret_cast<DllMemoryManager*>(ptr)->allocate_param(plugin_handler, is_output, type, name, type_inner);
        } else
            return reinterpret_cast<DllMemoryManager*>(ptr)->allocate_param(plugin_handler, is_output, type, name, std::nullopt);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool DllMemoryManager::add_required_handler(std::int64_t plugin_handler, const char* effect_name){
        std::lock_guard<std::mutex> lock{ mutex_ };
        // TODO: implement
        return true;
    }

    bool DllMemoryManager::add_required_handler_static(void* ptr, std::int64_t plugin_handler, const char* effect_name){
        return reinterpret_cast<DllMemoryManager*>(ptr)->add_required_handler(plugin_handler, effect_name);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool DllMemoryManager::add_handleable_effect(std::int64_t plugin_handler, const char* effect_name){
        std::lock_guard<std::mutex> lock{ mutex_ };
        // TODO: implement
        return true;
    }

    bool DllMemoryManager::add_handleable_effect_static(void* ptr, std::int64_t plugin_handler, const char* effect_name){
        return reinterpret_cast<DllMemoryManager*>(ptr)->add_handleable_effect(plugin_handler, effect_name);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool DllMemoryManager::load_video_buffer(VideoFrame* target, std::uint64_t frame){
        std::lock_guard<std::mutex> lock{ mutex_ };
        // TODO: implement
        return true;
    }

    bool DllMemoryManager::load_video_buffer_static(void* ptr, VideoFrame* target, std::uint64_t frame){
        return reinterpret_cast<DllMemoryManager*>(ptr)->load_video_buffer(target, frame);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    std::pair<ParameterPack, ParameterPack> DllMemoryManager::allocate_plugin_parameters(std::int64_t plugin_handler, std::int64_t plugin_instance_handler){
        ParameterPack params[2];
        for(bool is_output : {false, true}){
            std::vector<std::tuple<std::string, std::vector<VariableType>>>& parameter_type_info = parameter_type_informations[plugin_handler][is_output];
            std::shared_ptr<Parameter> parameter_pack_buffer(new Parameter[parameter_type_info.size()], [](Parameter* ptr){ if(ptr) delete[] ptr; });
            std::vector<std::shared_ptr<void>> parameter_buffer_list;
            for(std::size_t i=0; i < parameter_type_info.size(); ++i) {
                const auto& [name, type] = parameter_type_info[i];
                std::shared_ptr<void> parameter_buffer;
                // NOTE: 現状では vector の入れ子がないので，typeの長さは最大 2
                switch(type[0]){
                    case VariableType::Int:
                        parameter_buffer = std::reinterpret_pointer_cast<void>(std::make_shared<int>());
                    break;
                    case VariableType::Bool:
                        parameter_buffer = std::reinterpret_pointer_cast<void>(std::make_shared<bool>());
                    break;
                    case VariableType::Float:
                        parameter_buffer = std::reinterpret_pointer_cast<void>(std::make_shared<double>());
                    break;
                    case VariableType::Text:
                        parameter_buffer = std::reinterpret_pointer_cast<void>(std::make_shared<TextParam>());
                    break;
                    case VariableType::Vector:
                        {
                            auto vector_container = std::make_shared<VectorParam>();
                            vector_container->type = type.at(1);  // これが落ちた場合， allocate_param からの型の受け渡しがおかしい
                            parameter_buffer = std::reinterpret_pointer_cast<void>(std::make_shared<VectorParam>());
                        }
                    break;
                    case VariableType::Video:
                        parameter_buffer = std::reinterpret_pointer_cast<void>(std::make_shared<VideoFrame>());
                    break;
                    case VariableType::Audio:
                        parameter_buffer = std::reinterpret_pointer_cast<void>(std::make_shared<AudioParam>());
                        break;
                    default:
                        throw std::runtime_error("[DllMemoryManager::allocate_plugin_parameters] unknown type");
                }
                parameter_pack_buffer.get()[i].type  = type[0];
                parameter_pack_buffer.get()[i].value = parameter_buffer.get();
                parameter_buffer_list.emplace_back(std::move(parameter_buffer));
            }
            assert(parameter_buffer_list.size() == parameter_type_info.size());

            parameter_pack_instances[plugin_instance_handler].first = plugin_handler;
            parameter_pack_instances[plugin_instance_handler].second[is_output] = { parameter_pack_buffer, parameter_buffer_list };
            params[is_output].size = static_cast<int>(parameter_type_info.size());
            params[is_output].parameters = parameter_pack_buffer.get();
        }
        return std::make_pair(params[0], params[1]);
    }

    bool DllMemoryManager::free_plugin_parameters(std::int64_t plugin_instance_handler){
        return parameter_pack_instances.erase(plugin_instance_handler);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool DllMemoryManager::assign_text(TextParam* buffer, const char* text){
        std::lock_guard<std::mutex> lock{ mutex_ };
        auto text_buffer = std::make_shared<std::string>(text);
        text_instances[buffer] = text_buffer;

        buffer->size = static_cast<int>(text_buffer->size());
        buffer->buffer = text_buffer->c_str();
        return true;
    }

    bool DllMemoryManager::assign_text_static(void* ptr, TextParam* buffer, const char* text){
        return reinterpret_cast<DllMemoryManager*>(ptr)->assign_text(buffer, text);
    }

    void DllMemoryManager::copy_text(TextParam* dst, TextParam* src){
        std::lock_guard<std::mutex> lock{ mutex_ };
        assert(text_instances.count(src));
        text_instances[dst] = text_instances[src];
        dst->buffer = text_instances[dst]->data();
    }

    void DllMemoryManager::free_text(TextParam* buffer){
        std::lock_guard<std::mutex> lock{ mutex_ };
        text_instances.erase(buffer);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    namespace {
        template<class T>
        std::shared_ptr<void> get_buffer_instance(int size){
            return std::shared_ptr<void>(reinterpret_cast<void*>(new T[size]), [](void* ptr){
                // C++20 からは shared_ptr が自動でしてくれた気がする
                // それ以前でビルドした時にポインタに自動変換されてメモリリークしそうな気がするので，念のため自分で書いた
                if(ptr) delete[] reinterpret_cast<T*>(ptr);
            });
        }
    }

    bool DllMemoryManager::allocate_vector(VectorParam* buffer, int size){
        std::lock_guard<std::mutex> lock{ mutex_ };
        std::shared_ptr<void> vector_buffer;
        // 型チェック
        if([](VariableType type) -> bool{  // returns is_error
            for(auto valid_type : {VariableType::Int, VariableType::Bool, VariableType::Float, VariableType::Text, VariableType::Video, VariableType::Audio})
                if(type == valid_type)
                    return false;
            return true;
        }(buffer->type))
            throw std::runtime_error("[DllMemoryManager::allocate_vector] unexpected type in vector");
        // 確保
        switch(buffer->type){
            case VariableType::Int:
                vector_buffer = get_buffer_instance<int>(size);
            break;
            case VariableType::Bool:
                vector_buffer = get_buffer_instance<bool>(size);  // NOTE: 今後バッファとして std::vector を使う場合には特殊化に注意
            break;
            case VariableType::Float:
                vector_buffer = get_buffer_instance<double>(size);
            break;
            case VariableType::Text:
                vector_buffer = std::shared_ptr<void>(reinterpret_cast<void*>(new TextParam[size]), [this, size](void* ptr){
                    if(ptr){
                        TextParam* text_param_ptr = reinterpret_cast<TextParam*>(ptr);
                        for(int i=0; i < size; ++i)
                            free_text(text_param_ptr + i);
                        delete[] text_param_ptr;
                    }
                });
            break;
            case VariableType::Vector:
                // 将来的には対応するかも
                throw std::runtime_error("[DllMemoryManager::allocate_vector] vectors of vectors cannot be created (for now)");
            break;
            case VariableType::Video:
                vector_buffer = std::shared_ptr<void>(reinterpret_cast<void*>(new VideoFrame[size]), [this, size](void* ptr){
                    if(ptr){
                        VideoFrame* video_param_ptr = reinterpret_cast<VideoFrame*>(ptr);
                        for(int i=0; i < size; ++i)
                            free_video(video_param_ptr + i);
                        delete[] video_param_ptr;
                    }
                });
            break;
            case VariableType::Audio:
                vector_buffer = std::shared_ptr<void>(reinterpret_cast<void*>(new AudioParam[size]), [this, size](void* ptr){
                    if(ptr){
                        AudioParam* audio_param_ptr = reinterpret_cast<AudioParam*>(ptr);
                        for(int i=0; i < size; ++i)
                            free_audio(audio_param_ptr + i);
                        delete[] audio_param_ptr;
                    }
                });
                break;
            default:
                throw std::runtime_error("[DllMemoryManager::allocate_vector] unknown type");
        }

        vector_instances[buffer] = vector_buffer;
        buffer->size = size;
        // buffer->type はメモリ確保時点でセット済み
        buffer->buffer = vector_buffer.get();
        return true;
    }

    bool DllMemoryManager::allocate_vector_static(void* ptr, VectorParam* buffer,int size){
        return reinterpret_cast<DllMemoryManager*>(ptr)->allocate_vector(buffer, size);
    }

    void DllMemoryManager::copy_vector(VectorParam* dst, VectorParam* src){
        std::lock_guard<std::mutex> lock{ mutex_ };
        assert(vector_instances.count(src));
        assert(src->type == dst->type);
        vector_instances[dst] = vector_instances.at(src);
        dst->size = src->size;
        dst->buffer = vector_instances[dst].get();
    }

    void DllMemoryManager::free_vector(VectorParam* buffer){
        std::lock_guard<std::mutex> lock{ mutex_ };
        vector_instances.erase(buffer);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool DllMemoryManager::allocate_video(VideoFrame* buffer,  VideoMetaData metadata){
        std::lock_guard<std::mutex> lock{ mutex_ };
        long pixel_count = metadata.height * metadata.width;
        auto video_buffer = std::make_shared<std::vector<std::uint8_t>>(pixel_count);

        video_instances[buffer] = video_buffer;
        buffer->current_frame = 0;
        buffer->metadata = metadata;
        buffer->frame_buffer = video_buffer->data();
        return true;
    }

    bool DllMemoryManager::allocate_video_static(void* ptr, VideoFrame* buffer,  VideoMetaData metadata){
        return reinterpret_cast<DllMemoryManager*>(ptr)->allocate_video(buffer, metadata);
    }

    void DllMemoryManager::copy_video(VideoFrame* dst, VideoFrame* src){
        std::lock_guard<std::mutex> lock{ mutex_ };
        assert(video_instances.count(src));
        video_instances[dst] = video_instances[src];
        dst->frame_buffer = reinterpret_cast<std::uint8_t*>(video_instances[dst].get());
    }

    void DllMemoryManager::free_video(VideoFrame* buffer){
        std::lock_guard<std::mutex> lock{ mutex_ };
        video_instances.erase(buffer);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool DllMemoryManager::allocate_audio(AudioParam* buffer){
        std::lock_guard<std::mutex> lock{ mutex_ };
        long sample_count = 44100;  // TODO: メタデータを適宜取得して計算
        auto audio_buffer = std::make_shared<std::vector<double>>(sample_count);

        audio_instances[buffer] = audio_buffer;
        buffer->buffer = audio_buffer->data();
        return true;
    }

    bool DllMemoryManager::allocate_audio_static(void* ptr, AudioParam* buffer){
        return reinterpret_cast<DllMemoryManager*>(ptr)->allocate_audio(buffer);
    }

    void DllMemoryManager::copy_audio(AudioParam* dst, AudioParam* src){
        std::lock_guard<std::mutex> lock{ mutex_ };
        assert(audio_instances.count(src));
        audio_instances[dst] = audio_instances[src];
        dst->buffer = reinterpret_cast<double*>(audio_instances[dst].get());
    }

    void DllMemoryManager::free_audio(AudioParam* buffer){
        std::lock_guard<std::mutex> lock{ mutex_ };
        audio_instances.erase(buffer);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    void DllMemoryManager::dumpMemoryUsage()
    {
        std::lock_guard<std::mutex> lock{ mutex_ };

        std::cerr << "========== [DllMemoryManager Memory Usage] ==========" << std::endl;

        // 1) Text
        //    text_instances: TextParam* -> shared_ptr<std::string>
        //    同一shared_ptrを複数キーが指している可能性があるので、setでunique化
        {
            std::unordered_map<std::shared_ptr<std::string>, int> textRefCount;
            for(auto&& kv : text_instances){
                auto sp = kv.second; 
                textRefCount[sp]++; // counting how many param keys reference the same sp
            }
            std::size_t distinctCount = textRefCount.size();
            std::size_t totalCharCount = 0;
            for(auto&& [spStr, refCount] : textRefCount){
                totalCharCount += spStr->size();
            }
            std::cerr << "[Text]  Distinct=" << distinctCount 
                      << "  totalCharCount=" << totalCharCount 
                      << "  (some might share the same std::string)\n";
        }

        // 2) VectorParam
        //    vector_instances: VectorParam* -> shared_ptr<void>
        //    どの型かによってメモリサイズが異なるが、ここでは "size * sizeof(type)" 程度を推定することも可能
        //    ただし internal type (Int/Bool/Float/...) は VectorParam.type で判定
        {
            // distinctResources: sp<void> -> (estimated_bytes, count)
            std::unordered_map<std::shared_ptr<void>, std::pair<std::size_t,int>> distinctVector;
            for(auto&& kv : vector_instances){
                VectorParam* vparam = kv.first;
                std::shared_ptr<void> sp = kv.second;
                auto it = distinctVector.find(sp);
                if(it == distinctVector.end()){
                    // estimate memory
                    std::size_t bytes = 0;
                    switch(vparam->type){
                        case VariableType::Int:   bytes = vparam->size * sizeof(int); break;
                        case VariableType::Bool:  bytes = vparam->size * sizeof(bool); break;
                        case VariableType::Float: bytes = vparam->size * sizeof(double); break;
                        case VariableType::Text:  bytes = vparam->size * sizeof(TextParam); break; 
                        case VariableType::Video: bytes = vparam->size * sizeof(VideoFrame); break;
                        case VariableType::Audio: bytes = vparam->size * sizeof(AudioParam); break;
                        default: break; 
                    }
                    distinctVector[sp] = { bytes, 1 };
                } else {
                    // increment refCount
                    it->second.second++;
                }
            }
            std::size_t totalDistinct = distinctVector.size();
            std::size_t sumBytes = 0;
            for(auto&& [sp, info] : distinctVector){
                sumBytes += info.first; 
            }
            std::cerr << "[VectorParam] Distinct=" << totalDistinct 
                      << ", totalEstimatedBytes=" << sumBytes 
                      << " (some might share the same buffer)\n";
        }

        // 3) VideoFrame
        //    video_instances: VideoFrame* -> shared_ptr<std::vector<uint8_t>>
        //    *We can sum up the .size() of each distinct std::vector
        {
            std::unordered_map<std::shared_ptr<std::vector<std::uint8_t>>, int> distinctVideo;
            std::size_t totalBytes = 0;
            for(auto&& kv : video_instances){
                auto sp = kv.second;
                distinctVideo[sp]++;
            }
            for(auto&& [spVec, refCount] : distinctVideo){
                totalBytes += spVec->size();
            }
            std::cerr << "[VideoFrame] Distinct=" << distinctVideo.size()
                      << ", totalByteSize=" << totalBytes
                      << " (some might share the same std::vector)\n";
        }

        // 4) AudioParam
        //    audio_instances: AudioParam* -> shared_ptr<std::vector<double>>
        //    sum up the .size() * sizeof(double) of each distinct
        {
            std::unordered_map<std::shared_ptr<std::vector<double>>, int> distinctAudio;
            std::size_t totalBytes = 0;
            for(auto&& kv : audio_instances){
                distinctAudio[kv.second]++;
            }
            for(auto&& [spVec, refCount] : distinctAudio){
                totalBytes += spVec->size() * sizeof(double);
            }
            std::cerr << "[AudioParam] Distinct=" << distinctAudio.size()
                      << ", totalByteSize=" << totalBytes
                      << " (some might share the same std::vector)\n";
        }
    
        std::cerr << "=====================================================" << std::endl;
    }

}
