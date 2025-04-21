// src/plugin/dll_memory_manager.cpp
#include <stdarg.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <plugin/dll_memory_manager.hpp>
#include <stdexcept>
#include <unordered_map>

using namespace prismcascade;
using namespace prismcascade::plugin;

/*──────────────────────────────────────────────────────────────────*/
/*  private helpers                                                 */
/*──────────────────────────────────────────────────────────────────*/
namespace {

template <class T>
inline std::shared_ptr<void> make_buffer(std::size_t n) {
    return std::shared_ptr<void>(static_cast<void*>(new T[n]), [](void* p) { delete[] static_cast<T*>(p); });
}

template <class Map, class Key>
typename Map::mapped_type& require(Map& m, const Key& k, const char* what) {
    auto it = m.find(k);
    if (it == m.end()) throw std::runtime_error(what);
    return it->second;
}

}  // namespace

/*──────────────────────────────────────────────────────────────────*/
/*  allocate_param                                                  */
/*──────────────────────────────────────────────────────────────────*/
bool DllMemoryManager::allocate_param(std::int64_t plugin_handler, bool is_output, VariableType type, const char* name,
                                      std::optional<VariableType> inner) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto&                       list = plugin_param_info_[plugin_handler].io_types[is_output ? 1 : 0];
    if (inner)
        list.emplace_back(name, std::vector<VariableType>{type, *inner});
    else
        list.emplace_back(name, std::vector<VariableType>{type});
    return true;
}

bool DllMemoryManager::allocate_param_static(void* self, std::int64_t plugin_handler, bool is_output, VariableType type,
                                             const char* name, ...) {
    va_list ap;
    va_start(ap, name);
    std::optional<VariableType> inner;
    if (type == VariableType::Vector) inner = static_cast<VariableType>(va_arg(ap, int));
    va_end(ap);
    return static_cast<DllMemoryManager*>(self)->allocate_param(plugin_handler, is_output, type, name, inner);
}

/*──────────────────────────────────────────────────────────────────*/
/*  空実装 : required/handleable handler & load_video_buffer        */
/*──────────────────────────────────────────────────────────────────*/
bool DllMemoryManager::add_required_handler(std::int64_t, const char*) { return true; }
bool DllMemoryManager::add_required_handler_static(void* self, std::int64_t h, const char* n) {
    return static_cast<DllMemoryManager*>(self)->add_required_handler(h, n);
}

bool DllMemoryManager::add_handleable_effect(std::int64_t, const char*) { return true; }
bool DllMemoryManager::add_handleable_effect_static(void* self, std::int64_t h, const char* n) {
    return static_cast<DllMemoryManager*>(self)->add_handleable_effect(h, n);
}

bool DllMemoryManager::load_video_buffer(VideoFrame*, std::uint64_t) { return true; }
bool DllMemoryManager::load_video_buffer_static(void* self, VideoFrame* v, std::uint64_t f) {
    return static_cast<DllMemoryManager*>(self)->load_video_buffer(v, f);
}

/*──────────────────────────────────────────────────────────────────*/
/*  ParameterPack alloc / free                                      */
/*──────────────────────────────────────────────────────────────────*/
std::pair<ParameterPack, ParameterPack> DllMemoryManager::allocate_plugin_parameters(std::int64_t plugin_h,
                                                                                     std::int64_t inst_h) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto&                       info = require(plugin_param_info_, plugin_h, "plugin not registered");

    ParameterPack packs[2]{};

    for (int io = 0; io < 2; ++io) {
        auto& defs      = info.io_types[io];
        auto  param_arr = std::shared_ptr<Parameter>(new Parameter[defs.size()], [](Parameter* p) { delete[] p; });

        std::vector<std::shared_ptr<void>> buffers;

        for (std::size_t i = 0; i < defs.size(); ++i) {
            auto& [nm, types]       = defs[i];
            VariableType          t = types[0];
            std::shared_ptr<void> buf;

            switch (t) {
                case VariableType::Int:
                    buf = make_buffer<std::int64_t>(1);
                    break;
                case VariableType::Bool:
                    buf = make_buffer<bool>(1);
                    break;
                case VariableType::Float:
                    buf = make_buffer<double>(1);
                    break;
                case VariableType::Text:
                    buf = std::make_shared<TextParam>();
                    break;
                case VariableType::Vector:
                    buf = std::make_shared<VectorParam>();
                    break;
                case VariableType::Video:
                    buf = std::make_shared<VideoFrame>();
                    break;
                case VariableType::Audio:
                    buf = std::make_shared<AudioParam>();
                    break;
            }

            param_arr.get()[i].type  = t;
            param_arr.get()[i].value = buf.get();
            buffers.push_back(buf);
        }

        parameter_pack_instances_[inst_h].plugin_handler = plugin_h;
        parameter_pack_instances_[inst_h].io_buffers[io] = {param_arr, buffers};

        packs[io].size       = static_cast<std::int32_t>(defs.size());
        packs[io].parameters = param_arr.get();
    }
    return {packs[0], packs[1]};
}

bool DllMemoryManager::free_plugin_parameters(std::int64_t inst_h) {
    std::lock_guard<std::mutex> lk(mutex_);
    return parameter_pack_instances_.erase(inst_h) > 0;
}

