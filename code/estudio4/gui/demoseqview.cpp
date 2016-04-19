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

#include <QtWidgets/QStyleOption>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QDrawUtil.h>

#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPalette>
#include <QtGui/QPainter>

#include "demoseqview.hpp"

eDemoSeqItem::eDemoSeqItem(eISequencerOp *op, eF32 scale) : QGraphicsItem(nullptr),
    m_op(op),
    m_scale(scale),
    m_resizing(eFALSE),
    m_track(m_op->getParameter(2).getBaseValue().integer),
    m_duration(m_op->getParameter(1).getBaseValue().flt),
    m_startTime(m_op->getParameter(0).getBaseValue().flt)
{
    eASSERT(scale >= 1.0f);
    setFlags(ItemIsFocusable|ItemIsMovable|ItemIsSelectable|ItemClipsToShape|ItemSendsGeometryChanges);
    updatePosition();
}

void eDemoSeqItem::setScale(eF32 scale)
{
    eASSERT(scale >= 1.0f);
    m_scale = scale;
    updatePosition();
}

void eDemoSeqItem::updatePosition()
{
    setPos(m_startTime*m_scale, (eF32)(HEIGHT+m_track*HEIGHT+1));
}

eISequencerOp * eDemoSeqItem::getOperator() const
{
    return m_op;
}

eF32 eDemoSeqItem::getStartTime() const
{
    return m_startTime;
}

eF32 eDemoSeqItem::getEndTime() const
{
    return getStartTime()+getDuration();
}

eF32 eDemoSeqItem::getDuration() const
{
    return m_duration;
}

eU32 eDemoSeqItem::getTrack() const
{
    return (eU32)m_track;
}

QRectF eDemoSeqItem::boundingRect() const
{
    return QRectF(0, 0, getDuration()*m_scale, (eF32)HEIGHT-1);
}

void eDemoSeqItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // draw item
    const QString caption = (m_op->getUserName() == "" ? QString(m_op->getMetaInfos().name) : QString('"')+m_op->getUserName()+'"');
    qDrawShadePanel(painter, boundingRect().toRect(), qApp->palette(), isSelected(), 1, &qApp->palette().button());

    // draw resizing area
    painter->setPen(qApp->palette().dark().color());

    for (eInt i=boundingRect().right()-3; i>=boundingRect().right()-RESIZE_AREA; i-=2)
        for (eInt j=2; j<boundingRect().height()-2; j+=2)
            painter->drawPoint(i, j);

    // draw text
    painter->setPen(qApp->palette().windowText().color());
    painter->drawText(boundingRect().adjusted(1, 1, -RESIZE_AREA, -1), Qt::AlignCenter, caption);
}

void eDemoSeqItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *me)
{
    QGraphicsItem::mouseReleaseEvent(me);
    m_resizing = eFALSE;
}

void eDemoSeqItem::mousePressEvent(QGraphicsSceneMouseEvent *me)
{
    QGraphicsItem::mousePressEvent(me);

    if (me->button() & Qt::LeftButton)
        if (me->pos().x() >= boundingRect().width()-RESIZE_AREA)
            m_resizing = eTRUE;
}

void eDemoSeqItem::mouseMoveEvent(QGraphicsSceneMouseEvent *me)
{
    if (m_resizing)
    {
        const eF32 change = (me->pos().x()-me->lastPos().x())/m_scale;
        m_duration = eMax(0.0f, m_duration+change);
        
        m_op->getParameter(1).setChanged();
        m_op->setChanged(eFALSE, eTRUE);
        prepareGeometryChange();
        scene()->invalidate(QRectF(), QGraphicsScene::ForegroundLayer);
    }
    else
        QGraphicsItem::mouseMoveEvent(me);
}

QVariant eDemoSeqItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange)
    {
        QPointF newPos = value.toPointF();
        if (newPos.x() < 0.0f)
            newPos.setX(0.0f);

        m_track = eClamp(0, ((eInt)newPos.y()-HEIGHT)/HEIGHT, eSequencer::MAX_TRACKS-1);
        m_startTime = eClamp(0.0f, (eF32)newPos.x()/m_scale, eDemo::MAX_RUNNING_TIME_MINS*60.0f-getDuration());
        m_op->getParameter(2).setChanged();
        m_op->getParameter(0).setChanged();
        m_op->setChanged(eFALSE, eTRUE);

        if (scene())
            scene()->invalidate(QRectF(), QGraphicsScene::ForegroundLayer);

        return QPoint(newPos.x(), m_track*HEIGHT+HEIGHT+1);
    }
 
    return QGraphicsItem::itemChange(change, value);
}

