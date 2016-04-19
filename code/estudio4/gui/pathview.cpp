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

#include <QtGui/QMouseEvent>
#include <QtGui/QClipboard>

#include <QtCore/QMimeData>

#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QLabel>

#include "trackedit.hpp"
#include "pathview.hpp"

const eF32 ePathView::KEY_SIZE = 8.0f;

ePathView::ePathView(QWidget *parent) : QGraphicsView(parent),
    m_editWidgetsParent(nullptr),
    m_pathOp(nullptr),
    m_viewOp(nullptr),
    m_activeSubPath(nullptr),
    m_time(0.0f),
    m_zoom(50.0f, 15.0f),
    m_interpolAg(this),
    m_continueAg(this),
    m_rubberBand(QRubberBand::Rectangle, this),
    m_colMajorAxes(palette().window().color().darker(150)),
    m_colMinorAxes(palette().window().color().darker(110))
{
    setScene(new QGraphicsScene(this));

    m_timeMarkerItem = scene()->addLine(QLineF(), palette().highlight().color());
    m_timeMarkerItem->setZValue(2);
    m_timeMarkerItem->setFlags(m_timeMarkerItem->flags()|QGraphicsItem::ItemIgnoresTransformations);

    eMemSet(m_showPaths, eTRUE, sizeof(m_showPaths));

    _createMenus();
    _updateSceneRect();
}

ePathView::~ePathView()
{
    eDelete(m_timeMarkerItem);
}

void ePathView::setMajorAxesColor(const QColor &col)
{
    m_colMajorAxes = col;
}

void ePathView::setMinorAxesColor(const QColor &col)
{
    m_colMinorAxes = col;
}

// should be only called once
void ePathView::setEditWidgetsParent(QWidget *parent)
{
    eASSERT(!m_editWidgetsParent);
    m_editWidgetsParent = parent;
}

void ePathView::setPathOp(eIPathOp *pathOp)
{
    eASSERT(scene());

    m_pathOp = pathOp;
    _updateSceneRect();

    if (!m_pathOp)
    {
        // clear menu and actions
        for (eInt i=actions().size()-1; i>=0; i--)
            removeAction(actions().at(i));

        m_activeSubPath = nullptr;
        scene()->invalidate();
        return;
    }

    eASSERT(pathOp->getResultClass() == eOC_PATH);
    m_activeSubPath = &_getPath().getSubPath(0);
    _updatePathSamples();
    _createKeyEditWidgets();
    addActions(m_defMenu.actions());
    addActions(m_keyMenu.actions());
}

void ePathView::setViewOp(eIOperator *viewOp)
{
    m_viewOp = viewOp;
}

void ePathView::setTime(eF32 time)
{
    m_time = eClamp(0.0f, time, eDemo::MAX_RUNNING_TIME_MINS*60.0f);
    _updateTimeMarkerItem();
}

const QColor & ePathView::getMajorAxesColor() const
{
    return m_colMajorAxes;
}

const QColor & ePathView::getMinorAxesColor() const
{
    return m_colMinorAxes;
}

eIPathOp * ePathView::getPathOp() const
{
    return m_pathOp;
}

eIOperator * ePathView::getViewOp() const
{
    return m_viewOp;
}

eF32 ePathView::getTime() const
{
    return m_time;
}

void ePathView::_onCutKeys()
{
    _onCopyKeys();
    _onRemoveKeys();
}

void ePathView::_onCopyKeys()
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);

    for (eU32 i=0; i<4; i++)
    {
        const ePath &subPath = _getPath().getSubPath(i);
        eF32 minTime = eF32_MAX;

        for (eU32 j=0; j<subPath.getKeyCount(); j++)
            if (subPath.getKeyByIndex(j).selected)
                minTime = eMin(minTime, subPath.getKeyByIndex(j).time);

        for (eU32 j=0; j<subPath.getKeyCount(); j++)
        {
            const ePathKey &key = subPath.getKeyByIndex(j);
            if (key.selected)
                ds << key.time-minTime << key.val << i << key.interpol;
        }
    }

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("enigma-keys/binary", data);
    QApplication::clipboard()->setMimeData(mimeData);
}

