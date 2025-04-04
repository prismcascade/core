#pragma once

namespace PrismCascade {

/*
class TimelineManager {
public:
	TimelineManager(Project& projectRef, PluginManager& pm);
	void buildEffectChain();
	void addEffectToClip(const std::string& clipId, const Effect& eff);
private:
	Project& project;
	PluginManager& pluginMgr;
};
*/

}
using namespace PrismCascade;

#include <vector>
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <core/project_data.hpp>
#include <plugin/dynamic_library.hpp>
#include <plugin/plugin_manager.hpp>
#include <memory/memory_manager.hpp>
#include <render/rendering_scheduler.hpp>
#include <util/dump.hpp>


#if defined(_WIN32) || defined(_WIN64)
#define _TIMELINE_IS_WINDOWS_BUILD
#endif

#define TYPE_NAME_SIZE 256

/*
typedef struct{
	unsigned int height;
	unsigned int width;
	unsigned int buffer_size;
	unsigned char* frame_buf;
}Video_t;

typedef struct{
	unsigned int sampling_rate;
	unsigned int buffer_size;
	unsigned int* frame_buf;
}Audio_t;

typedef struct{
	unsigned int length;
	char* str;
}Text_t;

typedef union{
	Video_t video;
	Audio_t audio;
	Text_t text;
	int integer_num;
	float float_num;
}VarUnion_t;

typedef struct{
	VarUnion_t var_union;
	char var_type[TYPE_NAME_SIZE];
	char var_name[TYPE_NAME_SIZE];
}VarData_t;

typedef struct{
	unsigned int length;
	VarData_t* vars;
}VarVector_t;

typedef struct{
	char plugin_name[TYPE_NAME_SIZE];
	VarVector_t param_vars;
	VarVector_t input_vars;
	VarVector_t output_vars;
}Effect_t;
*/

typedef struct Clip Clip_t;
struct Clip{
	unsigned int start_frame;
	unsigned int end_frame;
	unsigned int layer;
	char clip_type[TYPE_NAME_SIZE];
	Effect_t effect;
	Clip_t* next_clip = NULL;
};

typedef struct GlobalData GlobalData_t;
struct GlobalData{
	VarData_t var;
	GlobalData_t* next_var = NULL;
};

// -- global data -----------------------------------------
unsigned int g_frames = 1;
unsigned int g_layers = 1;
unsigned int g_cursol = 0;
Clip_t* g_first_clip = NULL;
GlobalData_t* g_first_global_data = NULL;

// -- function ---------------------------------------------------
std::vector<Effect_t*> get_frame_effect(){
	std::vector<Effect_t*> frame_effect(g_layers, NULL);
	Clip_t* process_clip = g_first_clip;
	while (true){
		if (process_clip == NULL){
			break;
		}else{
			if (process_clip->start_frame <= g_cursol && g_cursol <= process_clip->end_frame){
				frame_effect[process_clip->layer] = &process_clip->effect;
			}
			process_clip = process_clip->next_clip;
		}
	}
	return frame_effect;
}

void output_data(){
	if (g_first_global_data != NULL){
		GlobalData_t* process_data = g_first_global_data;
		while(process_data != NULL){
			std::cout << "var_name:" << process_data->var.var_name << " | var_type: " << process_data->var.var_type << std::endl;;
			process_data = process_data->next_var;
		}
	}
	
}