eDemoSeqView::eDemoSeqView(QWidget *parent) : QGraphicsView(parent),
    m_time(0.0f),
    m_showGaps(eTRUE),
    m_demoOp(nullptr),
    m_scrolling(eFALSE),
    m_gridCol(palette().window().color().darker(110))
{
    setScene(new QGraphicsScene(this));
    scene()->setItemIndexMethod(QGraphicsScene::NoIndex);

    // create time marker
    m_timeMarkerItem = scene()->addLine(QLineF(), QPen(palette().highlight().color()));
    m_timeMarkerItem->setZValue(2);
    m_timeMarkerItem->setFlags(m_timeMarkerItem->flags()|QGraphicsItem::ItemIgnoresTransformations);

    _updateTimeMarkerItem();
    _setScale(10.0f);

    // create context menu
    QAction *act = m_menu.addAction("Show gaps", this, SLOT(_onShowGaps(bool)));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("G"));
    act->setCheckable(true);
    act->setChecked(true);
    addAction(act);

    act = m_menu.addAction("Finish editing", this, SIGNAL(onFinishedEditing()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("Q"));
    addAction(act);
}

eDemoSeqView::~eDemoSeqView()
{
    eDelete(m_timeMarkerItem);
}

void eDemoSeqView::clear()
{
    for (eInt i=0; i<m_seqItems.size(); i++)
        eDelete(m_seqItems[i]);

    m_seqItems.clear();
}

void eDemoSeqView::setGridColor(const QColor &gridCol)
{
    m_gridCol = gridCol;
}

void eDemoSeqView::setTime(eF32 time)
{
    m_time = eClamp(0.0f, time, eDemo::MAX_RUNNING_TIME_MINS*60.0f);
    _updateTimeMarkerItem();
}

void eDemoSeqView::setDemoOp(eIDemoOp *demoOp)
{
    m_demoOp = demoOp;
    if (m_demoOp)
    {
        eASSERT(m_demoOp->getResultClass() == eOC_DEMO);
        m_demoOp->process(0.0f);
    }

    _updateSceneRect();
}

const QColor & eDemoSeqView::getGridColor() const
{
    return m_gridCol;
}

eIDemoOp * eDemoSeqView::getDemoOp() const
{
    return m_demoOp;
}

eF32 eDemoSeqView::getTime() const
{
    return m_time;
}

void eDemoSeqView::onOperatorChanged()
{
    Q_FOREACH(eDemoSeqItem *item, m_seqItems)
        item->updatePosition();

    scene()->invalidate();
}

void eDemoSeqView::showEvent(QShowEvent *se)
{
    QGraphicsView::showEvent(se);
    _createItems();
    _updateSceneRect();
}

