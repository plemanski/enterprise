//
// Created by Peter on 5/16/2025.
//

#include "Camera.h"

// Used to get access to -, +, *, / operators
using namespace DirectX;
namespace Enterprise::Core {

Camera::Camera()
    : m_Fov(45.0f)
    , m_AspectRatio( 1.0f )
    , m_NearClip(0.1f)
    , m_FarClip(100.0f)
{
    pData = static_cast<AlignedData *>(_aligned_malloc(sizeof(AlignedData), 16));
    pData->m_RotationQ = XMQuaternionIdentity();
    pData->m_Translation = XMVectorZero();
}

Camera::Camera( uint32_t width, uint32_t height )
    :  m_Fov(80.0f)
    , m_AspectRatio ( static_cast<float>(width)  / static_cast<float>(height) )
    , m_NearClip(0.1f)
    , m_FarClip(100.0f)
{
    pData = static_cast<AlignedData *>(_aligned_malloc(sizeof(AlignedData), 16));
    pData->m_RotationQ = XMQuaternionIdentity();
    pData->m_Translation = XMVectorSet(0.0,1.0, -15.0,0);
}

XMMATRIX Camera::GetLookAtViewMatrix( XMFLOAT3* point ) const
{
    UpdateLookAtMatrix(point);
    return pData->m_ViewMatrix;
}

XMMATRIX Camera::GetViewMatrix() const
{
    UpdateViewMatrix();
    return pData->m_ViewMatrix;
}

XMMATRIX Camera::GetProjectionMatrix() const
{
    UpdateProjectionMatrix();
    return pData->m_ProjectionMatrix;
}


void Camera::SetProjection( float fovY, float aspectRatio, float nearClip, float farClip )
{
    m_Fov = fovY;
    m_AspectRatio = aspectRatio;
    m_NearClip = nearClip;
    m_FarClip = farClip;

}

void Camera::UpdateViewMatrix() const
{
    XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion( pData->m_RotationQ ));
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector( -(pData->m_Translation) );

    pData->m_ViewMatrix = translationMatrix * rotationMatrix;
}

void Camera::UpdateLookAtMatrix(XMFLOAT3* point) const
{
    auto data = pData;
    pData->m_ViewMatrix = XMMatrixLookAtLH(data->m_Translation, XMLoadFloat3(point), XMVectorSet(0.0,1.0,0.0,0.0));
}

void Camera::UpdateProjectionMatrix() const
{
    pData->m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_Fov), m_AspectRatio, m_NearClip, m_FarClip );
}

}