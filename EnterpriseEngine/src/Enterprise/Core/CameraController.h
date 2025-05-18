//
// Created by Peter on 5/16/2025.
//

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include "Camera.h"

namespace Enterprise::Core {

class CameraController {
public:
    CameraController();
    ~CameraController();

private:
    void UpdateCamera();

private:
    Camera m_Camera;

};
}


#endif //CAMERACONTROLLER_H
