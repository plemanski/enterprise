//
// Created by Peter on 5/6/2025.
//
#pragma once
#include <string>

namespace Enterprise::events {

enum class EventType {
    None = 0,
    WindowClose, WindowResize, WindowLostFocus, WindowMoved,
    AppTick, AppUpdate, AppRender,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};

#define EVENT_TYPE(type)                                                        \
    static EventType GetStaticEventType() {return EventType::type;}                  \
    virtual EventType GetEventType() const override { return GetStaticEventType(); } \
    virtual const char* GetName() const override { return #type; }

class Event {

public:
    virtual ~Event() = default;

    virtual const char* GetName() const = 0;
    virtual EventType GetEventType() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool isHandled { false };


};

}