void ePathView::_onPasteKeys()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    QByteArray &data = mimeData->data("enigma-keys/binary");
    QDataStream ds(&data, QIODevice::ReadOnly);

    while (!ds.atEnd())
    {
        eF32 time, val;
        eInt subPathIdx, interpol;
        ds >> time;
        ds >> val;
        ds >> subPathIdx;
        ds >> interpol;
        eASSERT(subPathIdx < 4);
        _getPath().getSubPath(subPathIdx).addKey(m_time+time, val, (ePathKeyInterpol)interpol);
    }

    _updatePathSamples();
    _createKeyEditWidgets();
    m_pathOp->getParameter(0).setChanged();
}

void ePathView::_onAddKey()
{
    const eF32 val = m_activeSubPath->evaluate(m_time);
    const ePathKey *keyAt = m_activeSubPath->getKeyByTime(m_time);

    m_activeSubPath->addKey(m_time, val, (keyAt ? keyAt->interpol : ePI_CUBIC));
    _updatePathSamples();
    _createKeyEditWidgets();
    m_pathOp->getParameter(0).setChanged();
}

void ePathView::_onAddAllKey()
{
    _add4NewKeys(m_time, _getPath().evaluate(m_time));
}

void ePathView::_add4NewKeys(eF32 time, const eVector4 &val)
{
    ePath4 &path = _getPath();

    for (eU32 i=0; i<4; i++)
    {
        ePath &subPath = path.getSubPath(i);
        const ePathKey *keyAt = subPath.getKeyByTime(m_time);
        subPath.addKey(m_time, val[i], (keyAt ? keyAt->interpol : ePI_CUBIC));
    }

    _updatePathSamples();
    _createKeyEditWidgets();
    m_pathOp->getParameter(0).setChanged();
}

void ePathView::_onSelectAll()
{
    const eArray<ePathKey *> &keys = _getKeysOfAllPaths();
    for (eU32 i=0; i<keys.size(); i++)
        keys[i]->selected = eTRUE;

    scene()->invalidate();
}

void ePathView::_onFramePath()
{
    eF32 maxAbsVal = eF32_MIN;
    eF32 maxAbsTime = eF32_MIN;

    for (eU32 i=0; i<m_pathSampler.getSampleCount(); i++)
    {
        const ePath4Sample &s = m_pathSampler.getSample(i);
        maxAbsTime = eMax(maxAbsTime, eAbs(s.time));

        for (eU32 j=0; j<4; j++)
            maxAbsVal = eMax(maxAbsVal, eAbs(s.values[j]));
    }

	maxAbsVal *= 1.2f;

    m_zoom.x = ((eF32)viewport()->width()-KEY_SIZE)/(maxAbsTime);
    m_zoom.y = ((eF32)viewport()->height()-KEY_SIZE)/(2.0f*maxAbsVal);

    _updateSceneRect();
}

void ePathView::_onRemoveKeys()
{
    for (eU32 i=0; i<4; i++)
    {
        ePath &subPath = _getPath().getSubPath(i);
        for (eInt j=(eInt)subPath.getKeyCount()-1; j>=0; j--)
            if (subPath.getKeyByIndex(j).selected && subPath.getKeyCount() > 2) // minimum number of keys is 2
                subPath.removeKey(j);
    }

    _updatePathSamples();
    _createKeyEditWidgets();
    m_pathOp->getParameter(0).setChanged();
}

void ePathView::_onCubicInterpolation()
{
    _setKeyInterpolation(ePI_CUBIC);
}

void ePathView::_onLinearInterpolation()
{
    _setKeyInterpolation(ePI_LINEAR);
}

void ePathView::_onStepInterpolation()
{
    _setKeyInterpolation(ePI_STEP);
}

void ePathView::_onToggleShowPathX(bool checked)
{
    m_showPaths[0] = (checked == true);
    scene()->invalidate();
}

void ePathView::_onToggleShowPathY(bool checked)
{
    m_showPaths[1] = (checked == true);
    scene()->invalidate();
}

void ePathView::_onToggleShowPathZ(bool checked)
{
    m_showPaths[2] = (checked == true);
    scene()->invalidate();
}

void ePathView::_onToggleShowPathW(bool checked)
{
    m_showPaths[3] = (checked == true);
    scene()->invalidate();
}

void ePathView::_onToggleContinueLoop()
{
    m_activeSubPath->setLoopMode(ePLM_LOOP);
    _updatePathSamples();
}

void ePathView::_onToggleContinueClampLast()
{
    m_activeSubPath->setLoopMode(ePLM_LAST);
    _updatePathSamples();
}

