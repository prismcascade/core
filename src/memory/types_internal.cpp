// src/common/types_internal.cpp
#include <prismcascade/common/types.hpp>
#include <prismcascade/memory/types_internal.hpp>
#include <stdexcept>

namespace prismcascade {
namespace memory {

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//       ParameterMemory (base)      //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
ParameterMemory::ParameterMemory(VariableType type) : type_{type} {}

const Parameter& ParameterMemory::get_paramter_struct() {
    refresh_parameter_struct();
    return parameter_;
}

std::shared_ptr<ParameterMemory> make_empty_value(const std::vector<VariableType>& types) {
    switch (types.at(0)) {
        case VariableType::Int:
            return std::static_pointer_cast<ParameterMemory>(std::make_shared<IntParamMemory>());
        case VariableType::Bool:
            return std::static_pointer_cast<ParameterMemory>(std::make_shared<BoolParamMemory>());
        case VariableType::Float:
            return std::static_pointer_cast<ParameterMemory>(std::make_shared<FloatParamMemory>());
        case VariableType::Text:
            return std::static_pointer_cast<ParameterMemory>(std::make_shared<TextParamMemory>());
        case VariableType::Vector:
            return std::static_pointer_cast<ParameterMemory>(std::make_shared<VectorParamMemory>(types.at(1)));
        case VariableType::Video:
            return std::static_pointer_cast<ParameterMemory>(std::make_shared<VideoFrameMemory>());
        case VariableType::Audio:
            return std::static_pointer_cast<ParameterMemory>(std::make_shared<AudioParamMemory>());
        default:
            throw std::runtime_error("prismcascade: types_internal: make_empty_value: unknown type");
    }
}

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//        Int / Bool / Float         //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
IntParamMemory::IntParamMemory() : ParameterMemory(VariableType::Int) {}
std::int64_t& IntParamMemory::buffer() { return parameter_instance_; }
void          IntParamMemory::refresh_parameter_struct() {
    parameter_.type  = VariableType::Int;
    parameter_.value = reinterpret_cast<void*>(&parameter_instance_);
}

std::size_t IntParamMemory::get_memory_usage() { return sizeof(IntParamMemory); };

BoolParamMemory::BoolParamMemory() : ParameterMemory(VariableType::Bool) {}
bool& BoolParamMemory::buffer() { return parameter_instance_; }
void  BoolParamMemory::refresh_parameter_struct() {
    parameter_.type  = VariableType::Bool;
    parameter_.value = reinterpret_cast<void*>(&parameter_instance_);
}

std::size_t BoolParamMemory::get_memory_usage() { return sizeof(BoolParamMemory); };

FloatParamMemory::FloatParamMemory() : ParameterMemory(VariableType::Float) {}
double& FloatParamMemory::buffer() { return parameter_instance_; }
void    FloatParamMemory::refresh_parameter_struct() {
    parameter_.type  = VariableType::Float;
    parameter_.value = reinterpret_cast<void*>(&parameter_instance_);
}

std::size_t FloatParamMemory::get_memory_usage() { return sizeof(FloatParamMemory); };

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//         VideoFrameMemory          //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

VideoFrameMemory::VideoFrameMemory() : ParameterMemory(VariableType::Video) {}
void VideoFrameMemory::update_metadata(VideoMetaData metadata) {
    metadata_ = metadata;
    buffer_.resize(metadata.height * metadata.width * 4);
    refresh_parameter_struct();
}
void VideoFrameMemory::update_metadata_static(void* handler, VideoMetaData metadata) {
    VideoFrameMemory* ptr = reinterpret_cast<VideoFrameMemory*>(handler);
    if (ptr) ptr->update_metadata(metadata);
}
const VideoMetaData& VideoFrameMemory::metadata() { return metadata_; }
std::size_t          VideoFrameMemory::size() const { return buffer_.size(); };
std::uint8_t&        VideoFrameMemory::at(std::size_t index) { return buffer_.at(index); };
void                 VideoFrameMemory::refresh_parameter_struct() {
    parameter_instance_.handler       = reinterpret_cast<void*>(this);
    parameter_instance_.current_frame = current_frame_;
    parameter_instance_.frame_buffer  = buffer_.data();
    parameter_instance_.metadata      = metadata_;

    parameter_.type  = VariableType::Video;
    parameter_.value = reinterpret_cast<void*>(&parameter_instance_);
}

std::size_t VideoFrameMemory::get_memory_usage() {
    return sizeof(VideoFrameMemory) + buffer_.size() * sizeof(buffer_.at(0));
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//          AudioParamMemory         //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

AudioParamMemory::AudioParamMemory() : ParameterMemory(VariableType::Audio) {}
void AudioParamMemory::update_metadata(AudioMetaData metadata) {
    metadata_ = metadata;
    buffer_.resize(metadata.channels * metadata.total_samples);
    refresh_parameter_struct();
}
void AudioParamMemory::update_metadata_static(void* handler, AudioMetaData metadata) {
    AudioParamMemory* ptr = reinterpret_cast<AudioParamMemory*>(handler);
    if (ptr) ptr->update_metadata(metadata);
}
const AudioMetaData& AudioParamMemory::metadata() { return metadata_; }
std::size_t          AudioParamMemory::size() const { return buffer_.size(); };
double&              AudioParamMemory::at(std::size_t index) { return buffer_.at(index); };
void                 AudioParamMemory::refresh_parameter_struct() {
    parameter_instance_.handler  = reinterpret_cast<void*>(this);
    parameter_instance_.buffer   = buffer_.data();
    parameter_instance_.metadata = metadata_;

    parameter_.type  = VariableType::Audio;
    parameter_.value = reinterpret_cast<void*>(&parameter_instance_);
}

std::size_t AudioParamMemory::get_memory_usage() {
    return sizeof(AudioParamMemory) + buffer_.size() * sizeof(buffer_.at(0));
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//         TextParamMemory           //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
TextParamMemory::TextParamMemory() : ParameterMemory(VariableType::Text) {}
void TextParamMemory::assign_text(const char* text) {
    buffer_ = std::string(text);
    refresh_parameter_struct();
}
void TextParamMemory::assign_text_static(void* handler, const char* text) {
    TextParamMemory* ptr = reinterpret_cast<TextParamMemory*>(handler);
    if (ptr) ptr->assign_text(text);
}
std::string& TextParamMemory::buffer() { return buffer_; }
void         TextParamMemory::refresh_parameter_struct() {
    parameter_instance_.handler = reinterpret_cast<void*>(this);
    parameter_instance_.size    = static_cast<std::uint64_t>(buffer_.size());
    parameter_instance_.buffer  = buffer_.data();

    parameter_.type  = VariableType::Text;
    parameter_.value = reinterpret_cast<void*>(&parameter_instance_);
}

std::size_t TextParamMemory::get_memory_usage() {
    return sizeof(TextParamMemory) + buffer_.size() * sizeof(buffer_.at(0));
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//        VectorParamMemory          //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
VectorParamMemory::VectorParamMemory(VariableType inner_type)
    : ParameterMemory(VariableType::Vector), inner_type_{inner_type} {}
void VectorParamMemory::allocate_vector(std::uint64_t size) {
    // NOTE: 内部にデフォルトでポインタを保持していないことを要確認
    // NOTE: デフォルト構築できることを確認 (特に，将来的に二重 Vector を許す際には注意)
    memory_buffer_.resize(size);
    for (auto& memory_buffer_instance : memory_buffer_) memory_buffer_instance = make_empty_value({inner_type_});
    refresh_parameter_struct();
}
void VectorParamMemory::allocate_vector_static(void* handler, std::uint64_t size) {
    VectorParamMemory* ptr = reinterpret_cast<VectorParamMemory*>(handler);
    if (ptr) ptr->allocate_vector(size);
}
void VectorParamMemory::refresh_parameter_struct() {
    // TODO: optimize
    vector_buffer_.clear();
    for (const auto& memory : memory_buffer_) vector_buffer_.emplace_back(memory->get_paramter_struct());

    parameter_instance_.handler = reinterpret_cast<void*>(this);
    parameter_instance_.size    = static_cast<std::uint64_t>(vector_buffer_.size());
    parameter_instance_.type    = inner_type_;
    parameter_instance_.buffer  = vector_buffer_.data();

    parameter_.type  = VariableType::Vector;
    parameter_.value = reinterpret_cast<void*>(&parameter_instance_);
}

std::size_t VectorParamMemory::get_memory_usage() {
    std::size_t result_size = sizeof(VectorParamMemory);
    for (const auto& memory : memory_buffer_) result_size += memory->get_memory_usage();
    result_size += vector_buffer_.size() * sizeof(Parameter);
    return result_size;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//        ParameterPackMemory        //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
void ParameterPackMemory::update_types(const std::vector<std::vector<VariableType>>& types) {
    // TODO: optimize
    memory_buffer_.clear();
    for (const auto& type : types) memory_buffer_.emplace_back(make_empty_value(type));
    types_ = types;
    size_  = memory_buffer_.size();
}
std::uint64_t                                       ParameterPackMemory::size() { return size_; }
const std::vector<std::vector<VariableType>>&       ParameterPackMemory::types() { return types_; }
const std::vector<std::shared_ptr<ParameterMemory>> ParameterPackMemory::buffer() { return memory_buffer_; }

const ParameterPack& ParameterPackMemory::get_paramter_struct() {
    // TODO: optimize
    buffer_.clear();
    for (const auto& memory : memory_buffer_) buffer_.push_back(memory->get_paramter_struct());
    parameter_pack_instance_.size       = buffer_.size();
    parameter_pack_instance_.parameters = buffer_.data();
    return parameter_pack_instance_;
}

std::size_t ParameterPackMemory::get_memory_usage() {
    std::size_t result_size = sizeof(ParameterPackMemory);
    for (const auto& memory : memory_buffer_) result_size += memory->get_memory_usage();
    result_size += types_.size() * sizeof(types_.at(0));
    for (const auto& type : types_) result_size += type.size() * sizeof(type.at(0));
    result_size += buffer_.size() * sizeof(Parameter);
    return result_size;
};

}  // namespace memory

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//         to_string helpers         //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
std::string to_string(VariableType variable_type) {
    switch (variable_type) {
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

std::string to_string(PluginType variable_type) {
    switch (variable_type) {
        case PluginType::Macro:
            return "Macro";
        case PluginType::Effect:
            return "Effect";
        default:
            return "(unknown type)";
    }
}
}  // namespace prismcascade
