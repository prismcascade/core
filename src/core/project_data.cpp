#include <core/project_data.hpp>
#include <stdexcept>

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

AstNode::input_t AstNode::make_empty_value(const std::vector<VariableType>& types){
	input_t empty_value{};
	switch(types.at(0)){
		case VariableType::Int:
			empty_value.emplace<std::int64_t>();
		break;
		case VariableType::Bool:
			empty_value.emplace<bool>();
		break;
		case VariableType::Float:
			empty_value.emplace<double>();
		break;
		case VariableType::Text:
			empty_value.emplace<std::string>();
		break;
		case VariableType::Vector:
			{
				VectorParam vector_param{};
				VariableType type_inner = types.at(1);
				vector_param.type = type_inner;
				empty_value.emplace<VectorParam>(vector_param);
			}
		break;
		case VariableType::Video:
		{
			VideoFrame video_frame{};
			empty_value.emplace<VideoFrame>(video_frame);
		}
		break;
		case VariableType::Audio:
		{
			AudioParam audio_param{};
			empty_value.emplace<AudioParam>(audio_param);
		}
		break;
		default:
			throw std::domain_error("[AstNode::make_empty_value] unknown type");
	}
	return empty_value;
}

// ---------------- //
/*
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
*/

}