void eDemoSeqView::mouseMoveEvent(QMouseEvent *me)
{
    const QPoint delta = me->pos()-m_lastMousePos;
    m_lastMousePos = me->pos();

    if (me->buttons()&Qt::LeftButton)
    {
        if (me->modifiers()&Qt::AltModifier) // zoom
        {
            _setScale(eMax(1.0f, m_scale+delta.x()/2.0f));
        }
        else if (m_demoOp && !m_scrolling) // edit
        {
            QGraphicsView::mouseMoveEvent(me);
            m_demoOp->process(m_time);
            _updateSceneRect();
        }
        else
        {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());
            me->accept();
        }
    }
    else if (me->buttons()&Qt::MidButton) // change time
    {
        const eF32 x = eMax(0.0, mapToScene(me->pos()).x());
        setTime(_positionToTime(x));
        Q_EMIT onTimeChanged(m_time);
    }
    else if (me->buttons()&Qt::RightButton) // modify right button events to left button to allow selection on right button
    {
        QMouseEvent me2(QEvent::MouseMove, me->pos(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QGraphicsView::mouseMoveEvent(&me2);
    }
}

void eDemoSeqView::mousePressEvent(QMouseEvent *me)
{
    m_mouseDownPos = me->pos();
    m_lastMousePos = m_mouseDownPos;

    if (me->button() == Qt::LeftButton && scene())
    {
        eDemoSeqItem *seqItem = qgraphicsitem_cast<eDemoSeqItem *>(scene()->itemAt(mapToScene(me->pos()), QTransform()));

        if (!seqItem)
        {
            setCursor(Qt::ClosedHandCursor);
            m_scrolling = eTRUE;
        }

        QGraphicsView::mousePressEvent(me);
    }
    else if (me->button() == Qt::MidButton)
    {
        mouseMoveEvent(me);
    }
    else // right button
    {
        setContextMenuPolicy(Qt::PreventContextMenu);
        QMouseEvent me2(QEvent::MouseButtonPress, me->pos(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QGraphicsView::mousePressEvent(&me2);
    }
}

void eDemoSeqView::mouseReleaseEvent(QMouseEvent *me)
{
    QGraphicsView::mouseReleaseEvent(me);

    if (me->button() == Qt::LeftButton)
    {
        if (m_scrolling)
        {
            setCursor(Qt::ArrowCursor);
            m_scrolling = eFALSE;
        }
        else
        {
            QGraphicsItem *item = scene()->itemAt(mapToScene(me->pos()), QTransform());
            if (item && item != m_timeMarkerItem)
            {
                eDemoSeqItem *seqItem = (eDemoSeqItem *)item;
                Q_EMIT onOperatorSelected(seqItem->getOperator());
            }
        }
    }
    else if (me->button() == Qt::RightButton)
    {
        QContextMenuEvent ce(QContextMenuEvent::Mouse, me->pos());
        setContextMenuPolicy(Qt::DefaultContextMenu);
        contextMenuEvent(&ce);
    }
}

void eDemoSeqView::contextMenuEvent(QContextMenuEvent *ce)
{
    QGraphicsView::contextMenuEvent(ce);

    // show menu only if the mouse was still
    if ((m_mouseDownPos-ce->pos()).manhattanLength() < 4)
        m_menu.exec(QCursor::pos());
}

void eDemoSeqView::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawBackground(painter, rect);
    painter->fillRect(rect, palette().window().color());
    painter->setClipRect(rect);

    // vertical lines and text
    QVector<QLineF> lines;
    QString buffer;

    eF64 s = 100.0f;
    eF32 scale = m_scale/100.0f;
    eU32 strength = 1;

    for (eU32 j=0; j<6; j++)
    {
        eF32 step = scale;
        if (step >= 10.0f)
        {
            for (eU32 i=0; ; i++)
            {
                eF32 x = i*scale;
                if (x > rect.right())
                {
                    strength++;
                    break;
                }

                if ((i%10) == 0)
                    continue;

                if (step > fontMetrics().width(buffer))
                {
                    const eF64 time = (eF64)i/s;
                    const eU32 mins = (eU32)time/60;
                    const eU32 secs = (eU32)time%60;
                    const eU32 hund = (eU32)((time-eTrunc(time))*100.0f);

                    buffer.sprintf("%.2i:%.2i:%.2i", mins, secs, hund);
                    painter->setPen(palette().windowText().color());
                    painter->drawText(x-fontMetrics().width(buffer)/2, 15, buffer);
                }
                
                painter->setPen(QPen(m_gridCol, strength));
                painter->drawLine(x, eDemoSeqItem::HEIGHT, x, sceneRect().bottom());
            }
        }

        scale *= 10.0f;
        s /= 10.0f;
    }

    // horizontal lines
    painter->setPen(m_gridCol);
    painter->drawLine(rect.left(), eDemoSeqItem::HEIGHT-1, rect.right(), eDemoSeqItem::HEIGHT-1);
    painter->drawLine(rect.left(), eDemoSeqItem::HEIGHT, rect.right(), eDemoSeqItem::HEIGHT);

    for (eInt i=2; i<=eSequencer::MAX_TRACKS+1; i++)
    {
        const eInt y = rect.top()+i*eDemoSeqItem::HEIGHT;
        painter->drawLine(rect.left(), y, rect.right(), y);
    }
}

void eDemoSeqView::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawForeground(painter, rect);

    if (m_showGaps)
        _drawGaps(painter, rect);

    _drawOverlaps(painter);
}

void eDemoSeqView::_onShowGaps(bool checked)
{
    m_showGaps = (checked == true);
    scene()->invalidate(QRectF(), QGraphicsScene::ForegroundLayer);
}

void eDemoSeqView::_createItems()
{
    clear();

    if (m_demoOp)
    {
        for (eU32 i=0; i<m_demoOp->getAboveOpCount(); i++)
        {
            eISequencerOp *seqOp = (eISequencerOp *)m_demoOp->getAboveOp(i);
            eDemoSeqItem *item = new eDemoSeqItem(seqOp, m_scale);
            m_seqItems.append(item);
            scene()->addItem(item);
        }
    }
}

 static eBool sortByStartTime(const eDemoSeqItem *item0, const eDemoSeqItem *item1)
 {
     return (item0->getStartTime() < item1->getStartTime());
 }

void eDemoSeqView::_drawGaps(QPainter *painter, const QRectF &rect)
{
    // remove all completely overlapped items
    // and sort by start time
    eDemoSeqItemPtrList items = m_seqItems;

    for (eInt i=items.size()-1; i>=0; i--)
    {
        for (eInt j=0; j<i; j++)
        {
            if (items[j]->getStartTime() <= items[i]->getStartTime() &&
                items[j]->getEndTime() >= items[i]->getEndTime())
            {
                items.removeAt(i);
                break;
            }
        }
    }

    qSort(items.begin(), items.end(), sortByStartTime);

    // find out gaps between remaining items
    QList<eF32> gapStarts, gapEnds;

    for (eInt i=0; i<items.size()-1; i++)
    {
        if (items[i]->getEndTime() < items[i+1]->getStartTime())
        {
            gapStarts.append(items[i]->getEndTime());
            gapEnds.append(items[i+1]->getStartTime());
        }
    }

    // is there a gap between time 0 and the first item?
    if (items.size() > 0 && items[0]->getStartTime() > 0.0f)
    {
        gapStarts.append(0.0f);
        gapEnds.append(items[0]->getStartTime());
    }

    // draw gaps
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(palette().mid().color(), Qt::Dense4Pattern));

    for (eInt i=0; i<gapStarts.size(); i++)
    {
        const eF32 x0 = _timeToPosition(gapStarts[i]);
        const eF32 x1 = _timeToPosition(gapEnds[i]);

        painter->drawRect(QRectF(x0, eDemoSeqItem::HEIGHT+1, x1-x0, rect.bottom()));
    }

    painter->restore();
}

