//
// Created by Peter on 5/18/2025.
//

#ifndef MODEL_H
#define MODEL_H

#include <DirectXMath.h>
#include "Mesh.h"
#include "assimp/scene.h"


struct aiNode;

namespace Enterprise::Core::Graphics {
struct Matrices {
    DirectX::XMMATRIX ModelMatrix;
    DirectX::XMMATRIX ModelViewMatrix;
    DirectX::XMMATRIX InverseTransposeModelViewMatrix;
    DirectX::XMMATRIX ModelViewProjectionMatrix;
};

class Model {
public:
    Model(): m_PositionWS(DirectX::XMVECTOR()), m_NumMeshes(0), m_NumInstances(1)
    {
    };


    ~Model() = default;

    aiNode *ProcessNode( const aiScene* scene, aiNode* node, Model* model, CommandList* commandList );

    static bool ImportModel( const std::string &pFile, Model* model, CommandList* commandList );

    void Draw( CommandList &commandList ) const;

    void AddMesh( const std::vector<VertexPosNormalTexture> &verts, const std::vector<uint32_t> &indices,
                  CommandList*                               commandList )
    {
        m_Meshes.emplace_back(std::make_unique<Mesh>(verts, indices, commandList));
        m_NumMeshes += 1;
    }

private:
    DirectX::XMVECTOR                      m_PositionWS;
    uint8_t                                m_NumInstances;
    std::vector<std::unique_ptr<Mesh> >    m_Meshes;
    uint8_t                                m_NumMeshes;
};
}

#endif //MODEL_H
