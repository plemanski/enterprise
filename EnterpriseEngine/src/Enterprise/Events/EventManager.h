//
// Created by Peter on 5/7/2025.
//
#pragma once
#include <memory>
#include <string>

#include "EventHandler.h"

namespace Enterprise::events {

class EventManager {
public:
    void Shutdown();

    void Subscribe(const std::string& eventId, std::unique_ptr<IEventHandlerWrapper>&& handler);
    void Unsubscribe(const std::string& eventId, const std::string& handlerTypeName);
    void TriggerEvent(const Event& event);
    void QueueEvent(std::unique_ptr<Event>&& event);
    void DispatchEvents();

private:
    std::vector<std::unique_ptr<Event>> m_eventsQueue;
    std::unordered_map<std::string, std::vector<std::unique_ptr<IEventHandlerWrapper>>> m_subscribers;

};

    extern EventManager gEventManager;

    template<typename EventType>
    inline void Subscribe(const EventHandler<EventType>& callback)
    {
        std::unique_ptr<IEventHandlerWrapper> handler = std::make_unique<EventHandlerWrapper<EventType>>(callback);
        gEventManager.Subscribe(EventType::GetStaticEventType(), std::move(handler));
    }

    template<typename EventType>
    inline void Unsubscribe(const EventHandler<EventType>& callback)
    {
        const std::string handlerTypeName = callback.target_type().name();
    }

    inline void TriggerEvent(const Event& triggeredEvent)
    {
        gEventManager.TriggerEvent(triggeredEvent);
    }

    inline void QueueEvent(std::unique_ptr<Event>&& event)
    {
       gEventManager.QueueEvent(std::forward<std::unique_ptr<Event>>(event));
    }

}
