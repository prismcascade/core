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


// 動作確認してません。とりあえず書いた。

// 型一覧。有理数は保留した。
typedef enum {	
	Int,
	Bool, 
	Float, 
	Text,
	Vector,	// これ関数との受け渡し大変だと思う。固定長配列にしない？
	Video,	// VideoFrameとVideoClipは上位互換側に寄せたい。RGBやRGBAも同様。
	Audio,	// 多分ここにバッファの概念は入ってきます。データ長め
}VariableType;

// 色空間とかはソフトウェア全体で統一してた方が良さそうなので一旦書いてない。
struct Video{
	int width;
	int height;
	int fps;
	int frame_index;		// バッファのうち現在のフレームの番号
	int*** frame_buffer;	// フレームが沢山入ったバッファ。最初はバッファサイズ１でやってみよう。
};

struct Audio{
	//TODO : 考えて書く
};

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