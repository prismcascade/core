#pragma once

#define PARAM_BYTES 2048
#define PLUGIN_NAME_BYTES 256
#define PLUGIN_QUANTITY 256
#define VAR_NAME_BYTES 256
#define VAR_QUANTITY 256
#define GLOBAL_DATA_NAME_BYTES 256
#define GLOBAL_DATA_BYTES 2048
#define GLOBAL_DATA_QUANTITY 2048
#define MAX_LAYER_SIZE 2048

#include <cstdint>
#include <tuple>

extern "C" {

// 型一覧
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
	std::uint32_t width;
	std::uint32_t height;
	double fps;
    std::uint64_t total_frames;
};

// 必ずホスト側がallocして渡し，プラグイン側では大きさの操作は不能
struct VideoFrame {
    VideoMetaData metadata;
	std::uint64_t current_frame;
	std::uint8_t* frame_buffer = nullptr;  // RGBA
};

struct Audio {
	//TODO : 考えて書く
};


struct TextParam {
    int size = 0;
    char* buf = nullptr;
};

struct VectorParam {
    VariableType type;
    int size = 0;
    void* value = nullptr;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

// 受け渡し時の一時的な入れ物（void*）
struct Parameter {
    VariableType type;
    void* value = nullptr;
};

struct ParameterPack {
    int size = 0;
    Parameter* parameter = nullptr;
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
// 書いてる途中
struct Project{
	struct ProjectMeta{
		int image_frame_rate;
		int audio_frame_rate;
		struct MovieSize{
			int horizontal;
			int vertical;
		};
		Project::ProjectMeta::MovieSize movie_size;
	};
	struct GlobalData{
		char global_data_names[GLOBAL_DATA_QUANTITY][GLOBAL_DATA_NAME_BYTES];
		VariableType global_data_types[GLOBAL_DATA_QUANTITY];
		char global_data[GLOBAL_DATA_QUANTITY][GLOBAL_DATA_BYTES];
	};
	Project::GlobalData global_data;
	struct Meta{
		int layer;
		struct Frame{
			int start;
			int end;
		};
		Project::Meta::Frame frame;
	};
	
	typedef enum {
		MacroType,
		FunctionType
	}LayerType;
	struct Layer {
		LayerType type;
		union {
			Macro* macroData;
			Function* functionData;
		};
	};

	struct Function;
	struct Macro;

	struct Function{
		Project::Meta meta_data;
		Project::GlobalData global_data;
		struct PluginData{
			char plugin_name[PLUGIN_NAME_BYTES];
			char params[PLUGIN_QUANTITY][PARAM_BYTES];
			char input_variables[VAR_QUANTITY][VAR_NAME_BYTES];
			char output_variables[VAR_QUANTITY][VAR_NAME_BYTES];
		};
		Project::Function::PluginData plugin_data;
	};

	struct Macro{
		Project::Meta meta_data;
		Project::GlobalData global_data;
		struct MacroData{
			char input_variables[VAR_QUANTITY][VAR_NAME_BYTES];
			char output_variables[VAR_QUANTITY][VAR_NAME_BYTES];
		};
		Project::Macro::MacroData macro_data;
		Project::Layer layers[MAX_LAYER_SIZE];
	};

	Layer layers[MAX_LAYER_SIZE];
};
Project project;