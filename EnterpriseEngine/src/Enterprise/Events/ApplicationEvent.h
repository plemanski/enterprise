//
// Created by Peter on 5/9/2025.
//

#ifndef WINDOWEVENT_H
#define WINDOWEVENT_H
#include "Event.h"

namespace Enterprise::events {

class AppRenderEvent : public Event {
public:
    AppRenderEvent() = default;
    EVENT_TYPE(AppRender);
};

class AppUpdateEvent : public Event {
public:
    AppUpdateEvent() = default;
    EVENT_TYPE(AppUpdate);
};

}

#endif //WINDOWEVENT_H
