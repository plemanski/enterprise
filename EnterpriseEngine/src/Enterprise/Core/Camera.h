//
// Created by Peter on 5/16/2025.
//

#ifndef CAMERA_H
#define CAMERA_H
#include <DirectXMath.h>



namespace Enterprise::Core {

class Camera {
public:
    Camera();
    Camera(uint32_t width, uint32_t height);
    virtual ~Camera() = default;

    [[nodiscard]] DirectX::XMMATRIX GetLookAtViewMatrix(DirectX::XMFLOAT3* point) const;
    [[nodiscard]] DirectX::XMMATRIX GetViewMatrix() const;
    [[nodiscard]] DirectX::XMMATRIX GetProjectionMatrix() const;
    [[nodiscard]] DirectX::XMFLOAT4 GetCameraLocation() const;
    void SetProjection( float fovY, float aspectRatio, float nearClip, float farClip);


protected:
    virtual void UpdateViewMatrix() const;
    virtual void UpdateLookAtMatrix(DirectX::XMFLOAT3 *point) const;
    virtual void UpdateProjectionMatrix() const;

protected:
    XM_ALIGNED_DATA(16) struct AlignedData {
        DirectX::XMFLOAT4 m_Translation;

        DirectX::XMFLOAT4 m_RotationQ;
        DirectX::XMMATRIX m_ViewMatrix, m_InverseViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix, m_InverseProjectionMatrix;
    };
    AlignedData* pData;

    float m_Fov;
    float m_AspectRatio;
    float m_NearClip;
    float m_FarClip;
};



}
#endif //CAMERA_H