void output_clips(){
	std::vector<std::string> output_matrix = {};
	for (int i=0; i<g_layers; i++){
		output_matrix.push_back("*");
		for (int j=0; j<g_frames; j++){
			output_matrix[i] += "*";
		}
	}

	Clip_t* process_clip = g_first_clip;
	int clip_num = 0;
	while (true){
		if (process_clip == NULL){
			break;
		}else{
			for (int j=0; j<g_frames; j++){
				if (process_clip->start_frame <= j && j <= process_clip->end_frame){
					output_matrix[process_clip->layer][j] = 'E';//char(clip_num+48);
				}
			}
			process_clip = process_clip->next_clip;
		}
		clip_num++;
	}
	std::vector<Effect_t*> frame_effect = get_frame_effect();
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	for (int i=0; i<g_layers; i++){
		for (int j=0; j<g_frames; j++){
			if (j==g_cursol){
				std::cout << "|";
			}else{
				std::cout << " ";
			}
			std::cout << output_matrix[i][j];
		}
		std::cout << std::endl;
	}
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	for (int i=0; i<g_layers; i++){
		if(frame_effect[i] != NULL){
			std::cout << "< " << frame_effect[i]->plugin_name << " | " << frame_effect[i]->plugin_uuid << " >" << std::endl;
			std::cout << "params : " << std::endl;
			for (int j=0; j<frame_effect[i]->param_vars.length; j++){
				std::cout << "  " << frame_effect[i]->param_vars.vars[j].var_name << " (Type: " << frame_effect[i]->param_vars.vars[j].var_type << ")" << std::endl;
			}
			std::cout << "input_vars : " << std::endl;
			for (int j=0; j<frame_effect[i]->input_vars.length; j++){
				std::cout << "  " << frame_effect[i]->input_vars.vars[j].var_name << " (Type: " << frame_effect[i]->input_vars.vars[j].var_type << ")" << std::endl;
			}
			std::cout << "output_vars : " << std::endl;
			for (int j=0; j<frame_effect[i]->output_vars.length; j++){
				std::cout << "  " << frame_effect[i]->output_vars.vars[j].var_name << " (Type: " << frame_effect[i]->output_vars.vars[j].var_type << ")" << std::endl;
			}
		}
		std::cout << std::endl;
		//std::cout << "*******************************************************************************" << std::endl;
	}
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
}

void update_global_data(){
	std::vector<Effect_t*> frame_effect = get_frame_effect();
	for (int i=0; i<g_layers; i++){
		if(frame_effect[i] != NULL){
			PluginManager plugin_manager;
			std::shared_ptr<AstNode> effect = plugin_manager.make_node(frame_effect[i]->plugin_uuid);
			plugin_manager.assign_input(effect, 0, frame_effect[i]->input_vars.vars[0].var_union.int_param);
			std::cout << "frame_effect[i]->input_vars.vars[0].var_union.int_param : " << frame_effect[i]->input_vars.vars[0].var_union.int_param << std::endl;
			plugin_manager.invoke_start_rendering(effect);
			bool ok_effect = plugin_manager.invoke_render_frame(effect, g_cursol);
			dump_parameters(plugin_manager.dll_memory_manager);
		}
	}
}

void add_global_data(VarData_t data){
	GlobalData_t* new_data = new GlobalData_t;
	new_data->var = data;
	if (g_first_global_data == NULL){
		g_first_global_data = new_data;
	}else{
		GlobalData_t* process_data = g_first_global_data;
		while(true){
			if (strcmp(process_data->var.var_name, new_data->var.var_name) == 0){
				return;
			}
			if (process_data->next_var == NULL){
				break;
			}else{
				process_data = process_data->next_var;
			}
		}
		process_data->next_var = new_data;
	}
}

void add_clip(unsigned int start_frame, unsigned int end_frame, unsigned int layer, const char* clip_type, const char* plugin_name, const char* plugin_uuid, VarVector_t param_vars, VarVector_t input_vars, VarVector_t output_vars){
	// 新しいクリップオブジェクトを作成
	Clip_t* new_clip = new Clip_t;
	new_clip->start_frame = start_frame;
	new_clip->end_frame = end_frame;
	new_clip->layer = layer;
	strcpy(new_clip->clip_type, clip_type);
	strcpy(new_clip->effect.plugin_name, plugin_name);
	strcpy(new_clip->effect.plugin_uuid, plugin_uuid);
	new_clip->effect.param_vars = param_vars;
	new_clip->effect.input_vars = input_vars;
	new_clip->effect.output_vars = output_vars;
	new_clip->next_clip = NULL;
	if (g_first_clip == NULL){
		g_first_clip = new_clip;
	}else{
		Clip_t* process_clip = g_first_clip;
		while(true){
			if (process_clip->next_clip == NULL){
				break;
			}else{
				process_clip = process_clip->next_clip;
			}
		}
		process_clip->next_clip = new_clip;
	}
	std::cout << new_clip << std::endl;
}

