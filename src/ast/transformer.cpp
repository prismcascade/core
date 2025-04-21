#include <prismcascade/ast/transformer.hpp>

using namespace prismcascade::ast;

/* 静的ヘルパは空実装 ------------------------------------------------ */
void Transformer::remove_parent_ref(const std::shared_ptr<Node>&, const std::shared_ptr<Node>&) {}
void Transformer::add_parent_ref(const std::shared_ptr<Node>&, const std::shared_ptr<Node>&) {}

/* 以下 7 関数はすべて「空の DiffStep を返すだけ」 ------------------- */
static DiffStep makeStep(DiffStep::Type t) {
    DiffStep s;
    s.action = t;
    return s;
}

DiffStep Transformer::insert_node(const std::shared_ptr<Node>&, int, std::shared_ptr<Node>) {
    return makeStep(DiffStep::Type::InsertNode);
}
DiffStep Transformer::delete_node(std::shared_ptr<Node>) { return makeStep(DiffStep::Type::DeleteNode); }
DiffStep Transformer::replace_node(const std::shared_ptr<Node>&, int, Node::Input) {
    return makeStep(DiffStep::Type::ReplaceNode);
}
DiffStep Transformer::connect_input(std::shared_ptr<Node>, int, Node::Input) {
    return makeStep(DiffStep::Type::ConnectInput);
}
DiffStep Transformer::disconnect_input(std::shared_ptr<Node>, int) { return makeStep(DiffStep::Type::DisconnectInput); }
DiffStep Transformer::reconnect_sub_edge(std::shared_ptr<Node>, int, SubEdge) {
    return makeStep(DiffStep::Type::ReconnectSubEdge);
}
