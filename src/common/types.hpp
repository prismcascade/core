#pragma once

#include <cstdint>
#include <string>

namespace PrismCascade {

    //-----------------------------------------------------
    // 変数型 (Int, Bool, Float, Text, Vector, Video, Audio)
    // プラグイン種別 (Effect, Macro) などの列挙
    //-----------------------------------------------------
    enum class VariableType {
        Int,
        Bool,
        Float,
        Text,
        Vector,
        Video,
        Audio
    };

    enum class PluginType {
        Effect,
        Macro
    };

    //-----------------------------------------------------
    // 補助関数
    //-----------------------------------------------------
    inline std::string to_string(VariableType t) {
        switch(t) {
        case VariableType::Int:    return "Int";
        case VariableType::Bool:   return "Bool";
        case VariableType::Float:  return "Float";
        case VariableType::Text:   return "Text";
        case VariableType::Vector: return "Vector";
        case VariableType::Video:  return "Video";
        case VariableType::Audio:  return "Audio";
        }
        return "Unknown";
    }
    
    inline std::string to_string(PluginType pt) {
        switch(pt) {
        case PluginType::Effect: return "Effect";
        case PluginType::Macro:  return "Macro";
        }
        return "Unknown";
    }
}
