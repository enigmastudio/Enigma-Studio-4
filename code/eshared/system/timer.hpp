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

#ifndef TIMER_HPP
#define TIMER_HPP

class eTimer
{
public:
    eTimer();

    eF32            restart();
    eF32            getElapsedMs() const;
    eF32            getAvgElapsedMs() const;

public:
    static eU32     getTimeMs();
    static eU64     getTickCount();
    static eU64     getFrequency();

private:
    static void     _initialize();

private:
    static eU64     m_freq;
    static eBool    m_inited;

private:
    eU64            m_startTime;
    eF32            m_elapsedHist[60];
    eU32            m_histIndex;
    eU32            m_histCount;
};

#endif // TIMER_HPP