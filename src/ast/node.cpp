#include <prismcascade/ast/node.hpp>

namespace prismcascade::ast {

void AstNode::resize_inputs(const std::vector<std::vector<VariableType>>& types) {
    inputs.resize(types.size());
    input_window.resize(types.size());
    input_parameters->update_types(types);
}
void AstNode::resize_outputs(const std::vector<std::vector<VariableType>>& types) {
    output_parameters->update_types(types);
}

}  // namespace prismcascade::ast
