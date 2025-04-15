#pragma once

#include <string>
#include <common/types.hpp>
#include <plugin/data_types.hpp>

namespace PrismCascade {

extern "C" {

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
// プラグイン側とやり取りする時に使う用の型

struct PluginMetaData {
    std::int32_t protocol_version{};
    PluginType type{};
    TextParam uuid;
    TextParam name;
};

}

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
// ホスト側で管理する時用の型

struct PluginMetaDataInternal {
    std::int32_t protocol_version{};
    PluginType type{};
    std::string uuid;
    std::string name;
};

}
