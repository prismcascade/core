// #pragma once
// /*  Prism Cascade – DLL‑side Memory Manager
//     --------------------------------------
//     ・ホスト⇔プラグイン間で受け渡される (TextParam / VectorParam /
//       VideoFrame / AudioParam / ParameterPack …) の実メモリを集中管理。
//     ・各バッファは shared_ptr で所有権を共有し、
//       DLL が unload されてもリークしない。
//     ・全メソッドは **スレッドセーフ**（mutex で排他）。
// */

// #include <array>
// #include <cstdint>
// #include <map>
// #include <memory>
// #include <mutex>
// #include <optional>
// #include <prismcascade/common/types.hpp>  // 変数型・メタデータ
// #include <string>
// #include <tuple>
// #include <utility>
// #include <vector>

// namespace prismcascade::plugin {

// class DllMemoryManager {
//    public:
//     /* -------- パラメータメタ情報生成 (プラグイン→ホスト呼び出し時) -------- */
//     bool allocate_param(std::int64_t plugin_handler, bool is_output, VariableType type, const char* name,
//                         std::optional<VariableType> inner_type);

//     static bool allocate_param_static(void* self, std::int64_t plugin_handler, bool is_output, VariableType type,
//                                       const char* name,
//                                       ...);  // (*inner_type)

//     /* -------- プラグイン依存関係 (未実装) ------------------------------- */
//     bool        add_required_handler(std::int64_t plugin_handler, const char* effect);
//     static bool add_required_handler_static(void*, std::int64_t, const char*);

//     bool        add_handleable_effect(std::int64_t plugin_handler, const char* effect);
//     static bool add_handleable_effect_static(void*, std::int64_t, const char*);

//     /* -------- バッファロード ------------------------------------------- */
//     bool        load_video_buffer(VideoFrame* dst, std::uint64_t frame_idx);
//     static bool load_video_buffer_static(void*, VideoFrame*, std::uint64_t);

//     /* -------- ParameterPack <-> 実バッファ -------------------------------- */
//     std::pair<ParameterPack, ParameterPack> allocate_plugin_parameters(std::int64_t plugin_handler,
//                                                                        std::int64_t plugin_instance_handler);

//     bool free_plugin_parameters(std::int64_t plugin_instance_handler);

//     /* -------- TextParam -------------------------------------------------- */
//     bool        assign_text(TextParam* dst, const char* utf8);
//     static bool assign_text_static(void*, TextParam*, const char*);
//     void        copy_text(TextParam* dst, TextParam* src);
//     void        free_text(TextParam*);

//     /* -------- VectorParam ------------------------------------------------ */
//     bool        allocate_vector(VectorParam* dst, std::int32_t size);
//     static bool allocate_vector_static(void*, VectorParam*, std::int32_t);
//     void        copy_vector(VectorParam* dst, VectorParam* src);
//     void        free_vector(VectorParam*);

//     /* -------- VideoFrame ------------------------------------------------- */
//     bool        allocate_video(VideoFrame* dst, VideoMetaData md);
//     static bool allocate_video_static(void*, VideoFrame*, VideoMetaData);
//     void        copy_video(VideoFrame* dst, VideoFrame* src);
//     void        free_video(VideoFrame*);

//     /* -------- AudioParam ------------------------------------------------- */
//     bool        allocate_audio(AudioParam* dst);
//     static bool allocate_audio_static(void*, AudioParam*);
//     void        copy_audio(AudioParam* dst, AudioParam* src);
//     void        free_audio(AudioParam*);

//     /* -------- デバッグ用 -------------------------------------------------- */
//     void dump_memory_usage();

//    private:
//     /* ハンドラ → 実データの紐付け */
//     std::map<TextParam*, std::shared_ptr<std::string>>                text_pool_;
//     std::map<VectorParam*, std::shared_ptr<void>>                     vector_pool_;
//     std::map<VideoFrame*, std::shared_ptr<std::vector<std::uint8_t>>> video_pool_;
//     std::map<AudioParam*, std::shared_ptr<std::vector<double>>>       audio_pool_;

//     /* プラグインごとのメタ情報／ParameterPack */
//     struct ParamInfo {
//         // (param name, [type | vector<inner_type>])
//         std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2> io_types;
//     };
//     std::map<std::int64_t, ParamInfo> plugin_param_info_;

//     struct PackInstance {
//         std::int64_t plugin_handler{};
//         // input/output 0/1
//         std::array<std::pair<std::shared_ptr<Parameter>, std::vector<std::shared_ptr<void>>>, 2> io_buffers;
//     };
//     std::map<std::int64_t, PackInstance> parameter_pack_instances_;

//     /* 排他制御 */
//     std::mutex mutex_;
// };

// }  // namespace prismcascade::plugin
