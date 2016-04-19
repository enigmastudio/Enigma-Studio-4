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

#ifndef PROFILER_HPP
#define PROFILER_HPP

#if defined(eUSE_PROFILER) && defined(eEDITOR)

#define ePROFILER_ADD_THIS_THREAD(name)     eProfiler::addThisThread(name)
#define ePROFILER_BEGIN_THREAD_FRAME()      eProfiler::beginThreadFrame()
#define ePROFILER_END_THREAD_FRAME()        eProfiler::endThreadFrame()
#define ePROFILER_AUTO_ZONE(zone, name)     static eProfilerZone zone(name); eProfilerZoneScope scope(zone);
#define ePROFILER_ZONE(name)                ePROFILER_AUTO_ZONE(eTOKENPASTE(zone_, eTOKENPASTE(__LINE__, __COUNTER__)), name)
#define ePROFILER_FUNC()                    ePROFILER_ZONE(__FUNCTION__)

// profiling statistics of a zone averaged
// over some frames
struct eProfilerZoneStats
{
    static const eU32               MAX_SAMPLES = 50;

    eArray<eF32>                    history;
    eF32                            min;
    eF32                            max;
    eF32                            mean;
    eF32                            stdDev;
};

// represents a profiling zone. to declare a new zone
// in global scope use the ePROFILER_DEFINE macro.
class eProfilerZone
{
public:
    eProfilerZone();
    eProfilerZone(const eString &name);

    void                            enter(eProfilerZone *lastZone);
    void                            leave(eProfilerZone *nextZone);
    void                            clear();
    void                            updateStats();

    const eString &                 getName() const;
    eColor                          getColor() const;
    eU64                            getSelfTicks() const;
    eU64                            getHierTicks() const;
    eF32                            getSelfTimeMs() const;
    eF32                            getHierTimeMs() const;
    eU32                            getCallCount() const;
    const eProfilerZoneStats &      getStatistics() const;

private:
    eString                         m_name;
    eColor                          m_color;
    eU64                            m_selfTotal;
    eU64                            m_hierTotal;
    eU64                            m_selfStart;
    eU64                            m_hierStart;
    eU32                            m_callCount;
    eU32                            m_zoneIndex;
    eProfilerZoneStats              m_stats;
    class eProfilerThread *         m_profThread;
};

// contains all zones of a profiled thread
class eProfilerThread
{
public:
    eProfilerThread(const eString &name, eThreadCtx &ctx);

    void                            beginFrame();
    void                            endFrame();
    eU32                            addZone(eProfilerZone &zone);
    void                            enterZone(eProfilerZone &zone);
    void                            leaveZone();

    const eProfilerZone *           getLastZones(eU32 &numZones) const;
    eU32                            getThreadId() const;
    const eString &                 getThreadName() const;

private:
    static eBool                    _sortZonesBySelfTime(const eProfilerZone &z0, const eProfilerZone &z1);

private:
    static const eU32               MAX_STACK_DEPTH = 512;
    static const eU32               MAX_ZONE_COUNT = 256;

private:
    eProfilerZone *                 m_zoneStack[MAX_STACK_DEPTH];
    eProfilerZone *                 m_zonesByIndex[MAX_ZONE_COUNT];
    eU32                            m_stackIndex;
    eU32                            m_zoneCount;
    eProfilerZone                   m_lastZonesByIndex[MAX_ZONE_COUNT];
    eU32                            m_lastZoneCount;
    const eString                   m_name;
    const eThreadCtx &              m_threadCtx;
};

// profiling system's manager class
class eProfiler
{
    friend eProfilerThread;

public:
    static void                     shutdown();
    static void                     addThread(eThreadCtx &ctx, const eString &name);
    static void                     addThisThread(const eString &name);
    static void                     beginThreadFrame();
    static void                     endThreadFrame();
    
    static eU32                     getThreadCount();
    static const eProfilerZone *    getThreadZones(eU32 index, eU32 &numZones, eF32 &totalFrameMs, const eChar *&threadName);

private:
    static eArray<eProfilerThread*> m_threads;
    static eMutex                   m_mutex;
};

// used to profile a particular code section.
// zone is automatically left when class
// gets out of scope. for convenience use the
// ePROFILE_SCOPE macro.
class eProfilerZoneScope
{
public:
    eFORCEINLINE eProfilerZoneScope(eProfilerZone &zone) :
        m_zone(zone)
    {
        // don't call ePROFILER_SCOPE in functions that
        // are called by static initializers. that might
        // cause the passed zone being yet uninitialized
        // and thus having no valid profiler thread.
        eThreadCtx &ctx = eThread::getThisContext();
        eASSERT(ctx.profThread);
        ctx.profThread->enterZone(zone);
    }

    eFORCEINLINE ~eProfilerZoneScope()
    {
        eThreadCtx &ctx = eThread::getThisContext();
        eASSERT(ctx.profThread);
        ctx.profThread->leaveZone();
    }

private:
    eProfilerZone & m_zone;
};

#else

#define ePROFILER_ADD_THIS_THREAD(name)
#define ePROFILER_BEGIN_THREAD_FRAME()
#define ePROFILER_END_THREAD_FRAME()
#define ePROFILER_ZONE(name)
#define ePROFILER_FUNC()

#endif

#endif // PROFILER_HPP