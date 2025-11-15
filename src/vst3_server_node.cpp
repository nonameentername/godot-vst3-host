#include "vst3_server_node.h"
#include "vst3_server.h"

using namespace godot;

Vst3ServerNode::Vst3ServerNode() {
}

Vst3ServerNode::~Vst3ServerNode() {
}

void Vst3ServerNode::_process() {
    Vst3Server::get_singleton()->process();
}

void Vst3ServerNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("process"), &Vst3ServerNode::_process);
}
