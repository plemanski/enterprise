//
// Created by Peter on 5/18/2025.
//

#ifndef LIGHT_H
#define LIGHT_H

#include "DirectXMath.h"


namespace Enterprise::Core::Graphics {
struct LightSB {
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
    DirectX::XMFLOAT4 CameraWS;
    //----------------------------16 bytes
    float_t AmbientStrength;
    //-----------------------------4 bytes
    float_t SpecularStrength;
    //-----------------------------4 bytes
    DirectX::XMFLOAT4 Padding;
    //----------------------------16 bytes
    DirectX::XMFLOAT2 Padding2;
    //-----------------------------8 bytes
    //---------------------Total: 128bytes
};

class Light {
public:
    Light( DirectX::XMFLOAT4 lightColour, float_t        ambientStrength, float_t specularStrength,
           DirectX::XMFLOAT4 lightPos, DirectX::XMVECTOR lightDirection )
        : m_Colour(lightColour)
          , m_AmbientStrength(ambientStrength)
          , m_SpecularStrength(specularStrength)
          , m_LightPos(lightPos)
          , m_LightDirectionNorm(DirectX::XMVector4Normalize(lightDirection))
    {
    }

    void SetLightDirection( DirectX::XMVECTOR lightDir )
    {
        m_LightDirectionNorm = DirectX::XMVector3Normalize(lightDir);
    }

private:
    DirectX::XMFLOAT4 m_Colour;
    float_t           m_AmbientStrength;
    float_t           m_SpecularStrength;
    DirectX::XMFLOAT4 m_LightPos;
    DirectX::XMVECTOR m_LightDirectionNorm;
};
}
#endif //LIGHT_H
