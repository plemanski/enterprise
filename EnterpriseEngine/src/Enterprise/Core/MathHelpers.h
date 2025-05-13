//
// Created by Peter on 5/12/2025.
//

#ifndef MATHHELPERS_H
#define MATHHELPERS_H


namespace Enterprise::Core::Graphics {

//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------
// Source DirectX TK12

// Helper to check for power-of-2
template<typename T>
constexpr bool IsPowerOf2(T x) noexcept { return ((x != 0) && !(x & (x - 1))); }

// Helpers for aligning values by a power of 2
template<typename T>
inline T AlignDown(T size, size_t alignment) noexcept
{
    if (alignment > 0)
    {
        assert(((alignment - 1) & alignment) == 0);
        auto mask = static_cast<T>(alignment - 1);
        return size & ~mask;
    }
    return size;
}

template<typename T>
inline T AlignUp(T size, size_t alignment) noexcept
{
    if (alignment > 0)
    {
        assert(((alignment - 1) & alignment) == 0);
        auto mask = static_cast<T>(alignment - 1);
        return (size + mask) & ~mask;
    }
    return size;
}

}
#endif //MATHHELPERS_H
