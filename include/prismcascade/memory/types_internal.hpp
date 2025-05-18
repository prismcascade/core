#pragma once

#include <memory>
#include <prismcascade/common/types.hpp>
#include <string>
#include <vector>

namespace prismcascade {
namespace memory {

// パラメータのメモリ実体
class ParameterMemory {
   public:
    const VariableType       type_{};
    virtual const Parameter& get_paramter_struct();
    virtual ~ParameterMemory()             = default;  // デストラクタを vtable に乗せる
    virtual std::size_t get_memory_usage() = 0;        // 単位: バイト

   protected:
    ParameterMemory(VariableType type);  // 基底クラスの直接生成は禁止
    Parameter    parameter_;
    virtual void refresh_parameter_struct() = 0;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

std::shared_ptr<ParameterMemory> make_empty_value(const std::vector<VariableType>& types);

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

class IntParamMemory : public ParameterMemory {
   public:
    IntParamMemory();
    std::int64_t& buffer();
    std::size_t   get_memory_usage() override;

   protected:
    void         refresh_parameter_struct() override;
    std::int64_t parameter_instance_{};
};

class BoolParamMemory : public ParameterMemory {
   public:
    BoolParamMemory();
    bool&       buffer();
    std::size_t get_memory_usage() override;

   protected:
    void refresh_parameter_struct() override;
    bool parameter_instance_{};
};

class FloatParamMemory : public ParameterMemory {
   public:
    FloatParamMemory();
    double&     buffer();
    std::size_t get_memory_usage() override;

   protected:
    void   refresh_parameter_struct() override;
    double parameter_instance_{};
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

class VideoFrameMemory : public ParameterMemory {
   public:
    VideoFrameMemory();
    void                 update_metadata(VideoMetaData metadata);
    static void          update_metadata_static(void* handler, VideoMetaData metadata);  // TODO: 不要なら削除
    const VideoMetaData& metadata();
    std::size_t          size() const;
    std::uint8_t&        at(std::size_t index);
    std::size_t          get_memory_usage() override;

   protected:
    std::uint64_t             current_frame_ = 0;
    void                      refresh_parameter_struct() override;
    VideoFrame                parameter_instance_{};
    VideoMetaData             metadata_{};
    std::vector<std::uint8_t> buffer_;
};

class AudioParamMemory : public ParameterMemory {
   public:
    AudioParamMemory();
    void                 update_metadata(AudioMetaData metadata);
    static void          update_metadata_static(void* handler, AudioMetaData metadata);  // TODO: 不要なら削除
    const AudioMetaData& metadata();
    std::size_t          size() const;
    double&              at(std::size_t index);
    std::size_t          get_memory_usage() override;

   protected:
    void                refresh_parameter_struct() override;
    AudioParam          parameter_instance_{};
    AudioMetaData       metadata_{};
    std::vector<double> buffer_;
};

struct TextParamMemory : public ParameterMemory {
   public:
    TextParamMemory();
    void         assign_text(const char* text);
    static void  assign_text_static(void* handler, const char* text);
    std::string& buffer();  // 直接書き込み可
    std::size_t  get_memory_usage() override;

   protected:
    void        refresh_parameter_struct() override;
    TextParam   parameter_instance_{};
    std::string buffer_;
};

struct VectorParamMemory : public ParameterMemory {
   public:
    VectorParamMemory() = delete;
    VectorParamMemory(VariableType inner_type);
    void                             allocate_vector(std::uint64_t size);
    static void                      allocate_vector_static(void* handler, std::uint64_t size);
    const VariableType               inner_type_;
    std::size_t                      size() const;
    std::shared_ptr<ParameterMemory> at(std::size_t index);
    std::size_t                      get_memory_usage() override;

   protected:
    void                                          refresh_parameter_struct() override;
    VectorParam                                   parameter_instance_{};
    std::vector<std::shared_ptr<ParameterMemory>> memory_buffer_;
    std::vector<Parameter>                        vector_buffer_;
};

// 入出力パラメータパック
class ParameterPackMemory {
   public:
    const ParameterPack&                          get_paramter_struct();
    void                                          update_types(const std::vector<std::vector<VariableType>>& types);
    std::uint64_t                                 size();
    const std::vector<std::vector<VariableType>>& types();
    const std::vector<std::shared_ptr<ParameterMemory>> buffer();
    std::size_t                                         get_memory_usage();

   private:
    std::uint64_t                                 size_ = 0;
    std::vector<std::vector<VariableType>>        types_;
    std::vector<std::shared_ptr<ParameterMemory>> memory_buffer_;

    ParameterPack          parameter_pack_instance_{};
    std::vector<Parameter> buffer_;
};
}

// プラグインのメタデータ
std::string to_string(VariableType variable_type);
std::string to_string(PluginType variable_type);

struct PluginMetaDataInternal {
    std::uint64_t protocol_version = 1;
    PluginType    type{};
    std::string   uuid;
    std::string   name;
};

}  // namespace prismcascade
