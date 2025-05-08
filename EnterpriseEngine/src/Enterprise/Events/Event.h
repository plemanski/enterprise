//
// Created by Peter on 5/6/2025.
//
#pragma once
#include <string>

namespace Enterprise::events {

class Event {

public:
    virtual const std::string GetEventType() const = 0;
    bool isHandled { false };

};

}