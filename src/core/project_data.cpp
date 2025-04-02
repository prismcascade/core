#include <core/project_data.hpp>

namespace PrismCascade {

std::string to_string(VariableType variable_type){
	switch(variable_type){
		case VariableType::Int:
			return "Int";
		case VariableType::Bool:
			return "Bool";
		case VariableType::Float:
			return "Float";
		case VariableType::Text:
			return "Text";
		case VariableType::Vector:
			return "Vector";
		case VariableType::Video:
			return "Video";
		case VariableType::Audio:
			return "Audio";
		default:
			return "(unknown type)";
	}
}

std::string to_string(PluginType variable_type){
	switch(variable_type){
		case PluginType::Macro:
			return "Macro";
		case PluginType::Effect:
			return "Effect";
		default:
			return "(unknown type)";
	}
}

// ---------------- //

bool AddClip(Clip_t clip)
{
	return false;
}

bool DeleteClip(Clip_t* clip)
{
	return false;
}

bool CutClip(Clip_t *clip, int cut_frame)
{
	return false;
}

bool MoveClip(Clip_t *clip, int frame_start, int layer)
{
	return false;
}

bool FlexClip(Clip_t *clip, int frame_start, int frame_end)
{
	return false;
}

}
