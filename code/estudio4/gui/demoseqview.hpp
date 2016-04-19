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

#ifndef DEMO_SEQ_VIEW_HPP
#define DEMO_SEQ_VIEW_HPP

#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QMenu>

#include "../../eshared/eshared.hpp"

// item in the sequencer view
class eDemoSeqItem : public QGraphicsItem
{
public:
    eDemoSeqItem(eISequencerOp *op, eF32 scale);

    void                setScale(eF32 scale);
    void                updatePosition();

    eISequencerOp *     getOperator() const;
    eF32                getStartTime() const;
    eF32                getEndTime() const;
    eF32                getDuration() const;
    eU32                getTrack() const;

    virtual QRectF      boundingRect() const;

private:
    virtual void        paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual void        mouseReleaseEvent(QGraphicsSceneMouseEvent *me);
    virtual void        mousePressEvent(QGraphicsSceneMouseEvent *me);
    virtual void        mouseMoveEvent(QGraphicsSceneMouseEvent *me);
    virtual QVariant    itemChange(GraphicsItemChange change, const QVariant &value);

public:
    static const eInt   HEIGHT = 22;
    static const eInt   RESIZE_AREA = 9;

private:
    eISequencerOp *     m_op;
    eF32                m_scale;
    eBool               m_resizing;

    eInt &              m_track;
    eF32 &              m_duration;
    eF32 &              m_startTime;
};

class eDemoSeqView : public QGraphicsView
{
    Q_OBJECT
    Q_PROPERTY(QColor colGrid READ getGridColor WRITE setGridColor);

public:
    eDemoSeqView(QWidget *parent);
    virtual ~eDemoSeqView();

    void                clear();

    void                setGridColor(const QColor &gridCol);
    void                setDemoOp(eIDemoOp *demoOp);
    void                setTime(eF32 time);

    const QColor &      getGridColor() const;
    eIDemoOp *          getDemoOp() const;
    eF32                getTime() const;

public Q_SLOTS:
    void                onOperatorChanged();

Q_SIGNALS:
    void                onFinishedEditing();
    void                onTimeChanged(eF32 time);
    void                onOperatorSelected(eIOperator *op);

private:
    virtual void        showEvent(QShowEvent *se);
    virtual void        mouseMoveEvent(QMouseEvent *me);
    virtual void        mousePressEvent(QMouseEvent *me);
    virtual void        mouseReleaseEvent(QMouseEvent *me);
    virtual void        contextMenuEvent(QContextMenuEvent *ce);

    virtual void        drawBackground(QPainter *painter, const QRectF &rect);
    virtual void        drawForeground(QPainter *painter, const QRectF &rect);

private Q_SLOTS:
    void                _onShowGaps(bool checked);

private:
    void                _createItems();
    void                _drawGaps(QPainter *painter, const QRectF &rect);
    void                _drawOverlaps(QPainter *painter);
    void                _updateTimeMarkerItem();
    void                _updateSceneRect();
    void                _setScale(eF32 scale);
    eF32                _timeToPosition(eF32 time) const;
    eF32                _positionToTime(eF32 pos) const;

private:
    typedef QList<eDemoSeqItem *> eDemoSeqItemPtrList;

private:
    QGraphicsLineItem * m_timeMarkerItem;
    eIDemoOp *          m_demoOp;
    eF32                m_time;
    eF32                m_scale;
    eDemoSeqItemPtrList m_seqItems;
    QPoint              m_lastMousePos;
    QPoint              m_mouseDownPos;
    eBool               m_scrolling;
    eBool               m_showGaps;
    QMenu               m_menu;
    QColor              m_gridCol;
};

#endif // DEMO_SEQ_VIEW_HPP