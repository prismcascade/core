#pragma once

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

typedef struct AudioParam {
}AudioParam_t;

typedef struct TextParam {
    int size = 0;
    const char* buffer = nullptr;
}TextParam_t;

typedef struct VectorParam {
    VariableType_t type;
    int size = 0;
    void* value = nullptr;
}VectorParam_t;

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


// --この下残しといて----------------------------------------

extern "C" {
	typedef struct ProjectMeta{
		int image_frame_rate;
		int audio_frame_rate;
		int screen_horizontal;
		int screen_vertical;
	}ProjectMeta_t;

	typedef struct VarData{
		VariableType_t var_type;
		char var_name;
		union VarUnion{
			int int_param;
			bool bool_param;
			float float_param;
			TextParam_t text_param;
			VectorParam_t vector_param;
			VectorParam_t video_param;
			AudioParam_t audio_param;
		};
		union VarUnion var_union;
	}VarData_t;

	typedef struct DataPack{
		int data_quantity;
		VarData_t* global_data;
	}DataPack_t;	// ParameterPackと同じだが、unionを使ったもの。どっちで作っていくかはまた話しましょう。

	typedef struct PluginMeta{
		int protocol_version;
		TextParam_t plugin_uuid;
		TextParam_t plugin_name;
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

	typedef struct Clip {
		ClipType_t clip_type;
		int layer;
		int frame_start;
		int frame_end;
		union ClipUnion{
			EffectClip_t macro_clip;
			MacroClip_t effect_clip;
		};
		union ClipUnion clip_union;
	}Clip_t;

	typedef struct TimelineData{
		int clip_quantity;
		Clip_t* clip;
	}TimelineData_t;

	typedef struct ProjectData{
		ProjectMeta_t project_meta;
		DataPack_t global_data;
		TimelineData_t timeline_data;
	}ProjectData_t;

	ProjectData_t project_data;

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
}

