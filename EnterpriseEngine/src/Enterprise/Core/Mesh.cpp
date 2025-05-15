//
// Created by Peter on 5/14/2025.
//

#include "Mesh.h"

#include "CommandList.h"
namespace Enterprise::Core::Graphics {


Mesh::Mesh()
    : m_IndexCount( 0 )
{
}

void Mesh::Draw( CommandList &commandList )
{
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.SetVertexBuffer(0, m_VertexBuffer);
    commandList.SetIndexBuffer(m_IndexBuffer);
    commandList.DrawIndexed(m_IndexCount);
}

std::unique_ptr<Mesh> Mesh::CreateDemoCube( CommandList &commandList )
{
    UINT i = std::size(g_Vertices);
    std::vector<VertexPosColor> vertexArray{};
    vertexArray.assign(g_Vertices, g_Vertices + i);

    std::vector<DWORD> indexArray{};
    i = std::size(g_Indicies);
    indexArray.assign(g_Indicies, g_Indicies + i);

    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
    mesh->Initialize(commandList, vertexArray, indexArray);

    return mesh;
}

void Mesh::Initialize( CommandList& commandList, std::vector<VertexPosColor> vertexArray, std::vector<DWORD> indexArray)
{
    commandList.CopyVertexBuffer(m_VertexBuffer, vertexArray);
    commandList.CopyIndexBuffer(m_IndexBuffer, indexArray);
    m_IndexCount = static_cast<uint32_t>(indexArray.size());
}

}
