//
// Created by Peter on 5/14/2025.
//

#ifndef MESH_H
#define MESH_H
#include <DirectXMath.h>
#include <intsafe.h>

#include "IndexBuffer.h"
#include "Material.h"
#include "VertexBuffer.h"
#include "../Core.h"


namespace Enterprise::Core::Graphics {
class ENTERPRISE_API CommandList;

struct VertexPosColor {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Color;
};

struct VertexPosNormalTexture {
    VertexPosNormalTexture(): Position(), Normal(), TexCoord()
    {

    };

    VertexPosNormalTexture( DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 norm, DirectX::XMFLOAT2 texCoord )
        : Position(pos)
          , Normal(norm)
          , TexCoord(texCoord)
    {
    }

    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
};

static VertexPosColor g_Vertices[8] = {
    {DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f)}, // 0
    {DirectX::XMFLOAT3(-1.0f, 1.0f, -1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)}, // 1
    {DirectX::XMFLOAT3(1.0f, 1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f)}, // 2
    {DirectX::XMFLOAT3(1.0f, -1.0f, -1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f)}, // 3
    {DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f)}, // 4
    {DirectX::XMFLOAT3(-1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 1.0f)}, // 5
    {DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)}, // 6
    {DirectX::XMFLOAT3(1.0f, -1.0f, 1.0f), DirectX::XMFLOAT3(1.0f, 0.0f, 1.0f)} // 7
};
// LH coords / cw winding
static WORD g_Indicies[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

class ENTERPRISE_API Mesh {
public:
    Mesh();
    Mesh(const std::vector<VertexPosNormalTexture>& vertArray,const std::vector<uint32_t>& indices, CommandList* commandList);
    ~Mesh() = default;

    void Draw( CommandList &commandList );

    //void CreateMesh( CommandList &commandList, VertexPosColor* vertexArray, WORD* indexArray );

    static std::unique_ptr<Mesh> CreateDemoCube( CommandList& commandList, UINT size );

    void Initialize( CommandList &      commandList, std::vector<VertexPosNormalTexture> vertexArray,
                     std::vector<DWORD> indexArray );

    void AddMaterial( const Material &mat) { m_Materials.push_back(std::move(mat));};

private:
    IndexBuffer                         m_IndexBuffer;
    VertexBuffer                        m_VertexBuffer;
    uint32_t                            m_IndexCount;
    std::shared_ptr<Texture>            m_pTexture;
    std::vector<Material>               m_Materials;
};
}

#endif //MESH_H
