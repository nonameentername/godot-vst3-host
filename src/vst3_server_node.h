#ifndef VST3_SERVER_NODE_H
#define VST3_SERVER_NODE_H

#include <godot_cpp/classes/node.hpp>

namespace godot {

class Vst3ServerNode : public Node {
    GDCLASS(Vst3ServerNode, Node);

private:
protected:
public:
    Vst3ServerNode();
    ~Vst3ServerNode();

    void _process();

    static void _bind_methods();
};
} // namespace godot

#endif
