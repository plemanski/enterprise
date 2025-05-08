//
// Created by Peter on 5/6/2025.
//
#pragma once
#include <functional>

#include "event.h"

namespace Enterprise::events {

    template<typename EventType>
    using EventHandler = std::function<void(const EventType& event)>;

    class IEventHandlerWrapper {
    public:
        virtual ~IEventHandlerWrapper() = default;

        void Exec(const Event& e)
        {
            Call(e);
        }

        virtual std::string GetType() const = 0;

    private:
        virtual void Call(const Event& e) = 0;
    };

    template<typename EventType>
    class EventHandlerWrapper : public IEventHandlerWrapper {
    public:
        explicit EventHandlerWrapper(const EventHandler<EventType>& handler)
        : m_handler(handler)
        , m_handlerType(m_handler.target_type().name()) {};

        // Refactor to use hashing and uint32
        std::string GetType() const override { return m_handlerType; }
    private:
        void Call(const Event& event) override
        {
            if (event.GetEventType() == EventType::GetStaticEventType())
            {
                m_handler(static_cast<const EventType&>(event));
            }
        }


        EventHandler<EventType> m_handler;
        const std::string m_handlerType;
    };

}