//
// Created by Peter on 5/18/2025.
//

#ifndef SCENE_H
#define SCENE_H
#include <memory>

namespace Enterprise::Scene {

class Scene {
public:
    Scene();
    ~Scene();
    bool LoadScene();
    void RenderScene();

private:
    template<typename T>
    struct Node : std::enable_shared_from_this<Node<T>>{
        std::shared_ptr<Node> m_Parent;
        std::shared_ptr<Node> m_Children[];
    };

};

}

#endif //SCENE_H
