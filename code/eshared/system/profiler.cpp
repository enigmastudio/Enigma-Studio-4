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

#include "system.hpp"

#if defined(eUSE_PROFILER) && defined(eEDITOR)

eProfilerZone::eProfilerZone()
{
}

eProfilerZone::eProfilerZone(const eString &name) :
    m_name(name),
    m_selfTotal(0),
    m_hierTotal(0),
    m_selfStart(0),
    m_hierStart(0),
    m_callCount(0)
{
    eThreadCtx &threadCtx = eThread::getThisContext();
    m_profThread = threadCtx.profThread;
    eASSERT(m_profThread);
    m_zoneIndex = m_profThread->addZone(*this);
    eRandomize(eHashStr(name));
    m_color.fromHsv(eRandom(0, 359), eRandom(90, 150), eRandom(210, 255));
}

void eProfilerZone::enter(eProfilerZone *lastZone)
{
    const eU64 enterTime = eTimer::getTickCount();
    m_selfStart = enterTime;
    m_hierStart = enterTime;
    m_callCount++;

    if (lastZone)
        lastZone->m_selfTotal += m_selfStart-lastZone->m_selfStart;
}

void eProfilerZone::leave(eProfilerZone *nextZone)
{
    const eU64 endTime = eTimer::getTickCount();
    m_selfTotal += (endTime-m_selfStart);
    m_hierTotal += (endTime-m_hierStart);

    if (nextZone)
        nextZone->m_selfStart = endTime;
}

void eProfilerZone::clear()
{
    m_callCount = 0;
    m_selfTotal = 0;
    m_hierTotal = 0;
}

void eProfilerZone::updateStats()
{
    eArray<eF32> &hist = m_stats.history;

    hist.append(getSelfTimeMs());
    while (hist.size() >= eProfilerZoneStats::MAX_SAMPLES)
        hist.removeAt(0);

    m_stats.max = hist[0];
    m_stats.min = hist[0];
    m_stats.mean = hist[0];

    for (eU32 k=1; k<hist.size(); k++)
    {
        const eF32 v = hist[k];
        m_stats.max = eMax(m_stats.max, v);
        m_stats.min = eMin(m_stats.min, v);
        m_stats.mean += v;
    }

    eASSERT(!hist.isEmpty());
    m_stats.mean /= (eF32)hist.size();

    eF32 var = 0.0f;
    for (eU32 k=1; k<hist.size(); k++)
        var += eSqr(hist[k]-m_stats.mean);

    m_stats.stdDev = (hist.size() > 1 ? eSqrt(var/(eF32)(hist.size()-1)) : 0.0f);
}

const eString & eProfilerZone::getName() const
{
    return m_name;
}

eColor eProfilerZone::getColor() const
{
    return m_color;
}

eU64 eProfilerZone::getSelfTicks() const
{
    return m_selfTotal;
}

eU64 eProfilerZone::getHierTicks() const
{
    return m_hierTotal;
}

eF32 eProfilerZone::getSelfTimeMs() const
{
    return (eF32)((eF64)m_selfTotal/(eF64)eTimer::getFrequency()*1000.0);
}

eF32 eProfilerZone::getHierTimeMs() const
{
    return (eF32)((eF64)m_hierTotal/(eF64)eTimer::getFrequency()*1000.0);
}

eU32 eProfilerZone::getCallCount() const
{
    return m_callCount;
}

const eProfilerZoneStats & eProfilerZone::getStatistics() const
{
    return m_stats;
}

eProfilerThread::eProfilerThread(const eString &name, eThreadCtx &ctx) :
    m_name(name),
    m_stackIndex(0),
    m_zoneCount(0),
    m_lastZoneCount(0),
    m_threadCtx(ctx)
{
    ctx.profThread = this;
    eMemSet(m_zoneStack, 0, sizeof(m_zoneStack));
    eMemSet(m_zonesByIndex, 0, sizeof(m_zonesByIndex));
}

