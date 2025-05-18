//
// Created by Peter on 5/18/2025.
//

#ifndef LIGHT_H
#define LIGHT_H

#include "DirectXMath.h"


namespace Enterprise::Core::Graphics{
struct LightSB{
    DirectX::XMFLOAT4 PositionWS;
    //----------------------------16 bytes
    DirectX::XMFLOAT4 PositionVS;
    //----------------------------16 bytes
    DirectX::XMFLOAT4 Colour;
    //----------------------------16 bytes
    DirectX::XMFLOAT4 DirectionWS;
    //----------------------------16 bytes
    DirectX::XMFLOAT4 DirectionVS;
    //----------------------------16 bytes
    DirectX::XMFLOAT4 Padding;
    DirectX::XMFLOAT4 Padding1;
    DirectX::XMFLOAT4 Padding2;

    //----------------------------16bytes
    // Total:                     16*6 = 96
};
class Light {
public:
    Light(DirectX::XMFLOAT4 lightColour, float_t ambientStrength,
        DirectX::XMFLOAT4 lightPos, DirectX::XMVECTOR lightDirection)
        : m_Colour(lightColour)
        , m_AmbientStrength( ambientStrength )
        , m_LightPos( lightPos )
        , m_LightDirectionNorm( DirectX::XMVector4Normalize(lightDirection) )
    {}

    void SetLightDirection(DirectX::XMVECTOR lightDir){ m_LightDirectionNorm = DirectX::XMVector3Normalize(lightDir); }


private:
    DirectX::XMFLOAT4 m_Colour;
    float_t m_AmbientStrength;
    DirectX::XMFLOAT4 m_LightPos;
    DirectX::XMVECTOR m_LightDirectionNorm;
};



}
#endif //LIGHT_H
