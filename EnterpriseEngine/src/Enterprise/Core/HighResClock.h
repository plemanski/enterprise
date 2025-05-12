//
// Created by Peter on 5/11/2025.
//

#ifndef HIGHRESCLOCK_H
#define HIGHRESCLOCK_H
#include <chrono>


namespace Enterprise::Core {


class HighResClock {
public:
    HighResClock()
        : m_DeltaTime(0)
        , m_TotalTime(0)
    {
        m_InitialTime = std::chrono::high_resolution_clock::now();
    }

    void Tick();

    double GetDeltaSeconds() const;

    void Reset();


private:
    std::chrono::high_resolution_clock::duration m_DeltaTime;
    std::chrono::high_resolution_clock::duration m_TotalTime;
    std::chrono::high_resolution_clock::time_point m_InitialTime;
};


}

#endif //HIGHRESCLOCK_H

