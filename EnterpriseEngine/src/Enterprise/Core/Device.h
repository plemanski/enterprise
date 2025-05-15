//
// Created by Peter on 5/10/2025.
//

#ifndef DEVICE_H
#define DEVICE_H
#include <cstdint>
#include <wrl/client.h>
#include "Core.h"


#include "directx/d3d12.h"


namespace Enterprise::Core::Graphics {
class DescriptorAllocation;

class ENTERPRISE_API Device {
public:
    [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device() const { return m_D3D12Device; }



private:
    Microsoft::WRL::ComPtr<ID3D12Device2> m_D3D12Device;
};
}

#endif //DEVICE_H
