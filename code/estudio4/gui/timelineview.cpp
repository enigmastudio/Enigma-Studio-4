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

#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include "timelineview.hpp"

eTimelineView::eTimelineView(QWidget *parent) : QFrame(parent),
    m_time(0.0f),
    m_playStartTime(0.0f),
    m_loopBegin(0.0f),
    m_loopEnd(0.0f),
    m_timerId(-1),
    m_playing(eFALSE),
    m_looping(eFALSE)
{
}

void eTimelineView::setTime(eF32 time)
{
    if (!eAreFloatsEqual(m_time, time))
    {
        m_time = time;
        _clampTime();
        m_playStartTime = m_time;
        m_timer.restart();

		if (m_playing)
		{
			eTfPlayerStop(*eSfx);
			eTfPlayerStart(*eSfx, m_time);
		}

        repaint();
    }
}

void eTimelineView::toggleLooping()
{
    m_looping = !m_looping;
    repaint();
}

void eTimelineView::togglePlayPause()
{
    if (m_playing)
    {
        killTimer(m_timerId);
        m_playing = eFALSE;
        eTfPlayerStop(*eSfx);
    }
    else
    {
        m_playing = eTRUE;
        m_timer.restart();
        m_playStartTime = m_time;
        m_timerId = startTimer(20);
        eTfPlayerStart(*eSfx, m_time);
    }
}

void eTimelineView::skipForward()
{
    setTime(eDemo::MAX_RUNNING_TIME_MINS*60.0f);
    Q_EMIT onTimeChanged(m_time);
}

void eTimelineView::skipBackward()
{
    setTime(0.0f);
    Q_EMIT onTimeChanged(m_time);
}

eBool eTimelineView::isPlaying() const
{
    return m_playing;
}

eF32 eTimelineView::getTime() const
{
    return m_time;
}

void eTimelineView::_drawLoopMarker(QPainter *painter)
{
    const QColor &col = palette().dark().color();

    painter->save();
    painter->setPen(QPen(Qt::NoPen));
    painter->setBrush(QBrush(col, m_looping ? Qt::SolidPattern : Qt::Dense6Pattern));
    painter->drawRect(QRect(_timeToPos(m_loopBegin), 10, _timeToPos(m_loopEnd-m_loopBegin), geometry().height()));
    painter->restore();
}

void eTimelineView::_drawTimeMarker(QPainter *painter)
{
    const eF32 xPos = _timeToPos(m_time);

    painter->save();
    painter->setPen(palette().highlight().color());
    painter->drawLine(QLineF(xPos, 4.0f, xPos, geometry().height()));
    painter->restore();
}

void eTimelineView::_drawTimeline(QPainter *painter)
{
    painter->save();

    // horizontal step between two lines
    const eF32 step = (eF32)geometry().width()/eDemo::MAX_RUNNING_TIME_MINS/60.0f;
    eF32 lastX = 0;

    for (eU32 i=1; i<(eU32)eDemo::MAX_RUNNING_TIME_MINS*60; i++)
    {
        QPointF p(i*step, 3.0f);

        if (i%60 == 0) // minutes
        {
            const QString time = QString("0%1:00").arg(i/60);
            painter->drawText((eU32)p.x()-fontMetrics().width(time)/2, 9, time);
            p.setY(10.0f);
        }
        else if (i%30 == 0) // half minutes
            p.setY(12.0f);
        else
            p.setY(16.0f); // seconds
        
        // paint seconds lines only if they are not to close
        if (p.x() > lastX+2)
        {
            painter->drawLine(p, QPointF(p.x(), geometry().height()));
            lastX = p.x();
        }
    }

    painter->restore();
}

void eTimelineView::_clampTime()
{
    const eF32 minTime = m_looping ? m_loopBegin : 0.0f;
    const eF32 maxTime = m_looping ? m_loopEnd : eDemo::MAX_RUNNING_TIME_MINS*60.0f;
    m_time = eClamp(minTime, m_time, maxTime);
}

eF32 eTimelineView::_posToTime(eInt xPos) const
{
    return (eF32)xPos/(eF32)geometry().width()*eDemo::MAX_RUNNING_TIME_MINS*60.0f;
}

eU32 eTimelineView::_timeToPos(eF32 time) const
{
    return eMin(eTrunc((eF32)geometry().width()/eDemo::MAX_RUNNING_TIME_MINS/60.0f*time), size().width()-1);
}

void eTimelineView::mouseMoveEvent(QMouseEvent *me)
{
    QFrame::mouseMoveEvent(me);

    if (me->buttons()&Qt::RightButton) // create loop interval
    {
        m_loopEnd = eMax(0.0f, _posToTime(me->pos().x()));
        repaint();
    }
    else if (me->buttons()&Qt::LeftButton || me->buttons()&Qt::MiddleButton) // drag time
    {
        setTime(_posToTime(me->pos().x()));
        Q_EMIT onTimeChanged(m_time);
    }
}

void eTimelineView::mousePressEvent(QMouseEvent *me)
{
    QFrame::mousePressEvent(me);

    if (me->buttons()&Qt::RightButton)
    {
        m_loopBegin = eMax(0.0f, _posToTime(me->pos().x()));
        m_loopEnd = m_loopBegin;
    }

    mouseMoveEvent(me);
}

void eTimelineView::mouseReleaseEvent(QMouseEvent *me)
{
    QFrame::mouseReleaseEvent(me);

    if (m_loopBegin > m_loopEnd)
        eSwap(m_loopBegin, m_loopEnd);
}

void eTimelineView::paintEvent(QPaintEvent *pe)
{
    QFrame::paintEvent(pe);

    QPainter p(this);
    _drawLoopMarker(&p);
    _drawTimeline(&p);
    _drawTimeMarker(&p);
}

void eTimelineView::timerEvent(QTimerEvent *te)
{
    QFrame::timerEvent(te);
    
    eF32 newTime = m_playStartTime+m_timer.getElapsedMs()*0.001f;

    // looping enabled?
    if (m_looping && !eIsFloatZero(m_loopEnd-m_loopBegin))
    {
        if (newTime < m_loopBegin || newTime > m_loopEnd)
        {
            newTime = m_loopBegin;
            m_playStartTime = newTime;
            m_timer.restart();
        }
    }
    else if (newTime > eDemo::MAX_RUNNING_TIME_MINS*60.0f) // wrap around when reached end
    {
        newTime = 0.0f;
        m_playStartTime = 0.0f;
        m_timer.restart();
    }

    m_time = newTime;
    _clampTime();
    repaint();

    Q_EMIT onTimeChanged(m_time);
}