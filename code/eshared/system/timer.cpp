/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       ______        _                             __ __
 *      / ____/____   (_)____ _ ____ ___   ____ _   / // /
 *     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
 *    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
 *   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
 *                    /____/                              
 *
 *   Copyright © 2003-2012 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "system.hpp"

eU64  eTimer::m_freq   = 0;
eBool eTimer::m_inited = eFALSE;

eTimer::eTimer() :
    m_histIndex(0),
    m_histCount(0)
{
    _initialize();
    QueryPerformanceCounter((LARGE_INTEGER *)&m_startTime);
}

eF32 eTimer::restart()
{
    const eF32 elapsedMs = getElapsedMs();
    m_elapsedHist[m_histIndex] = elapsedMs;
    m_histIndex = (m_histIndex+1)%eELEMENT_COUNT(m_elapsedHist);
    m_histCount = eMin(m_histCount+1, eELEMENT_COUNT(m_elapsedHist));

    QueryPerformanceCounter((LARGE_INTEGER *)&m_startTime);
    return elapsedMs;
}

eF32 eTimer::getElapsedMs() const
{
    eU64 curTime;
    QueryPerformanceCounter((LARGE_INTEGER *)&curTime);
    return (eF32)((eF64)(curTime-m_startTime)/(eF64)m_freq*1000.0);
}

eF32 eTimer::getAvgElapsedMs() const
{
    eF32 avg = 0.0f;

    if (m_histCount > 0)
    {
        for (eU32 i=0; i<m_histCount; i++)
            avg += m_elapsedHist[i];

        avg /= (eF32)m_histCount;
    }

    return eMax(avg, 0.1f); // minimum 1 micro-second to avoid division-by-zero
}

eU32 eTimer::getTimeMs()
{
    return eFtoL((eF32)((eF64)getTickCount()/(eF64)m_freq*1000.0));
}

eU64 eTimer::getTickCount()
{
    eU64 curTime;
    QueryPerformanceCounter((LARGE_INTEGER *)&curTime);
    return curTime;
}

eU64 eTimer::getFrequency()
{
    _initialize(); // make sure the frequency is valid
    return m_freq;
}

void eTimer::_initialize()
{
    if (!m_inited)
    {
        const BOOL res = QueryPerformanceFrequency((LARGE_INTEGER *)&m_freq);
        eASSERT(res);
        m_inited = eTRUE;
    }
}