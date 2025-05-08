//
// Created by Peter on 5/7/2025.
//

#include "EventManager.h"

#include "Log.h"

namespace Enterprise::events {


void EventManager::Shutdown()
{
    m_subscribers.clear();
}

void EventManager::Subscribe(const std::string& eventId, std::unique_ptr<IEventHandlerWrapper>&& handler)
{
    // Find subscribers for the event id
    auto subscribers = m_subscribers.find(eventId);
    if (subscribers != m_subscribers.end())
    {
        auto& handlers = subscribers->second;
        // Check existing subscribers for handler before adding handler
        for (const auto& it : handlers)
        {
            if (it->GetType() == handler->GetType())
            {
                EE_CORE_ERROR("Attempting to double-register subscriber callback");
                return;
            }
        }
        handlers.emplace_back(std::move(handler));
    }
    m_subscribers[eventId].emplace_back(std::move(handler));
}

void EventManager::Unsubscribe(const std::string &eventId, const std::string& handlerTypeName)
{
    auto& handlers = m_subscribers[eventId];
    for (auto it = handlers.begin(); it != handlers.end(); ++it)
    {
        if (it->get()->GetType() == handlerTypeName )
        {
            handlers.erase(it);
            return;
        }
    }
}

void EventManager::TriggerEvent(const Event& event)
{
    for (auto& handler : m_subscribers[event.GetEventType()])
    {
        handler->Exec(event);
    }
}

void EventManager::QueueEvent(std::unique_ptr<Event>&& event)
{
    m_eventsQueue.emplace_back(std::move(event));
}

void EventManager::DispatchEvents()
{
    for (auto eventIt = m_eventsQueue.begin(); eventIt != m_eventsQueue.end(); ++eventIt)
    {
       if (!eventIt ->get()->isHandled)
       {
           TriggerEvent(*eventIt->get());
           eventIt = m_eventsQueue.erase(eventIt);
       } else
       {
           ++eventIt;
       }
    }
}



}