void ePathView::_onToggleContinueClampZero()
{
    m_activeSubPath->setLoopMode(ePLM_ZERO);
    _updatePathSamples();
}

void ePathView::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawBackground(painter, rect);

    painter->fillRect(rect, palette().window().color());
    _drawCoordinateSystem(painter, rect);
    _drawPaths(painter);
}

void ePathView::mouseDoubleClickEvent(QMouseEvent *me)
{
    QGraphicsView::mouseDoubleClickEvent(me);

    ePathKey *selKey = _clickedOnSelection(mapToScene(me->pos()));
    if (selKey)
        m_key2Widget[selKey]->setFocus();
}

void ePathView::mousePressEvent(QMouseEvent *me)
{
    m_lastMousePos = me->pos();
    m_mouseDownPos = me->pos();
    m_oldZoom = m_zoom;

    if (me->button() == Qt::RightButton)
    {
        ePathKey *selKey = _clickedOnSelection(mapToScene(me->pos()));
        if (!selKey)
        {
            m_rubberBand.setGeometry(QRect(m_mouseDownPos, QSize()));
            m_rubberBand.show();
        }
        
        setContextMenuPolicy(Qt::PreventContextMenu);
    }
    else if (me->button() == Qt::MidButton)
    {
        // change time
        mouseMoveEvent(me);
    }
    else
    {
        const eArray<ePathKey *> keys = _getKeysOfAllPaths();
        ePathKey *selKey = _clickedOnSelection(mapToScene(me->pos()));
        eBool inside = eFALSE;

        for (eU32 i=0; i<keys.size(); i++)
            if (keys[i]->selected && keys[i] == selKey)
                inside = eTRUE;

        if (!inside)
            for (eU32 i=0; i<keys.size(); i++)
                keys[i]->selected = eFALSE;

        if (selKey)
        {
            selKey->selected = eTRUE;
            m_activeSubPath = selKey->ownerPath;
        }

        scene()->invalidate();

        if (!selKey)
        {
            setCursor(Qt::ClosedHandCursor);
            me->accept();
        }

        mouseMoveEvent(me);
    }
}

void ePathView::mouseReleaseEvent(QMouseEvent *me)
{
    if (me->button() == Qt::LeftButton)
    {
        setCursor(Qt::ArrowCursor);
        me->accept();
    }
    else if (me->button() == Qt::RightButton)
    {
        QContextMenuEvent ce(QContextMenuEvent::Mouse, me->pos());
        
        QGraphicsView::mouseReleaseEvent(me);
        m_rubberBand.hide();
        setContextMenuPolicy(Qt::DefaultContextMenu);
        contextMenuEvent(&ce);
    }
}

void ePathView::mouseMoveEvent(QMouseEvent *me)
{
    const QPoint delta = me->pos()-m_lastMousePos;
    m_lastMousePos = me->pos();

    if (me->buttons()&Qt::LeftButton)
    {
        if (me->modifiers()&Qt::AltModifier) // zoom
        {
            // calculate new zoom based on moved distance such that
            // time(clicked at, new zoom) -> time(moved to, old zoom)
            m_zoom.x = (eF32)me->pos().x()/(eF32)m_mouseDownPos.x()*m_oldZoom.x;
            m_zoom.y = (eF32)me->pos().y()/(eF32)m_mouseDownPos.y()*m_oldZoom.y;
            m_zoom.x = eClamp(1.0f, m_zoom.x, eF32_MAX);
            _updateSceneRect();
        }
        else if (m_pathOp) // edit
        {
            eArray<ePathKey *> keys = _getKeysOfAllPaths();
            const eVector2 &keyDelta = _posToKey(delta);
            eBool changed = eFALSE;

            for (eU32 i=0; i<keys.size(); i++)
            {
                if (keys[i]->selected)
                {
                    // change time (x-coordinate)
                    if (!me->modifiers() || me->modifiers()&Qt::ShiftModifier)
                    {
                        const eF32 t = eClamp(0.0f, keys[i]->time+keyDelta.x, eDemo::MAX_RUNNING_TIME_MINS*60.0f);
                        if (_isKeyMovable(*keys[i], t))
                        {
                            keys[i]->time = t;
                            changed = eTRUE;
                        }
                    }

                    // change value (y-coordinate)
                    if (!me->modifiers() || me->modifiers()&Qt::ControlModifier)
                    {
                        keys[i]->val -= keyDelta.y;
                        changed = eTRUE;
                    }
                }
            }

            // panning or a key was changed?
            if (!changed) 
            {
                horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());
                verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());
                me->accept();
            }
            else
            {
                m_pathOp->getParameter(0).setChanged();
                _updatePathSamples();
            }
        }
    }
    else if (me->buttons()&Qt::MidButton) // change time
    {
        const eF32 t = _posToKey(mapToScene(me->pos())).x;
        setTime(t);
        Q_EMIT onTimeChanged(m_time);
    }
    else if (me->buttons()&Qt::RightButton) // select
    {
        m_rubberBand.setGeometry(QRect(m_mouseDownPos, me->pos()).normalized());

        eArray<ePathKey *> keys = _getKeysOfAllPaths();
        for (eU32 i=0; i<keys.size(); i++)
        {
            const QPointF &p = _keyToPos(keys[i]->time, keys[i]->val);
            const QRect r(p.x()-KEY_SIZE*0.5f, p.y()-KEY_SIZE*0.5f, KEY_SIZE, KEY_SIZE);
            keys[i]->selected = mapToScene(m_rubberBand.geometry()).boundingRect().contains(r);
        }

        scene()->invalidate();
    }
}

