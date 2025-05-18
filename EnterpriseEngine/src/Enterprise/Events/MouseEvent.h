//
// Created by Peter on 5/16/2025.
//

#ifndef MOUSEEVENT_H
#define MOUSEEVENT_H
#include "event.h"

namespace Enterprise::events {
//---------------------------------------------------------|
//MK_CONTROL                                               |
//0x0008            The CTRL key is down                   |
//---------------------------------------------------------|
//MK_LBUTTON                                               |
//0x0001            The left The left mouse button is down |
//---------------------------------------------------------|
//MK_MBUTTON                                               |
//0x0010            The middle mouse button is down        |
//.--------------------------------------------------------|
//MK_RBUTTON                                               |
//0x0002            The right mouse button is down         |
//---------------------------------------------------------|
//MK_SHIFT                                                 |
//0x0004            The SHIFT key is down                  |
//---------------------------------------------------------|
//MK_XBUTTON1                                              |
//0x0020            The XBUTTON1 is down                   |
//---------------------------------------------------------|
//MK_XBUTTON2                                              |
//0x0040            The XBUTTON2 is down                   |
//-------------------------------------------------------- |
struct MouseCoords {
    SHORT x;
    SHORT y;
};

struct MouseButton {
    bool LMB;
    bool RMB;
    bool MMB;
    bool CTRL;
    bool SHIFT;
    bool XBUTTON_1;
    bool XBOTTON_2;
};


class MouseEvent : public Event {
public:
    MouseEvent( MouseButton buttons, MouseCoords coords )
        : m_Buttons(buttons)
          , m_Coords(coords)
    {
    };

    MouseEvent( MouseCoords coords )
        : m_Coords(coords)
          , m_Buttons()
    {
    };

    EVENT_TYPE("MouseEvent");

private:
    MouseCoords m_Coords;
    MouseButton m_Buttons;
};
};

#endif //MOUSEEVENT_H