eU32 eProfilerThread::addZone(eProfilerZone &zone)
{
    m_zonesByIndex[m_zoneCount++] = &zone;
    return m_zoneCount-1;
}

void eProfilerThread::enterZone(eProfilerZone &zone)
{
    eProfilerZone *lastZone = m_zoneStack[m_stackIndex];
    m_zoneStack[++m_stackIndex] = &zone;
    zone.enter(lastZone);
}

void eProfilerThread::leaveZone()
{
    eProfilerZone *zone = m_zoneStack[m_stackIndex--];
    eProfilerZone *nextZone = m_zoneStack[m_stackIndex];
    zone->leave(nextZone);
}

void eProfilerThread::beginFrame()
{
    for (eU32 j=0; j<m_zoneCount; j++)
        m_zonesByIndex[j]->clear();
}

void eProfilerThread::endFrame()
{
    eScopedLock lock(eProfiler::m_mutex);
    
    // update statistics
    for (eU32 i=0; i<m_zoneCount; i++)
        m_zonesByIndex[i]->updateStats();

    // backup zones to provide access of
    // profiling data of the last frame
    for (eU32 i=0; i<m_zoneCount; i++)
        m_lastZonesByIndex[i] = *m_zonesByIndex[i];

    m_lastZoneCount = m_zoneCount;
    eSort(m_lastZonesByIndex, m_lastZoneCount, _sortZonesBySelfTime);
}

const eProfilerZone * eProfilerThread::getLastZones(eU32 &numZones) const
{
    numZones = m_lastZoneCount;
    return m_lastZonesByIndex;
}

eU32 eProfilerThread::getThreadId() const
{
    return m_threadCtx.tid;
}

const eString & eProfilerThread::getThreadName() const
{
    return m_name;
}

eBool eProfilerThread::_sortZonesBySelfTime(const eProfilerZone &z0, const eProfilerZone &z1)
{
    return (z0.getSelfTimeMs() < z1.getSelfTimeMs());
}

eArray<eProfilerThread *> eProfiler::m_threads;
eMutex                    eProfiler::m_mutex;

void eProfiler::shutdown()
{
    eScopedLock lock(m_mutex);

    for (eU32 i=0; i<m_threads.size(); i++)
        eDelete(m_threads[i]);

    m_threads.clear();
}

void eProfiler::addThisThread(const eString &name)
{
    addThread(eThread::getThisContext(), name);
}

void eProfiler::addThread(eThreadCtx &ctx, const eString &name)
{
    eScopedLock lock(m_mutex);

    for (eU32 i=0; i<m_threads.size(); i++)
        if (m_threads[i]->getThreadId() == ctx.tid)
            return;

    m_threads.append(new eProfilerThread(name, ctx));
}

void eProfiler::beginThreadFrame()
{
    eThreadCtx &ctx = eThread::getThisContext();
    eASSERT(ctx.profThread);
    ctx.profThread->beginFrame();
}

void eProfiler::endThreadFrame()
{
    eThreadCtx &ctx = eThread::getThisContext();
    eASSERT(ctx.profThread);
    ctx.profThread->endFrame();
}

eU32 eProfiler::getThreadCount()
{
    return m_threads.size();
}

const eProfilerZone * eProfiler::getThreadZones(eU32 index, eU32 &numZones, eF32 &totalFrameMs, const eChar *&threadName)
{
    eASSERT(index < m_threads.size());

    eScopedLock lock(m_mutex);
    const eProfilerThread *pt = m_threads[index];
    const eProfilerZone *zones = pt->getLastZones(numZones);
    totalFrameMs = 0.0f;
    threadName = pt->getThreadName();
    
    for (eU32 i=0; i<numZones; i++)
        totalFrameMs += zones[i].getSelfTimeMs();

    totalFrameMs = eMax(0.1f, totalFrameMs); // minimum 1 micro-second to avoid division-by-zero
    return zones;
}

#endif