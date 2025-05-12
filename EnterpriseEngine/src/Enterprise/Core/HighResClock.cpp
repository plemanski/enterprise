//
// Created by Peter on 5/11/2025.
//

#include "HighResClock.h"


namespace Enterprise::Core {

using namespace std::chrono;

void HighResClock::Tick()
{
    auto t1 = high_resolution_clock::now();
    m_DeltaTime = t1 - m_InitialTime;
    m_TotalTime += m_DeltaTime;
    m_InitialTime = t1;
}

void HighResClock::Reset()
{
    m_InitialTime = high_resolution_clock::now();
    m_DeltaTime = high_resolution_clock::duration();
    m_TotalTime = high_resolution_clock::duration();

}

double HighResClock::GetDeltaSeconds() const
{
    return m_DeltaTime.count() * 1e-9;
}


}
