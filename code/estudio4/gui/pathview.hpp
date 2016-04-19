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

#ifndef PATH_VIEW_HPP
#define PATH_VIEW_HPP

#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QRubberBand>
#include <QtWidgets/QMenu>

#include "../../eshared/eshared.hpp"

class ePathView : public QGraphicsView
{
    Q_OBJECT
    Q_PROPERTY(QColor colMajorAxes READ getMajorAxesColor WRITE setMajorAxesColor);
    Q_PROPERTY(QColor colMinorAxes READ getMinorAxesColor WRITE setMinorAxesColor);

public:
    ePathView(QWidget *parent);
    virtual ~ePathView();

    void                        setMajorAxesColor(const QColor &col);
    void                        setMinorAxesColor(const QColor &col);
    void                        setEditWidgetsParent(QWidget *parent);
    void                        setPathOp(eIPathOp *pathOp);
    void                        setViewOp(eIOperator *viewOp); // for "add camera ..." feature
    void                        setTime(eF32 time);

    const QColor &              getMajorAxesColor() const;
    const QColor &              getMinorAxesColor() const;
    eIPathOp *                  getPathOp() const;
    eIOperator *                getViewOp() const;
    eF32                        getTime() const;

Q_SIGNALS:
    void                        onFinishedEditing();
    void                        onTimeChanged(eF32 time);

private Q_SLOTS:
    void                        _onCutKeys();
    void                        _onCopyKeys();
    void                        _onPasteKeys();
    void                        _onAddKey();
    void                        _onAddAllKey();
    void                        _onSelectAll();
    void                        _onFramePath();
    void                        _onRemoveKeys();
    void                        _onCubicInterpolation();
    void                        _onLinearInterpolation();
    void                        _onStepInterpolation();
    void                        _onToggleShowPathX(bool checked);
    void                        _onToggleShowPathY(bool checked);
    void                        _onToggleShowPathZ(bool checked);
    void                        _onToggleShowPathW(bool checked);
    void                        _onToggleContinueLoop();
    void                        _onToggleContinueClampLast();
    void                        _onToggleContinueClampZero();
    void                        _onKeyEdited();

private:
    virtual void                drawBackground(QPainter *painter, const QRectF &rect);
    virtual void                mouseDoubleClickEvent(QMouseEvent *me);
    virtual void                mousePressEvent(QMouseEvent *me);
    virtual void                mouseReleaseEvent(QMouseEvent *me);
    virtual void                mouseMoveEvent(QMouseEvent *me);
    virtual void                contextMenuEvent(QContextMenuEvent *ce);

private:
    void                        _createMenus();
    void                        _drawCoordinateSystem(QPainter *painter, const QRectF &r) const;
    void                        _drawAxisLabels(QPainter *painter, const QPointF &center, const QString &text) const;
    void                        _drawKeys(QPainter *painter) const;
    void                        _drawPaths(QPainter *painter) const;
    void                        _updatePathSamples();
    void                        _createKeyEditWidgets();
    void                        _updateSceneRect();
    void                        _updateTimeMarkerItem();
    QPointF                     _keyToPos(const eF32 &time, const eF32 &val) const;
    eVector2                    _posToKey(const QPointF &pos) const;
    eF32                        _timeToPixel(eF32 time) const;
    eF32                        _valueToPixel(eF32 val) const;
    eBool                       _isKeyMovable(const ePathKey &key, eF32 newTime) const;
    void                        _setKeyInterpolation(ePathKeyInterpol interpol);
    ePathKey *                  _clickedOnSelection(const QPointF &mousePos);
    eArray<ePathKey *>          _getKeysOfAllPaths();
    const ePath4 &              _getPath() const;
    ePath4 &                    _getPath();
    void                        _add4NewKeys(eF32 time, const eVector4 &val);

private:
    static const eF32           KEY_SIZE;

private:
    QWidget *                   m_editWidgetsParent;
    QMap<ePathKey *, QWidget *> m_key2Widget;
    eIOperator *                m_viewOp;
    eIPathOp *                  m_pathOp;
    ePath *                     m_activeSubPath;
    eVector2                    m_zoom;
    eVector2                    m_oldZoom;
    eF32                        m_time;
    eBool                       m_showPaths[4];
    QMenu                       m_keyMenu;
    QMenu                       m_defMenu;
    QActionGroup                m_interpolAg;
    QActionGroup                m_continueAg;
    QRubberBand                 m_rubberBand;
    QPoint                      m_lastMousePos;
    QPoint                      m_mouseDownPos;
    ePath4Sampler               m_pathSampler;
    QGraphicsLineItem *         m_timeMarkerItem;
    QColor                      m_colMajorAxes;
    QColor                      m_colMinorAxes;
};

#endif // PATH_VIEW_HPP