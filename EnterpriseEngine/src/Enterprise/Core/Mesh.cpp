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

std::unique_ptr<Mesh> Mesh::CreateDemoCube( CommandList &commandList, UINT size )
{
    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
    // Cube is centered at 0,0,0.
    float s = size * 0.5f;

    // 8 edges of cube.
    DirectX::XMFLOAT3 p[8] = { { s, s, -s }, { s, s, s },   { s, -s, s },   { s, -s, -s },
                               { -s, s, s }, { -s, s, -s }, { -s, -s, -s }, { -s, -s, s } };
    // 6 face normals
    DirectX::XMFLOAT3 n[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
    // 4 unique texture coordinates
    DirectX::XMFLOAT2 t[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

    // Indices for the vertex positions.
    uint16_t i[24] = {
        0, 1, 2, 3,  // +X
        4, 5, 6, 7,  // -X
        4, 1, 0, 5,  // +Y
        2, 7, 6, 3,  // -Y
        1, 4, 7, 2,  // +Z
        5, 0, 3, 6   // -Z
    };

    std::vector<VertexPosNormalTexture> vertexArray;
    std::vector<DWORD>                  indexArray;

    for ( uint16_t f = 0; f < 6; ++f )  // For each face of the cube.
    {
        // Four vertices per face.
        vertexArray.emplace_back( p[i[f * 4 + 0]], n[f], t[0] );
        vertexArray.emplace_back( p[i[f * 4 + 1]], n[f], t[1] );
        vertexArray.emplace_back( p[i[f * 4 + 2]], n[f], t[2] );
        vertexArray.emplace_back( p[i[f * 4 + 3]], n[f], t[3] );

        // First triangle.
        indexArray.emplace_back( f * 4 + 0 );
        indexArray.emplace_back( f * 4 + 1 );
        indexArray.emplace_back( f * 4 + 2 );

        // Second triangle
        indexArray.emplace_back( f * 4 + 2 );
        indexArray.emplace_back( f * 4 + 3 );
        indexArray.emplace_back( f * 4 + 0 );
    }


    mesh->Initialize(commandList, vertexArray, indexArray);
    return mesh;
}

void Mesh::Initialize( CommandList& commandList, std::vector<VertexPosNormalTexture> vertexArray, std::vector<DWORD> indexArray)
{
    commandList.CopyVertexBuffer(m_VertexBuffer, vertexArray);
    commandList.CopyIndexBuffer(m_IndexBuffer, indexArray);
    m_IndexCount = static_cast<uint32_t>(indexArray.size());
}

}
