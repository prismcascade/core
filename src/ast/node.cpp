#include <prismcascade/ast/node.hpp>

namespace prismcascade::ast {

Node::Input Node::make_empty_value(const std::vector<VariableType>& types) {
    if (types.empty()) return std::int64_t{0};

    switch (types.front()) {
        case VariableType::Int:
            return std::int64_t{0};
        case VariableType::Bool:
            return false;
        case VariableType::Float:
            return 0.0;
        case VariableType::Text:
            return std::string{};
        case VariableType::Vector:
            return VectorParam{VariableType::Int, 0, nullptr};
        case VariableType::Video:
            return VideoFrame{};
        case VariableType::Audio:
            return AudioParam{};
    }
    return std::int64_t{0};
}

}  // namespace prismcascade::ast
