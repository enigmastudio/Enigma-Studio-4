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

#ifndef PAGE_VIEW_HPP
#define PAGE_VIEW_HPP

#include <QtCore/QTimeLine>

#include <QtWidgets/QGraphicsItemAnimation>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QMenu>

#include "addopdlg.hpp"
#include "findopdlg.hpp"

#include "../../eshared/eshared.hpp"

class QDomDocument;
class QDomElement;

enum eGuiOpDimensions
{
    eGUIOP_WIDTH = 20,
    eGUIOP_HEIGHT = 17,
    eGUIOP_RESIZE = 9
};

class eGuiOperator : public QGraphicsObject
{
    Q_OBJECT

public:
    eGuiOperator(eU32 opType, const ePoint &pos, class eGuiOpPage *ownerPage, eInt width=-1, eID opId=eNOID, eBool reconnect=eTRUE);
    virtual ~eGuiOperator();

    void                    saveToXml(QDomElement &node) const;
    void                    loadFromXml(const QDomElement &node);

    eIOperator *            getOperator() const;
    eBool                   isResizing() const;

    static void             setViewingOp(eGuiOperator *guiOp);
    static void             setEditingOp(eGuiOperator *guiOp);

    virtual QRectF          boundingRect() const;
    virtual void            paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    void                    _initialize();
    void                    _paintStatusMarker(QPainter *painter);
    QPixmap                 _createPixmap() const;
    const QString &         _cacheKey() const;
    void                    _saveParameter(const eParameter &param, QDomElement &parent) const;
    void                    _savePath(const ePath4 &path, QDomElement &parent) const;
    void                    _loadParameter(eParameter &param, QDomElement &parent) const;
    void                    _loadPath(ePath4 &path, QDomElement &parent) const;

protected:
    virtual void            mouseReleaseEvent(QGraphicsSceneMouseEvent *me);
    virtual void            mousePressEvent(QGraphicsSceneMouseEvent *me);
    virtual void            mouseMoveEvent(QGraphicsSceneMouseEvent *me);
    virtual void            hoverLeaveEvent(QGraphicsSceneHoverEvent *he);
    virtual void            hoverMoveEvent(QGraphicsSceneHoverEvent *he);

private:
    static eID              m_viewingOpId;
    static eID              m_editingOpId;
    eIOperator *            m_op;
    eBool                   m_resizing;
};

class eGuiOpPage : public QGraphicsScene
{
    Q_OBJECT
    Q_PROPERTY(QColor colInsertAt READ getInsertAtColor WRITE setInsertAtColor);

public:
    eGuiOpPage(const QString &name="", QGraphicsView *parent=nullptr);
    eGuiOpPage(eOperatorPage *opPage, QGraphicsView *parent=nullptr);
    virtual ~eGuiOpPage();

    void                    saveToXml(QDomElement &node) const;
    void                    loadFromXml(const QDomElement &node);
    void                    incInsertAt();
    void                    setInsertAtColor(const QColor &col);

    const QColor &          getInsertAtColor() const;
    eOperatorPage *         getPage() const;
    ePoint                  getInsertAt() const;
    QPoint                  getViewPosition() const;
    eGuiOperator *          getGuiOperator(eID opId) const;

private:
    void                    _initialize();
    void                    _updateViewingPos();
    void                    _drawLinkLines(QPainter *painter) const;
    void                    _drawInsertAtMarker(QPainter *painter) const;
    QRectF                  _getOperatorRect(const eIOperator *op) const;
    QPointF                 _getNearestIntersectionPoint(const QRectF &r, const QLineF &line) const;

protected:
    virtual void            drawForeground(QPainter *painter, const QRectF &rect);
    virtual void            drawBackground(QPainter *painter, const QRectF &rect);
    virtual void            mousePressEvent(QGraphicsSceneMouseEvent *me);
    virtual void            mouseReleaseEvent(QGraphicsSceneMouseEvent *me);

private:
    static const eInt       ARROW_SIZE = 5;

private:
    eOperatorPage *         m_opPage;
    ePoint                  m_insertAt;
    QPoint                  m_viewPos;
    Qt::MouseButtons        m_pressButtons;
    QPointF                 m_pressPos;
    QColor                  m_colInsertAt;
};

typedef QMap<eOperatorPage *, eGuiOpPage *> eGuiOpPagePtrMap;

class ePageView : public QGraphicsView
{
    Q_OBJECT

public:
    ePageView(QWidget *parent);

    void                    scrollTo(const QPoint &pos);
    void                    gotoOperator(eGuiOperator *guiOp);

Q_SIGNALS:
    void                    onOperatorShow(eIOperator *op);
    void                    onOperatorAdded(eIOperator *op);
    void                    onOperatorSelected(eIOperator *op);
    void                    onOperatorRemoved(eIOperator *op);
    void                    onPathOperatorEdit(eIOperator *op);
    void                    onDemoOperatorEdit(eIOperator *op);
    void                    onGotoOperator(eID opId);

private:
    void                    _createActions();
    void                    _createGotoAnimation();

    eGuiOperator *          _addOperator(eU32 opType, const ePoint &pos, eBool selectOp=eTRUE);

private Q_SLOTS:
    void                    _onZoomIn();
    void                    _onZoomOut();
    void                    _onShowGrid();
    void                    _onSelectAll();
    void                    _onFindOperators();
    void                    _onOperatorAdd();
    void                    _onRemoveSelected();

    void                    _onOperatorShow();
    void                    _onOperatorRemove();
    void                    _onOperatorCut();
    void                    _onOperatorCopy();
    void                    _onOperatorPaste();
    void                    _onPathOperatorEdit();
    void                    _onDemoOperatorEdit();
    void                    _onLoadOperatorGoto();

private:
    virtual void            drawForeground(QPainter *painter, const QRectF &rect);
    virtual void            drawBackground(QPainter *painter, const QRectF &rect);

    virtual void            mouseDoubleClickEvent(QMouseEvent *de);
    virtual void            mouseMoveEvent(QMouseEvent *me);
    virtual void            mousePressEvent(QMouseEvent *me);
    virtual void            mouseReleaseEvent(QMouseEvent *me);
    virtual void            contextMenuEvent(QContextMenuEvent *ce);

private:
    static const eF32       ZOOM_STEP;

private:
    QPoint                  m_lastMousePos;
    QPoint                  m_mouseDownPos;
    QPoint                  m_mouseMoveDist;
    eBool                   m_showGrid;
    eBool                   m_moving;
    eBool                   m_connecting;
    QMenu                   m_opMenu;
    QMenu                   m_viewMenu;
    eAddOpDlg               m_addOpDlg;
    eFindOpDlg              m_findOpDlg;
    QTimeLine               m_gotoAnimTimeLine;
    QGraphicsItemAnimation  m_gotoAnim;
    eF32                    m_zoom;
};

#endif // PAGE_VIEW_HPP