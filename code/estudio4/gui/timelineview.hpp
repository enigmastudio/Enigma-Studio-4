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

#ifndef TIMELINE_VIEW_HPP
#define TIMELINE_VIEW_HPP

#include <QtWidgets/QFrame>

#include "../../eshared/eshared.hpp"

class eTimelineView : public QFrame
{
    Q_OBJECT

public:
    eTimelineView(QWidget *parent);

public Q_SLOTS:
    void                setTime(eF32 time);
    void                toggleLooping();
    void                togglePlayPause();
    void                skipForward();
    void                skipBackward();

public:
    eF32                getTime() const;
    eBool               isPlaying() const;

Q_SIGNALS:
    void                onTimeChanged(eF32 time);

private:
    void                _drawLoopMarker(QPainter *painter);
    void                _drawTimeMarker(QPainter *painter);
    void                _drawTimeline(QPainter *painter);

    void                _clampTime();
    eF32                _posToTime(eInt xPos) const;
    eU32                _timeToPos(eF32 time) const;

private:
    virtual void        mouseMoveEvent(QMouseEvent *me);
    virtual void        mousePressEvent(QMouseEvent *me);
    virtual void        mouseReleaseEvent(QMouseEvent *me);
    virtual void        paintEvent(QPaintEvent *pe);
    virtual void        timerEvent(QTimerEvent *te);

private:
    eF32                m_time;
    eF32                m_playStartTime;
    eF32                m_loopBegin;
    eF32                m_loopEnd;
    eInt                m_timerId;
    eBool               m_playing;
    eBool               m_looping;
    eTimer              m_timer;
};

#endif // TIMELINE_VIEW_HPP