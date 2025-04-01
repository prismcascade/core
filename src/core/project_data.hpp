#pragma once

#define PLUGIN_NAME_BYTES 256
#define PLUGIN_UUID_SIZE 128
#define PLUGIN_PARAM_BYTES 2048
#define PLUGIN_PARAM_QUANTITY 256
#define PLUGIN_VAR_NAME_BYTES 256
#define PLUGIN_VAR_QUANTITY 256
#define GLOBAL_DATA_NAME_BYTES 256
#define GLOBAL_DATA_BYTES 2048
#define GLOBAL_DATA_QUANTITY 2048

#include <cstdint>
#include <tuple>

extern "C" {

// 型一覧
typedef enum{
    Int,
    Bool,
    Float,
    Text,
    Vector,
    Video,
    Audio,
} VariableType_t;

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
};

struct TextParam {
    int size = 0;
    const char* buffer = nullptr;
};
struct VectorParam {
    VariableType_t type;
    int size = 0;
    void* value = nullptr;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

// 受け渡し時の一時的な入れ物
struct Parameter {
    VariableType_t type;
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

struct PluginMetaDataInternal {
    int protocol_version = 1;
    PluginType type;
    std::string uuid;
    std::string name;
};


// --この下一旦残しといて----------------------------------------
extern "C" {
	typedef struct ProjectMeta{
		int image_frame_rate;
		int audio_frame_rate;
		int screen_horizontal;
		int screen_vertical;
	}ProjectMeta_t;

	typedef struct GlobalData{
		char global_data_names[GLOBAL_DATA_QUANTITY][GLOBAL_DATA_NAME_BYTES];
		VariableType_t global_data_types[GLOBAL_DATA_QUANTITY];
		char global_data[GLOBAL_DATA_QUANTITY][GLOBAL_DATA_BYTES];
	}GlobalData_t;

	typedef struct PluginMeta{
		int protocol_version;
		char plugin_uuid[PLUGIN_UUID_SIZE];
		char plugin_name[PLUGIN_NAME_BYTES];
		char params[PLUGIN_PARAM_QUANTITY][PLUGIN_PARAM_BYTES];
	}PluginMeta_t;

	typedef struct EffectClip{
		PluginMeta_t plugin_meta;
		char input_variables[PLUGIN_VAR_QUANTITY][PLUGIN_VAR_NAME_BYTES];		// GlobalDataとの紐づけ (何番目の引数にどのグローバルデータを入れるか)
		char output_variables[PLUGIN_VAR_QUANTITY][PLUGIN_VAR_NAME_BYTES];		// GlobalDataとの紐づけ (何番目の返り値をどのグローバルデータに入れるか)
	}EffectClip_t;

	typedef struct MacroClip{
		PluginMeta_t plugin_meta;
		// 多分ここにマクロ特有の何かが入ってくる
	}MacroClip_t;

	typedef enum {
		Macro,
		Effect
	}ClipType_t;

	typedef struct Clip {
		ClipType_t clip_type;
		int layer;
		int frame_start;
		int frame_end;
		union ClipUnion{
			EffectClip_t macro_clip;
			MacroClip_t Effect_clip;
		};
		union ClipUnion clip_union;
	}Clip_t;

	typedef struct ProjectData{
		ProjectMeta_t project_meta;
		GlobalData_t global_data;
		Clip_t* clips[MAX_LAYER_SIZE];
	}ProjectData_t;

	bool AddClip(Clip_t clip);
	bool DeleteClip(Clip_t* clip);
	bool CutClip(Clip_t* clip, int cut_frame);
	bool MoveClip(Clip_t* clip, int frame_start, int layer);
}

