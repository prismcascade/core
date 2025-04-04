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
			std::cout << "var_name:" << process_data->var.var_name << " | var_type" << process_data->var.var_type << std::endl;;
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
			std::cout << "< " << frame_effect[i]->plugin_name << " >" << std::endl;
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

void add_clip(unsigned int start_frame, unsigned int end_frame, unsigned int layer, const char* clip_type, const char* plugin_name, VarVector_t param_vars, VarVector_t input_vars, VarVector_t output_vars){
	// 新しいクリップオブジェクトを作成
	Clip_t* new_clip = new Clip_t;
	new_clip->start_frame = start_frame;
	new_clip->end_frame = end_frame;
	new_clip->layer = layer;
	strcpy_s(new_clip->clip_type, clip_type);
	strcpy_s(new_clip->effect.plugin_name, plugin_name);
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

void start_update(){
	std::cout << "Start Timeline Update!!" << std::endl;
	while (true){
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
			
			VarVector_t param_vars;
			param_vars.length = 1;
			param_vars.vars = (VarData_t*)calloc(param_vars.length, sizeof(VarData_t));
			strcpy_s(param_vars.vars[0].var_name, "param_01");
			strcpy_s(param_vars.vars[0].var_type,"int_param");
			param_vars.vars[0].var_union.int_param = 3;
			VarVector_t input_vars;
			input_vars.length = 2;
			input_vars.vars = (VarData_t*)calloc(input_vars.length, sizeof(VarData_t));
			strcpy_s(input_vars.vars[0].var_name, "input_01");
			strcpy_s(input_vars.vars[0].var_type,"int_param");
			strcpy_s(input_vars.vars[1].var_name, "input_02");
			strcpy_s(input_vars.vars[1].var_type,"int_param");
			VarData_t data_01;
			strcpy_s(data_01.var_name, "input_01");
			strcpy_s(data_01.var_type, "int_param");
			add_global_data(data_01);
			VarData_t data_02;
			strcpy_s(data_02.var_name, "input_02");
			strcpy_s(data_02.var_type, "int_param");
			add_global_data(data_02);
			/*
			VarVector_t param_vars;
			param_vars.length = 0;
			param_vars.vars = NULL;
			VarVector_t input_vars;
			input_vars.length = 0;
			input_vars.vars = NULL;
			*/
			VarVector_t output_vars;
			output_vars.length = 0;
			output_vars.vars = NULL;
			if (g_frames <= end_frame_input){
				g_frames = end_frame_input + 1;
			}
			if (g_layers <= layer_input){
				g_layers = layer_input + 1;
			}
			add_clip(start_frame_input, end_frame_input, layer_input, "Effect", effect_name.c_str(), param_vars, input_vars, output_vars);
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
			strcpy_s(data.var_name, "VAR");
			strcpy_s(data.var_type, "int_param");
			add_global_data(data);
		}
		std::system("cls");
		output_data();
		output_clips();
	}
}