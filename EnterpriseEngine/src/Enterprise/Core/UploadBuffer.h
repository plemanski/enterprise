//
// Created by Peter on 5/12/2025.
//

#ifndef UPLOADBUFFER_H
#define UPLOADBUFFER_H

#include <wrl/client.h>
#include <directx/d3d12.h>

#include <memory>
#include <deque>

#include "Renderer.h"
#include "MathHelpers.h"

namespace Enterprise::Core::Graphics {

class UploadBuffer {
public:
    struct Allocation {
        void * CPU;
        D3D12_GPU_VIRTUAL_ADDRESS GPU;
    };

    explicit UploadBuffer(size_t pageSize = 2 * 1024 * 1024);

    size_t GetPageSize() const { return m_PageSize; }

    Allocation Allocate(size_t sizeInBytes, size_t alignment);

    void Reset();

private:
    struct Page {
        Page(size_t sizeInBytes);

        ~Page();

        bool HasSpace(size_t sizeInBytes, size_t alignment ) const;

        Allocation Allocate( size_t sizeInBytes, size_t alignment );

        void Reset();

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_D3D12Resource;

        // Base pointer
        void* m_pCPU;
        D3D12_GPU_VIRTUAL_ADDRESS m_pGPU;

        //Allocated Page size
        size_t m_PageSize;

        //Current allocation offset in bytes
        size_t m_Offset;
    };

    using PagePool = std::deque< std::shared_ptr<Page> >;

    std::shared_ptr<Page> RequestPage();

    PagePool m_PagePool;

    PagePool m_AvailablePages;

    std::shared_ptr<Page> m_CurrentPage;

    size_t m_PageSize;


};



#endif //UPLOADBUFFER_H

}