/*──────────────────────────────────────────────────────────────────*/
/*  TextParam                                                       */
/*──────────────────────────────────────────────────────────────────*/
bool DllMemoryManager::assign_text(TextParam* dst, const char* s) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto                        sp = std::make_shared<std::string>(s);
    text_pool_[dst]                = sp;
    dst->size                      = static_cast<std::int32_t>(sp->size());
    dst->buffer                    = sp->c_str();
    return true;
}
bool DllMemoryManager::assign_text_static(void* self, TextParam* b, const char* s) {
    return static_cast<DllMemoryManager*>(self)->assign_text(b, s);
}

void DllMemoryManager::copy_text(TextParam* dst, TextParam* src) {
    std::lock_guard<std::mutex> lk(mutex_);
    dst->buffer     = src->buffer;
    dst->size       = src->size;
    text_pool_[dst] = text_pool_[src];
}
void DllMemoryManager::free_text(TextParam* b) {
    std::lock_guard<std::mutex> lk(mutex_);
    text_pool_.erase(b);
}

/*──────────────────────────────────────────────────────────────────*/
/*  VectorParam                                                     */
/*──────────────────────────────────────────────────────────────────*/
bool DllMemoryManager::allocate_vector(VectorParam* dst, std::int32_t sz) {
    std::lock_guard<std::mutex> lk(mutex_);
    std::shared_ptr<void>       buf;

    switch (dst->type) {
        case VariableType::Int:
            buf = make_buffer<std::int64_t>(sz);
            break;
        case VariableType::Bool:
            buf = make_buffer<bool>(sz);
            break;
        case VariableType::Float:
            buf = make_buffer<double>(sz);
            break;
        case VariableType::Text:
            buf = make_buffer<TextParam>(sz);
            break;
        case VariableType::Video:
            buf = make_buffer<VideoFrame>(sz);
            break;
        case VariableType::Audio:
            buf = make_buffer<AudioParam>(sz);
            break;
        default:
            throw std::runtime_error("allocate_vector: unsupported type");
    }

    vector_pool_[dst] = buf;
    dst->size         = sz;
    dst->buffer       = buf.get();
    return true;
}
bool DllMemoryManager::allocate_vector_static(void* self, VectorParam* b, int s) {
    return static_cast<DllMemoryManager*>(self)->allocate_vector(b, s);
}

void DllMemoryManager::copy_vector(VectorParam* d, VectorParam* s) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto&                       sp = require(vector_pool_, s, "copy_vector");
    vector_pool_[d]                = sp;
    d->size                        = s->size;
    d->type                        = s->type;
    d->buffer                      = sp.get();
}
void DllMemoryManager::free_vector(VectorParam* b) {
    std::lock_guard<std::mutex> lk(mutex_);
    vector_pool_.erase(b);
}

/*──────────────────────────────────────────────────────────────────*/
/*  VideoFrame / AudioParam                                         */
/*──────────────────────────────────────────────────────────────────*/
bool DllMemoryManager::allocate_video(VideoFrame* dst, VideoMetaData md) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto                        vec = std::make_shared<std::vector<std::uint8_t>>(md.width * md.height * 4);
    video_pool_[dst]                = vec;
    *dst                            = {};
    dst->metadata                   = md;
    dst->frame_buffer               = vec->data();
    return true;
}
bool DllMemoryManager::allocate_video_static(void* self, VideoFrame* b, VideoMetaData m) {
    return static_cast<DllMemoryManager*>(self)->allocate_video(b, m);
}

void DllMemoryManager::copy_video(VideoFrame* d, VideoFrame* s) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto&                       sp = require(video_pool_, s, "copy_video");
    video_pool_[d]                 = sp;
    *d                             = *s;
    d->frame_buffer                = sp->data();
}
void DllMemoryManager::free_video(VideoFrame* b) {
    std::lock_guard<std::mutex> lk(mutex_);
    video_pool_.erase(b);
}

bool DllMemoryManager::allocate_audio(AudioParam* dst) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto                        vec = std::make_shared<std::vector<double>>(44100);
    audio_pool_[dst]                = vec;
    dst->buffer                     = vec->data();
    return true;
}
bool DllMemoryManager::allocate_audio_static(void* self, AudioParam* b) {
    return static_cast<DllMemoryManager*>(self)->allocate_audio(b);
}

void DllMemoryManager::copy_audio(AudioParam* d, AudioParam* s) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto&                       sp = require(audio_pool_, s, "copy_audio");
    audio_pool_[d]                 = sp;
    d->buffer                      = sp->data();
}
void DllMemoryManager::free_audio(AudioParam* b) {
    std::lock_guard<std::mutex> lk(mutex_);
    audio_pool_.erase(b);
}

/*──────────────────────────────────────────────────────────────────*/
void DllMemoryManager::dump_memory_usage() {
    std::lock_guard<std::mutex> lk(mutex_);
    std::size_t                 txt = 0, vec = vector_pool_.size(), vid = video_pool_.size(), aud = audio_pool_.size();
    for (auto& kv : text_pool_)
        if (kv.second.use_count() == 1) txt++;
    std::cerr << "[DllMemoryManager] Text=" << txt << " Vector=" << vec << " Video=" << vid << " Audio=" << aud << "\n";
}
