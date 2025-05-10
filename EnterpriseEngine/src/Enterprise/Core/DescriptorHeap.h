//
// Created by Peter on 5/8/2025.
//

#pragma once
#include <d3d12.h>


namespace Enterprise::Core::Renderer {

class DescriptorHeap {
public:
    DescriptorHeap()  {}
    ~DescriptorHeap() { Destroy(); }

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap) {};
    void Destroy() { m_heap = nullptr; }

private:
    ID3D12DescriptorHeap* m_heap;
};

}


