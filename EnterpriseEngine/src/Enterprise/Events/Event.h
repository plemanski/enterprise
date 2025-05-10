//
// Created by Peter on 5/6/2025.
//
#pragma once
#include <string>

#include "Core/crc32.h"

namespace Enterprise::events {


#define EVENT_TYPE(type)                                                        \
    static std::uint32_t GetStaticEventType() {return crc32_fast(type, sizeof(type), 0) ;}                  \
    virtual std::uint32_t GetEventType() const override { return GetStaticEventType(); } \
    virtual const char* GetName() const override { return type; }


class Event {

public:
    virtual ~Event() = default;
    virtual const char* GetName() const = 0;
    virtual std::uint32_t GetEventType() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool isHandled { false };


};

}