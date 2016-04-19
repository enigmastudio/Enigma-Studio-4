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

eThread::eThread(eInt flags, eThreadFunc threadFunc) :
    m_prio((eThreadPriority)(flags&(~eTHCF_SUSPENDED))),
    m_threadFunc(threadFunc)
{
    const eU32 tf = (flags&eTHCF_SUSPENDED ? CREATE_SUSPENDED : 0);
    m_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_threadTrunk, this, tf, (LPDWORD)&m_tid);
    eASSERT(m_handle);

#ifdef eEDITOR
    m_ctx.thread = this;
    m_ctx.tid = m_tid;
#ifdef eUSE_PROFILER
    m_ctx.profThread = nullptr;
#endif
#endif

    setPriority(m_prio);
}

eThread::~eThread()
{
    join();
}

void eThread::sleep(eU32 ms)
{
    Sleep(ms);
}

void eThread::join()
{
    WaitForSingleObject((HANDLE)m_handle, INFINITE);
    CloseHandle((HANDLE)m_handle);
    m_handle = nullptr;
}

void eThread::yield()
{
    sleep(1);
}

void eThread::resume()
{
    ResumeThread(m_handle);
}

void eThread::suspend()
{
    SuspendThread(m_handle);
}

void eThread::terminate(eU32 exitCode)
{
    TerminateThread(m_handle, exitCode);
}

void eThread::setPriority(eThreadPriority prio)
{
    struct ClassPriority
    {
        eU32 cls;
        eU32 prio;
    }
    cp[] =
    {
        {BELOW_NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_LOWEST},        // low
        {NORMAL_PRIORITY_CLASS,       THREAD_PRIORITY_NORMAL},        // normal
        {HIGH_PRIORITY_CLASS,         THREAD_PRIORITY_TIME_CRITICAL}, // high
    };

    SetPriorityClass(m_handle, cp[prio].cls);
    SetThreadPriority(m_handle, cp[prio].prio);
    m_prio = prio;
}

eU32 eThread::getId() const
{
    return m_tid;
}

eThreadPriority eThread::getPriority() const
{
    return m_prio;
}

#ifdef eEDITOR
static eThreadCtx g_mainThreadCtx;
static eTHREADLOCAL eThreadCtx *g_curThreadCtx = nullptr;

eThreadCtx & eThread::getThisContext()
{
    return (g_curThreadCtx ? *g_curThreadCtx : g_mainThreadCtx);
}

eThreadCtx & eThread::getContext()
{
    return m_ctx;
}

const eThreadCtx & eThread::getContext() const
{
    return m_ctx;
}
#endif

eU32 eThread::operator () ()
{
    eASSERT(m_threadFunc);
    return m_threadFunc(this);
}

eU32 eThread::_threadTrunk(ePtr arg)
{
    eThread *thread = (eThread *)arg;
#ifdef eEDITOR
    g_curThreadCtx = &thread->m_ctx;
#endif
    return (*thread)();
}

eMutex::eMutex() :
    m_locked(eFALSE)
{
    CRITICAL_SECTION *cs = new CRITICAL_SECTION;
    InitializeCriticalSection(cs);
    m_handle = (ePtr)cs;
}

eMutex::~eMutex()
{
    eASSERT(!m_locked);
    CRITICAL_SECTION *cs = (CRITICAL_SECTION *)m_handle;
    DeleteCriticalSection(cs);
    eDelete(cs);
}

void eMutex::enter()
{
    EnterCriticalSection((CRITICAL_SECTION *)m_handle);
    eASSERT(!m_locked);
    m_locked = eTRUE;
}

void eMutex::tryEnter()
{
    if (TryEnterCriticalSection((CRITICAL_SECTION *)m_handle))
        m_locked = eTRUE;
}

void eMutex::leave()
{
    eASSERT(m_locked);
    m_locked = eFALSE;
    LeaveCriticalSection((CRITICAL_SECTION *)m_handle);
}

eBool eMutex::isLocked() const
{
    return m_locked;
}