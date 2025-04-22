#include <memory>
#include <prismcascade/common/types.hpp>
#include <string>
#include <vector>

namespace prismcascade {

// パラメータのメモリ実体
class ParameterMemory {
   public:
    const VariableType       type_{};
    virtual const Parameter& get_paramter_struct();
    virtual ~ParameterMemory() = default;  // デストラクタを vtable に乗せる

   protected:
    ParameterMemory(VariableType type);  // 基底クラスの直接生成は禁止
    Parameter    parameter_;
    virtual void refresh_parameter_struct() = 0;
};

// -------------------------------- //

std::shared_ptr<ParameterMemory> make_empty_value(const std::vector<VariableType>& types);

// -------------------------------- //

class IntMemory : public ParameterMemory {
   public:
    IntMemory();
    std::int64_t& buffer();

   protected:
    void         refresh_parameter_struct() override;
    std::int64_t parameter_instance_{};
};

class BoolMemory : public ParameterMemory {
   public:
    BoolMemory();
    bool& buffer();

   protected:
    void refresh_parameter_struct() override;
    bool parameter_instance_{};
};

class FloatMemory : public ParameterMemory {
   public:
    FloatMemory();
    double& buffer();

   protected:
    void   refresh_parameter_struct() override;
    double parameter_instance_{};
};

// -------------------------------- //

class VideoFrameMemory : public ParameterMemory {
   public:
    VideoFrameMemory();
    void                 update_metadata(VideoMetaData metadata);
    static void          update_metadata_static(void* handler, VideoMetaData metadata);
    const VideoMetaData& metadata();
    std::size_t          size() const;
    std::uint8_t&        at(std::size_t index);

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
    static void          update_metadata_static(void* handler, AudioMetaData metadata);
    const AudioMetaData& metadata();
    std::size_t          size() const;
    double&              at(std::size_t index);

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

   protected:
    void        refresh_parameter_struct() override;
    TextParam   parameter_instance_{};
    std::string buffer_;
};

struct VectorParamMemory : public ParameterMemory {
   public:
    VectorParamMemory() = delete;
    VectorParamMemory(VariableType inner_type);
    void                             allocate_vector(std::uint32_t size);
    static void                      allocate_vector_static(void* handler, std::uint32_t size);
    const VariableType               inner_type_;
    std::size_t                      size() const;
    std::shared_ptr<ParameterMemory> at(std::size_t index);

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
    std::int32_t                                  size();
    const std::vector<std::vector<VariableType>>& types();
    const std::vector<std::shared_ptr<ParameterMemory>> buffer();

   private:
    std::int32_t                                  size_ = 0;
    std::vector<std::vector<VariableType>>        types_;
    std::vector<std::shared_ptr<ParameterMemory>> memory_buffer_;

    ParameterPack          parameter_pack_instance_{};
    std::vector<Parameter> buffer_;
};

// プラグインのメタデータ
std::string to_string(VariableType variable_type);
std::string to_string(PluginType variable_type);

struct PluginMetaDataInternal {
    std::int32_t protocol_version = 1;
    PluginType   type{};
    std::string  uuid;
    std::string  name;
};

}  // namespace prismcascade