// cuiな気がする。
void start_update(){
	std::cout << "Start Timeline Update!!" << std::endl;
	PluginManager plugin_manager;
	
	int plugin_size = plugin_manager.dll_memory_manager.plugin_uuid_to_handler.size();
	std::vector<std::string> uuid_vector = {};
	auto plug_nanikore = plugin_manager.dll_memory_manager.plugin_uuid_to_handler.begin();
	for (int i=0; i<plugin_size; i++){
		uuid_vector.push_back(plug_nanikore->first);
		plug_nanikore = ++plug_nanikore;
	}
	while (true){
		dump_plugins(plugin_manager.dll_memory_manager);
		std::cout << "-------------------------------------------------------------------------------" << std::endl;
		std::cout << "Command : " << std::endl << "0:end  1:add  2:delete  3:move  4:flex  5:cursol_right  6:cursol_left  7:add_global_data" << std::endl;
		int command_input;
		std::cin >> command_input;
		if (command_input == 0){
			break;
		}else if (command_input == 1){
			std::cout << "StartFrame : ";
			int start_frame_input;
			std::cin >> start_frame_input;
			std::cout << "EndFrame : ";
			int end_frame_input;
			std::cin >> end_frame_input;
			std::cout << "Layer : ";
			int layer_input;
			std::cin >> layer_input;
			std::cout << "EffectName : ";
			std::string effect_name;
			std::cin >> effect_name;

			for (int i=0; i<plugin_size; i++){
				std::cout << i << " : " << uuid_vector[i] << std::endl;
			}
			std::cout << "EffectNumber : ";
			int effect_num;
			std::cin >> effect_num;
			std::string effect_uuid = uuid_vector[effect_num];
			
			// パラメータ値(いらんかもしれん)
			VarVector_t param_vars;
			param_vars.length = 1;
			param_vars.vars = (VarData_t*)calloc(param_vars.length, sizeof(VarData_t));
			strcpy(param_vars.vars[0].var_name, "param_01");
			strcpy(param_vars.vars[0].var_type,"int_param");
			param_vars.vars[0].var_union.int_param = 3;
			
			// 入力値
			VarVector_t input_vars;
			input_vars.length = 1;
			input_vars.vars = (VarData_t*)calloc(input_vars.length, sizeof(VarData_t));
			strcpy(input_vars.vars[0].var_name, "input_01");
			strcpy(input_vars.vars[0].var_type,"int_param");
			input_vars.vars[0].var_union.int_param = 3;
			VarData_t data_01;
			strcpy(data_01.var_name, "input_01");
			strcpy(data_01.var_type, "int_param");
			add_global_data(data_01);
			
			// 出力値
			VarVector_t output_vars;
			output_vars.length = 2;
			output_vars.vars = (VarData_t*)calloc(output_vars.length, sizeof(VarData_t));
			strcpy(output_vars.vars[0].var_name, "output_01");
			strcpy(output_vars.vars[0].var_type,"int_param");
			strcpy(output_vars.vars[1].var_name, "output_02");
			strcpy(output_vars.vars[1].var_type,"vector_param");
			VarData_t data_03;
			strcpy(data_03.var_name, "output_01");
			strcpy(data_03.var_type, "int_param");
			add_global_data(data_03);
			VarData_t data_04;
			strcpy(data_04.var_name, "output_02");
			strcpy(data_04.var_type, "vector_param");
			add_global_data(data_04);
			
			// 最大フレーム及び最大レイヤの更新
			if (g_frames <= end_frame_input){
				g_frames = end_frame_input + 1;
			}
			if (g_layers <= layer_input){
				g_layers = layer_input + 1;
			}

			// クリップの追加
			add_clip(start_frame_input, end_frame_input, layer_input, "Effect", effect_name.c_str(), effect_uuid.c_str(), param_vars, input_vars, output_vars);
		}else if(command_input == 5){
			if (g_cursol < g_frames-1){
				g_cursol++;
			}
		}else if(command_input == 6){
			if (g_cursol > 0){
				g_cursol--;
			}
		}else if(command_input == 7){
			VarData_t data;
			strcpy(data.var_name, "VAR");
			strcpy(data.var_type, "int_param");
			add_global_data(data);
		}
#ifdef _TIMELINE_IS_WINDOWS_BUILD
		std::system("cls");
#else
		std::system("clear");
#endif
		output_data();
		output_clips();
		update_global_data();
	}
}