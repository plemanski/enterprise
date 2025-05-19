//
// Created by Peter on 5/18/2025.
//

#include "Model.h"

#include "Renderer.h"
#include "../Log.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

using namespace Enterprise::Core::Graphics;

void ProcessVertices( const aiMesh* _mesh, std::vector<VertexPosNormalTexture>* vertexArray)
{
    for ( auto x = 0; x < _mesh->mNumVertices; ++x)
    {
        auto vertex = _mesh->mVertices + x;
        auto normal = _mesh->mNormals + x;
        auto texCoords = _mesh->mTextureCoords + x;
        VertexPosNormalTexture vertPosNormTex{};
        vertPosNormTex.Position = DirectX::XMFLOAT3(vertex->x, vertex->y, vertex->z);
        vertPosNormTex.Normal = DirectX::XMFLOAT3(normal->x,normal->y, normal->z);
        if (_mesh->mTextureCoords[0])
        {
            vertPosNormTex.TexCoord = DirectX::XMFLOAT2(_mesh->mTextureCoords[0][x].x, _mesh->mTextureCoords[0][x].y);
        } else
        {
            vertPosNormTex.TexCoord = DirectX::XMFLOAT2(0.0,0.0);
        }
        vertexArray->emplace_back(vertPosNormTex);
    }
};

void ProcessIndicies( const aiMesh* _mesh, std::vector<uint32_t>* indexArray)
{
    for (auto x = 0; x < _mesh->mNumFaces; ++x)
    {
        auto face = _mesh->mFaces[x];
        for (auto j = 0; j < face.mNumIndices; ++j)
        {
            indexArray->emplace_back(face.mIndices[j]);
        }
    }
}

void ProcessMeshes( const aiScene* scene, const aiNode* node, Model* model, CommandList* commandList)
{
    for (auto i = 0; i < node->mNumMeshes; ++i )
    {
        auto nodeMesh = node->mMeshes[i];
        auto _mesh = scene->mMeshes[nodeMesh];
        std::vector<VertexPosNormalTexture> vertexArray;
        vertexArray.reserve(_mesh->mNumVertices);
        ProcessVertices(_mesh, &vertexArray);
        std::vector<uint32_t> indexArray;
        indexArray.reserve(_mesh->mNumVertices);
        ProcessIndicies(_mesh, &indexArray);
        model->AddMesh(vertexArray, indexArray, commandList);
    }
};

namespace Enterprise::Core::Graphics {


aiNode* Model::ProcessNode(const aiScene* scene, aiNode* node, Model* model, CommandList* commandList)
{
    if (node->mNumMeshes != 0)
    {
        ProcessMeshes(scene, node, model, commandList);
    }
    for (auto x = 0; x < node->mNumChildren; x++)
    {
        ProcessNode(scene, node->mChildren[x], model, commandList);
    }
    if ( node->mNumChildren == 0 )
    {
        return nullptr;
    }
    return nullptr;

};


bool Model::ImportModel( const std::string &pFile, Model* model, CommandList* commandList )
{
    Assimp::Importer importer;
    model->m_Meshes.reserve(16);
    const aiScene* scene = importer.ReadFile( pFile,
        aiProcess_ConvertToLeftHanded |
        aiProcessPreset_TargetRealtime_Quality);

    if (scene == nullptr)
    {
        EE_CORE_ERROR("Unable to import model; {}", pFile);
        return false;
    }
    auto node = scene->mRootNode;
    while (node != nullptr)
    {
        node = model->ProcessNode(scene, scene->mRootNode, model, commandList);
    }

    return true;
}

void Model::Draw(CommandList& commandList) const
{
    for (auto&& mesh : m_Meshes)
    {
        mesh->Draw(commandList);
    }
}

}
