//
// Created by Peter on 5/9/2025.
//

#ifndef APPLICATIONEVENT_H
#define APPLICATIONEVENT_H
#include "Event.h"

namespace Enterprise::events {

class AppRenderEvent : public Event {
public:
    AppRenderEvent(double fDeltaTime, double fTotalTime)
        : ElapsedTime( fDeltaTime )
        , TotalTime( fTotalTime )
    {};
    EVENT_TYPE("AppRender");

    double ElapsedTime;
    double TotalTime;
};

class AppUpdateEvent : public Event {
public:
    AppUpdateEvent(double fDeltaTime, double fTotalTime)
        : ElapsedTime( fDeltaTime )
        , TotalTime( fTotalTime )
    {};
    EVENT_TYPE("AppUpdate");

    double ElapsedTime;
    double TotalTime;

};

class AppWindowResizeEvent: public Event {
public:
    inline AppWindowResizeEvent(uint32_t width_, uint32_t height_)
        : Width( width_ )
        , Height( height_ )
    {}
    EVENT_TYPE("AppWindowResizeEvent");

    uint32_t Width;
    uint32_t Height;
};

}

#endif //APPLICATIONEVENT_H