void eDemoSeqView::_drawOverlaps(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(Qt::red, Qt::Dense4Pattern));

    for (eInt i=0; i<m_seqItems.size(); i++)
    {
        const QList<QGraphicsItem *> collided = m_seqItems[i]->collidingItems();

        for (eInt j=0; j<collided.size(); j++)
            if (collided[j] != m_timeMarkerItem) // don't collide with time marker
            painter->drawRect(collided[j]->sceneBoundingRect().intersected(m_seqItems[i]->sceneBoundingRect()));
    }

    painter->restore();
}

void eDemoSeqView::_updateTimeMarkerItem()
{
    const QPointF p0 = mapToScene(0.0, 0.0);
    const QPointF p1 = mapToScene(0.0, viewport()->height());
    const eF32 x = _timeToPosition(m_time);
    m_timeMarkerItem->setLine(x, p0.y(), x, p1.y());
    scene()->invalidate();
}

void eDemoSeqView::_updateSceneRect()
{
    eF32 maxEndTime = 0.0f;
    for (eInt i=0; i<m_seqItems.size(); i++)
        maxEndTime = eMax(maxEndTime, m_seqItems[i]->getEndTime());

    const eF32 w = _timeToPosition(!m_seqItems.size() ? 30.0f : maxEndTime);
    const eF32 h = (eF32)((eSequencer::MAX_TRACKS+1)*eDemoSeqItem::HEIGHT);

    setSceneRect(0.0f, 0.0f, w, h);
    _updateTimeMarkerItem();
}

void eDemoSeqView::_setScale(eF32 scale)
{
    eASSERT(scale >= 1.0f);
    m_scale = scale;

    for (eInt i=0; i<m_seqItems.size(); i++)
        m_seqItems[i]->setScale(scale);

    _updateSceneRect();
    _updateTimeMarkerItem();
}

eF32 eDemoSeqView::_timeToPosition(eF32 time) const
{
    eASSERT(time >= 0.0f);
    return time*m_scale;
}

eF32 eDemoSeqView::_positionToTime(eF32 pos) const
{
    eASSERT(pos >= 0.0f);
    return pos/m_scale;
}