#pragma once

#include <cstdint>
#include <tuple>
#include <string>
#include <variant>
#include <memory>
#include <optional>
#include <vector>

#define TYPE_NAME_SIZE 256

namespace PrismCascade {

extern "C" {

// 型一覧 (Float の中身は doubleなので注意)
enum class VariableType {
    Int,
    Bool,
    Float,
    Text,
    Vector,
    Video,
    Audio,
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

// パラメータとして受け渡される型 (int, bool, float はプリミティブ型を使う)
struct VideoMetaData {
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	double fps = 0;
    std::uint64_t total_frames = 0;
};

// 必ずホスト側がallocして渡し，プラグイン側では大きさの操作は不能
struct VideoFrame {
    VideoMetaData metadata;
	std::uint64_t current_frame = 0;
	std::uint8_t* frame_buffer = nullptr;  // RGBA
};

struct AudioMetaData {
	// TODO: 必要なメンバ変数を適宜考える
	std::uint64_t total_samples = 0;
};

struct AudioParam {
	// TODO: 必要なメンバ変数を適宜考える
	double* buffer = nullptr;
};

struct TextParam {
    int size = 0;
    const char* buffer = nullptr;
};

// TODO : 多分これをunionに入れるとエラー出る。なんとかする。
struct VectorParam {
    VariableType type{};
    int size = 0;
    void* buffer = nullptr;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

// 受け渡し時の一時的な入れ物
struct Parameter {
    VariableType type{};
    void* value = nullptr;
};

struct ParameterPack {
    int size = 0;
    Parameter* parameters = nullptr;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

// プラグインのメタデータ
enum class PluginType {
    Macro,
    Effect,
};

struct PluginMetaData {
    int protocol_version = 1;
    PluginType type;
    TextParam uuid;
    TextParam name;
};

}

std::string to_string(VariableType variable_type);
std::string to_string(PluginType variable_type);

struct PluginMetaDataInternal {
    int protocol_version = 1;
    PluginType type;
    std::string uuid;
    std::string name;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

// AST
struct AstNode {
	struct SubEdge {
		std::weak_ptr<AstNode> from_node;
		int index = 0;
	};
	using input_t = std::variant<std::shared_ptr<AstNode>, SubEdge, int, bool, double, VectorParam, VideoFrame, AudioParam, std::string>;

	// handler は実行ごとに変化する，勝手につけた通し番号
	std::uint64_t plugin_handler{};
	std::uint64_t plugin_instance_handler{};
	// uuidは，プラグイン側で決められる識別ID
	std::string plugin_uuid{};

	// メタデータからも辿れるが，処理の途中で途中で変化しない値なので，扱いやすさのためにもコピーをここに置く
	int protocol_version{};
	PluginType plugin_type{};
	std::string plugin_name{};

	// パラメータ入力一覧
	// TODO: 将来バージョンでは vector をプリミティブ入力として許す
	std::vector<input_t> children;

	// 主出力先
	std::weak_ptr<AstNode> parent;

	// 削除時にdead_input を検査するための，仮親ノード一覧
	std::vector<std::weak_ptr<AstNode>> sub_output_destinations;


	// これ自体が video clip や audio clip としてふるまう場合，そのメタデータが入る
	std::optional<VideoMetaData> video_clip_meta_data;
	std::optional<AudioMetaData> audio_clip_meta_data;

	// TODO: macro を実装したら使えるようになる予定のメンバ変数
	// [depth, ast_node]
	// 外側のコンテナが vector で良いのかは要検討
	std::vector<std::pair<int, std::shared_ptr<AstNode>>> generated_child_ast;

	// DLL側のメモリ
	ParameterPack input_params{};
	ParameterPack output_params{};

	bool type_check(){ return true; }  // TODO: 子についても再帰的にチェックする
	bool check_dead_input(){ return false; }  // TODO: 子についても再帰的にチェックする

	static input_t make_empty_value(const std::vector<VariableType>& types);
};


// --この下残しといて----------------------------------------

extern "C" {
	typedef struct ProjectMeta{
		int image_frame_rate;
		int audio_frame_rate;
		int screen_horizontal;
		int screen_vertical;
	}ProjectMeta_t;

	typedef union{
		int int_param;
		bool bool_param;
		float float_param;
		struct TextParam* text_param;
		struct VectorParam* vector_param;
		struct VectorParam* video_param;
		struct AudioParam* audio_param;
	}VarUnion_t;

	typedef struct VarData{
		char var_type[TYPE_NAME_SIZE];
		char var_name[TYPE_NAME_SIZE];
		VarUnion_t var_union;
	}VarData_t;

	typedef struct DataPack{
		int data_quantity;
		VarData_t* global_data;
	}DataPack_t;	// ParameterPackと同じだが、unionを使ったもの。どっちで作っていくかはまた話しましょう。

	typedef struct PluginMeta{
		int protocol_version;
		TextParam plugin_uuid;
		TextParam plugin_name;
	}PluginMeta_t;

	typedef struct EffectClip{
		PluginMeta_t plugin_meta;
		DataPack_t params;
		DataPack_t input_variables;		// GlobalDataとの紐づけ (何番目の引数にどのグローバルデータを入れるか)
		DataPack_t output_variables;	// GlobalDataとの紐づけ (何番目の返り値をどのグローバルデータに入れるか)
	}EffectClip_t;

	typedef struct MacroClip{
		PluginMeta_t plugin_meta;
		DataPack_t params;
		int layer_range;	// 影響するレイヤ範囲(基準レイヤから何レイヤか)
	}MacroClip_t;

	typedef enum {
		Macro,
		Effect
	}ClipType_t;
	
	typedef struct{
		unsigned int length;
		VarData_t* vars;
	}VarVector_t;
	
	typedef struct{
		char plugin_name[TYPE_NAME_SIZE];
		char plugin_uuid[TYPE_NAME_SIZE];
		VarVector_t param_vars;
		VarVector_t input_vars;
		VarVector_t output_vars;
	}Effect_t;
	
	/*
	typedef struct Clip Clip_t;
	typedef struct Clip {
		char clip_type[TYPE_NAME_SIZE];
		int layer;
		int start_frame;
		int end_frame;
		union ClipUnion{
			EffectClip_t macro_clip;
			MacroClip_t effect_clip;
		};
		union ClipUnion clip_union;
		Clip_t* next_clip = NULL;
	}Clip_t;
	*/
	/*
	typedef struct TimelineData{
		int clip_quantity;
		Clip_t* clip;
	}TimelineData_t;

	typedef struct ProjectData{
		ProjectMeta_t project_meta;
		DataPack_t global_data;
		TimelineData_t timeline_data;
	}ProjectData_t;
	*/

	// hpp内で変数定義するとエラー出るからこれだけコメントアウトした
	// ProjectData_t project_data;

	/*
	// project_dataにclipを追加する関数
	bool AddClip(Clip_t clip);

	// project_dataからclipを削除する関数
	bool DeleteClip(Clip_t* clip);

	// project_dataのclipを分割する関数
	bool CutClip(Clip_t* clip, int cut_frame);

	// project_dataのclipを移動する関数
	bool MoveClip(Clip_t* clip, int frame_start, int layer);

	//  project_dataのclipを伸縮する関数
	bool FlexClip(Clip_t* clip, int frame_start, int frame_end);
	*/
}

// yukatayu記述部分とchekegirl記述部分の間を取り持つ関数
static ParameterPack vecvart2parampack (std::vector<VarData_t> var_data){
	ParameterPack param_pack;
	int param_size = var_data.size();
	param_pack.size = param_size;
	param_pack.parameters = (Parameter*)calloc(param_size, sizeof(Parameter));
	for (int i=0; i<param_size; i++){
		if(std::string(var_data[i].var_type) == std::string("int_param")){
			param_pack.parameters[i].type = VariableType::Int;
			param_pack.parameters[i].value = &var_data[i].var_union.int_param;
		}
		if(std::string(var_data[i].var_type) == std::string("bool_param")){
			param_pack.parameters[i].type = VariableType::Bool;
			param_pack.parameters[i].value = &var_data[i].var_union.bool_param;
		}
		if(std::string(var_data[i].var_type) == std::string("float_param")){
			param_pack.parameters[i].type = VariableType::Float;
			param_pack.parameters[i].value = &var_data[i].var_union.float_param;
		}
		if(std::string(var_data[i].var_type) == std::string("text_param")){
			param_pack.parameters[i].type = VariableType::Text;
			param_pack.parameters[i].value = var_data[i].var_union.text_param;
		}
		if(std::string(var_data[i].var_type) == std::string("vector_param")){
			param_pack.parameters[i].type = VariableType::Vector;
			param_pack.parameters[i].value = var_data[i].var_union.vector_param;
		}
		if(std::string(var_data[i].var_type) == std::string("video_param")){
			param_pack.parameters[i].type = VariableType::Video;
			param_pack.parameters[i].value = var_data[i].var_union.video_param;
		}
		if(std::string(var_data[i].var_type) == std::string("audio_param")){
			param_pack.parameters[i].type = VariableType::Audio;
			param_pack.parameters[i].value = var_data[i].var_union.audio_param;
		}
	}
	return param_pack;
}

}

// 必要なのこっちだった
//static ParameterPack varvect2parampack (VarVector_t var_data);