void ePathView::contextMenuEvent(QContextMenuEvent *ce)
{
    QGraphicsView::contextMenuEvent(ce);

    // show menu only if the mouse was still
    if (m_pathOp && (m_mouseDownPos-ce->pos()).manhattanLength() < 4)
    {
        if (_clickedOnSelection(mapToScene(ce->pos())))
        {
            const eArray<ePathKey *> &keys = _getPath().getAllKeys();
            const ePathLoopMode loopMode = (m_activeSubPath ? m_activeSubPath->getLoopMode() : ePLM_LOOP);
            ePathKeyInterpol interpol = ePI_CUBIC;

            for (eU32 i=0; i<keys.size(); i++)
                if (keys[i]->selected)
                    interpol = keys[i]->interpol;

            m_interpolAg.actions().at(interpol)->setChecked(true);
            m_continueAg.actions().at(loopMode)->setChecked(true);
            m_keyMenu.exec(QCursor::pos());
        }
        else
            m_defMenu.exec(QCursor::pos());
    }
}

void ePathView::_createMenus()
{
    // create default context menu
    QAction *act = m_defMenu.addAction("Add", this, SLOT(_onAddKey()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("A"));

    act = m_defMenu.addAction("Add all", this, SLOT(_onAddAllKey()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("W"));

    act = m_defMenu.addAction("Paste", this, SLOT(_onPasteKeys()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("V"));

    act = m_defMenu.addAction("Frame path", this, SLOT(_onFramePath()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("F"));

    act = m_defMenu.addAction("Select all", this, SLOT(_onSelectAll()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("Ctrl+A"));

    m_defMenu.addSeparator();

    act = m_defMenu.addAction("Show path X", this, SLOT(_onToggleShowPathX(bool)));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("1"));
    act->setCheckable(true);
    act->setChecked(true);

    act = m_defMenu.addAction("Show path Y", this, SLOT(_onToggleShowPathY(bool)));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("2"));
    act->setCheckable(true);
    act->setChecked(true);

    act = m_defMenu.addAction("Show path Z", this, SLOT(_onToggleShowPathZ(bool)));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("3"));
    act->setCheckable(true);
    act->setChecked(true);

    act = m_defMenu.addAction("Show path W", this, SLOT(_onToggleShowPathW(bool)));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("4"));
    act->setCheckable(true);
    act->setChecked(true);

    m_defMenu.addSeparator();

    act = m_defMenu.addAction("Finish editing", this, SIGNAL(onFinishedEditing()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("P"));

    // create key context menu
    act = m_keyMenu.addAction("Remove", this, SLOT(_onRemoveKeys()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence::Delete);

    act = m_keyMenu.addAction("Cut", this, SLOT(_onCutKeys()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("X"));

    act = m_keyMenu.addAction("Copy", this, SLOT(_onCopyKeys()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("C"));

    act = m_keyMenu.addSeparator();

    act = m_keyMenu.addAction("Cubic", this, SLOT(_onCubicInterpolation()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("Q"));
    act->setCheckable(true);
    act->setChecked(true);
    m_interpolAg.addAction(act);

    act = m_keyMenu.addAction("Linear", this, SLOT(_onLinearInterpolation()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("L"));
    act->setCheckable(true);
    m_interpolAg.addAction(act);

    act = m_keyMenu.addAction("Step", this, SLOT(_onStepInterpolation()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("S"));
    act->setCheckable(true);
    m_interpolAg.addAction(act);

    m_keyMenu.addSeparator();

    act = m_keyMenu.addAction("Loop path", this, SLOT(_onToggleContinueLoop()));
    act->setCheckable(true);
    act->setChecked(true);
    m_continueAg.addAction(act);

    act = m_keyMenu.addAction("Clamp last", this, SLOT(_onToggleContinueClampLast()));
    act->setCheckable(true);
    m_continueAg.addAction(act);

    act = m_keyMenu.addAction("Clamp zero", this, SLOT(_onToggleContinueClampZero()));
    act->setCheckable(true);
    m_continueAg.addAction(act);
}

void ePathView::_drawCoordinateSystem(QPainter *painter, const QRectF &rect) const
{
    // draw vertical lines
    painter->save();
    painter->setPen(QPen(m_colMinorAxes, 1, Qt::DashLine));

    QString text;
    eF32 timeStep = 1.0f;
    const QRectF &r = sceneRect();
    const eInt w = 2*painter->fontMetrics().width("0:00");
    QPointF p = _keyToPos(timeStep, 0.0f);
    eInt mul = eMax((eInt)eRound(w/p.x()), 1);
    eInt index = 0;

    for (eF32 time=0.0f; ; time+=timeStep, index++)
    {
        p = _keyToPos(time, 0.0f);
        if (p.x() > r.right()+2.0)
            break;

        if (index%mul == 0)
        {
            const eU32 mins = (eU32)time/60;
            const eU32 secs = (eU32)time%60;
            text.sprintf("%.1i:%.2i", mins, secs);

            painter->drawLine(p.x(), rect.top(), p.x(), rect.bottom());
            _drawAxisLabels(painter, p, (time == 0.0 ? "" : text));
        }
    }

    // draw horizontal lines
    eF32 scale = 1.0f/10.0f;

    for (eU32 i=0; i<5; i++, scale*=10.0f)
    {
        const eF32 d = eAbs((eF32)_keyToPos(0, 2.0f*scale).y())-eAbs((eF32)_keyToPos(0.0f, scale).y());
        if (d > 10.0f)
        {
            for (eInt j=-9; j<=9; j++)
            {
                if (j != 0) // do not draw line at y=0
                {
                    const eF32 y = (eF32)j*scale;
                    p = _keyToPos(0.0f, y);
                    painter->drawLine(r.left(), p.y(), r.right(), p.y());
                    _drawAxisLabels(painter, p, QString::number(y, 'f', 1));
                }
            }
        }
    }

    painter->restore();

    // draw coordinate system's major axis
    const eF32 yOrigin = _keyToPos(0.0f, 0.0f).y();

    painter->save();
    painter->setPen(QPen(m_colMajorAxes, 2, Qt::SolidLine));
    painter->drawLine(r.left(), yOrigin, r.right(), yOrigin);
    painter->drawLine(1.0f, rect.top(), 1.0f, rect.bottom());
    painter->drawLine(r.right(), rect.top(), r.right(), rect.bottom());
    painter->restore();
}

void ePathView::_drawAxisLabels(QPainter *painter, const QPointF &center, const QString &text) const
{
    QRectF br = painter->fontMetrics().boundingRect(text+" "); // bound rect somehow a bit to narrow
    br.moveCenter(center);

    if (br.left() < 0.0) // left from y-axis?
        br.moveLeft(2.0);
    if (center.y() == 0.0) // on x-axis?
        br.translate(0.0, br.height()/2.0);

    painter->save();
    painter->setPen(palette().windowText().color());
    painter->fillRect(br, palette().window().color());
    painter->drawText(br, Qt::AlignVCenter|Qt::AlignRight, text);
    painter->restore();
}

void ePathView::_drawKeys(QPainter *painter) const
{
    painter->save();

    for (eU32 i=0; i<4; i++)
    {
        if (m_showPaths[i])
        {
            const ePath &subPath = _getPath().getSubPath(i);
            for (eU32 j=0; j<subPath.getKeyCount(); j++)
            {
                const ePathKey &key = subPath.getKeyByIndex(j);
                const QPointF &p = _keyToPos(key.time, key.val);

                if (key.selected)
                {
                    const QString text = QString("(%1/%2)").arg(key.time, 0, 'f', 3).arg(key.val, 0, 'f', 3);
                    const QSizeF s = painter->fontMetrics().size(Qt::TextSingleLine, text);
                    painter->setPen(palette().windowText().color());
                    painter->drawText(QRectF(p.x()-0.5f*s.width(), p.y()-s.height()-KEY_SIZE*0.5-1, s.width(), s.height()), text);
                }

                painter->setPen(palette().windowText().color());
                painter->setBrush(key.selected ? QBrush(palette().windowText().color()) : Qt::NoBrush);
                painter->drawRect(QRectF(p-0.5f*QPointF(KEY_SIZE, KEY_SIZE), QSizeF(KEY_SIZE, KEY_SIZE)));
            }
        }
    }

    painter->restore();
}

void ePathView::_drawPaths(QPainter *painter) const
{
    if (!m_pathOp)
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // draw path between keys
    const QColor pathCols[4] = {Qt::red, Qt::green, Qt::blue, Qt::magenta};

    for (eU32 i=0; i<4; i++)
    {
        if (m_showPaths[i])
        {
            const ePath &subPath = _getPath().getSubPath(i);
            const eF32 penWidth = (m_activeSubPath == &subPath ? 2.0 : 0.5);
            painter->setPen(QPen(pathCols[i], penWidth));

            for (eInt j=0; j<(eInt)m_pathSampler.getSampleCount()-1; j++)
            {
                const ePath4Sample &s0 = m_pathSampler.getSample(j);
                const ePath4Sample &s1 = m_pathSampler.getSample(j+1);
                painter->drawLine(_keyToPos(s0.time, s0.values[i]), _keyToPos(s1.time, s1.values[i]));
            }
        }
    }

    // draw keys
    _drawKeys(painter);
    painter->restore();
}

void ePathView::_updatePathSamples()
{
    if (m_pathOp)
    {      
        m_pathSampler.sample(_getPath(), (eU32)(m_zoom.x+m_zoom.y));
        _updateSceneRect();
    }
}

void ePathView::_createKeyEditWidgets()
{
    // delete all widgets
    qDeleteAll(m_editWidgetsParent->findChildren<QWidget *>());
    delete m_editWidgetsParent->layout();
    m_key2Widget.clear();

    if (!m_pathOp)
        return;

    // create new widgets
    QVBoxLayout *vbl = new QVBoxLayout(m_editWidgetsParent);
    QHBoxLayout *hbl = new QHBoxLayout;
    hbl->addWidget(new QLabel("Time"), 1, Qt::AlignHCenter);
    hbl->addWidget(new QLabel("Value"), 1, Qt::AlignHCenter);
    vbl->addLayout(hbl);

    ePath4 &path = _getPath();
    eParameter &param = m_pathOp->getParameter(0);

    for (eU32 i=0, row=0; i<4; i++)
    {
        static const QString pathNames[] =
        {
            "<font color=\"red\">Path X</font>",
            "<font color=\"green\">Path Y</font>",
            "<font color=\"blue\">Path Z</font>",
            "<font color=\"magenta\">Path W</font>"
        };

        QFrame *sepLeft = new QFrame;
        sepLeft->setFrameStyle(QFrame::HLine|QFrame::Sunken);
        QFrame *sepRight = new QFrame;
        sepRight->setFrameStyle(QFrame::HLine|QFrame::Sunken);
        QLabel *lbl = new QLabel(pathNames[i]);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QGridLayout *gl = new QGridLayout;
        QHBoxLayout *subHbl = new QHBoxLayout;
        subHbl->addWidget(sepLeft, 1);
        subHbl->addWidget(lbl);
        subHbl->addWidget(sepRight, 1);
        vbl->addLayout(subHbl);
        vbl->addLayout(gl);

        for (eU32 j=0; j<path.getSubPath(i).getKeyCount(); j++, row++)
        {
            ePathKey &key = path.getSubPath(i).getKeyByIndex(j);
            eTrackEdit *teTime = new eTrackEdit(key.time, 0.0f, eF32_MAX, this);
            eTrackEdit *teVal = new eTrackEdit(key.val, eF32_MIN, eF32_MAX, this);
            connect(teTime, SIGNAL(onValueChanged()), this, SLOT(_onKeyEdited()));
            connect(teVal, SIGNAL(onValueChanged()), this, SLOT(_onKeyEdited()));
            gl->addWidget(teTime, row, 0);
            gl->addWidget(teVal, row, 1);
            m_key2Widget[&key] = teTime;
        }
    }

    // add spacer to compact widgets
    vbl->addItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));
}

void ePathView::_onKeyEdited()
{
    m_pathOp->getParameter(0).setChanged();
    _updatePathSamples();
}

void ePathView::_updateSceneRect()
{
    // find scene extents
    const eArray<ePathKey *> &keys = _getKeysOfAllPaths();
    const eF32 endTime = (keys.isEmpty() ? 10.0f : _getPath().getEndTime());
    
    eF32 maxAbsY = eF32_MIN;
    for (eU32 i=0; i<keys.size(); i++)
        maxAbsY = eMax(maxAbsY, eAbs(keys[i]->val));

    // set scene extents
    const eF32 top = _valueToPixel(keys.isEmpty() ? 1.0f : maxAbsY);
    setSceneRect(0.0, top, _timeToPixel(endTime), 2.0*eAbs(top));
    _updateTimeMarkerItem();
}

void ePathView::_updateTimeMarkerItem()
{
    const QPointF p0 = mapToScene(0.0, 0.0);
    const QPointF p1 = mapToScene(0.0, viewport()->height());
    const eF32 x = _keyToPos(m_time, 0.0f).x();
    m_timeMarkerItem->setLine(x, p0.y(), x, p1.y());
    scene()->invalidate();
}

QPointF ePathView::_keyToPos(const eF32 &time, const eF32 &val) const
{
    return QPointF(_timeToPixel(time), _valueToPixel(val));
}

eVector2 ePathView::_posToKey(const QPointF &pos) const
{
    return eVector2(pos.x()/m_zoom.x, pos.y()/m_zoom.y);
}

eF32 ePathView::_timeToPixel(eF32 time) const
{
    return time*m_zoom.x;
}

eF32 ePathView::_valueToPixel(eF32 val) const
{
    // negate cause coordiante system of graphics scene is flipped
    return -val* m_zoom.y;
}

eBool ePathView::_isKeyMovable(const ePathKey &key, eF32 newTime) const
{
    for (eU32 i=0; i<key.ownerPath->getKeyCount(); i++)
    {
        const ePathKey &curKey = key.ownerPath->getKeyByIndex(i);
        if (&key != &curKey)
        {
            if ((key.time <= curKey.time && newTime >= curKey.time) ||
                (key.time >= curKey.time && newTime <= curKey.time))
            {
                return eFALSE;
            }
        }
    }

    return eTRUE;
}

void ePathView::_setKeyInterpolation(ePathKeyInterpol ipt)
{
    const eArray<ePathKey *> &keys = _getKeysOfAllPaths();
    for (eU32 i=0; i<keys.size(); i++)
        if (keys[i]->selected)
            keys[i]->interpol = ipt;

    m_pathOp->getParameter(0).setChanged();
    _updatePathSamples();
}

ePathKey * ePathView::_clickedOnSelection(const QPointF &mousePos)
{
    eArray<ePathKey *> &keys = _getKeysOfAllPaths();
    for (eU32 i=0; i<keys.size(); i++)
    {
        const QPointF &p = _keyToPos(keys[i]->time, keys[i]->val);
        const QRectF r(p.x()-KEY_SIZE*0.5f, p.y()-KEY_SIZE*0.5f, KEY_SIZE, KEY_SIZE);

        if (r.contains(mousePos))
            return keys[i];
    }

    return nullptr;
}

eArray<ePathKey *> ePathView::_getKeysOfAllPaths()
{
    return (m_pathOp ? _getPath().getAllKeys() : eArray<ePathKey *>());
}

const ePath4 & ePathView::_getPath() const
{
    eASSERT(m_pathOp);
    return m_pathOp->getParameter(0).getBaseValue().path;
}

ePath4 & ePathView::_getPath()
{
    eASSERT(m_pathOp);
    return m_pathOp->getParameter(0).getBaseValue().path;
}