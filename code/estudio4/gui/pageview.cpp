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

#include "findopdlg.hpp"
#include "pageview.hpp"

#include <QtXml/QDomDocument>

#include <QtWidgets/QStyleOptionGraphicsItem>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QDrawUtil.h>

#include <QtGui/QPixmapCache>
#include <QtGui/QMouseEvent>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>

#include <QtCore/QMimeData>
#include <QtCore/QBuffer>
#include <QtCore/QTimer>

eID eGuiOperator::m_viewingOpId = eNOID;
eID eGuiOperator::m_editingOpId = eNOID;

// set reonnect to false to reconnect page your-self
// after adding a large amount of operators (e.g.
// when loading a project or pasting operators)
eGuiOperator::eGuiOperator(eU32 opType, const ePoint &pos, eGuiOpPage *ownerPage, eInt width, eID opId, eBool reconnect) : QGraphicsObject(nullptr),
    m_resizing(eFALSE)
{
    eOperatorPage *page = ownerPage->getPage();
    m_op = page->addOperator(opType, pos, width, opId);

    // was there enough space to create operator?
    if (m_op)
    {
        setFlags(ItemIsFocusable|ItemIsSelectable|ItemClipsToShape);
        setPos(m_op->getPosition().x*eGUIOP_WIDTH, m_op->getPosition().y*eGUIOP_HEIGHT);
        setAcceptHoverEvents(true);

        if (reconnect)
            eDemoData::connectPages();
    }
}

eGuiOperator::~eGuiOperator()
{
    if (m_op)
        m_op->getOwnerPage()->removeOperator(m_op->getId());
}

eIOperator * eGuiOperator::getOperator() const
{
    return m_op;
}

void eGuiOperator::saveToXml(QDomElement &node) const
{
    QDomDocument xml = node.ownerDocument();
    QDomElement opEl = xml.createElement("operator");
    opEl.setAttribute("category", QString(m_op->getMetaInfos().category));
    opEl.setAttribute("name", QString(m_op->getMetaInfos().name));
    opEl.setAttribute("id", m_op->getId());
    opEl.setAttribute("username", QString(m_op->getUserName()));
    opEl.setAttribute("xpos", m_op->getPosition().x);
    opEl.setAttribute("ypos", m_op->getPosition().y);
    opEl.setAttribute("width", m_op->getWidth());
    opEl.setAttribute("bypassed", m_op->getBypassed());
    opEl.setAttribute("hidden", m_op->getHidden());
    opEl.setAttribute("script", QString(m_op->getScript().source));
    node.appendChild(opEl);

    for (eU32 i=0; i<m_op->getParameterCount(); i++)
    {
        QDomElement xmlParam = xml.createElement("parameter");
        _saveParameter(m_op->getParameter(i), xmlParam);
        opEl.appendChild(xmlParam);
    }
}

void eGuiOperator::loadFromXml(const QDomElement &node)
{
    m_op->getScript().source = node.attribute("script").toLocal8Bit().constData();
    m_op->setUserName(node.attribute("username").toLocal8Bit().constData());
    m_op->setBypassed(node.attribute("bypassed").toInt());
    m_op->setHidden(node.attribute("hidden").toInt());

    QDomElement xmlParam = node.firstChildElement("parameter");
    while (!xmlParam.isNull())
    {
        const QString &paramName = xmlParam.attribute("name");
        for (eU32 i=0; i<m_op->getParameterCount(); i++)
        {
            eParameter &param = m_op->getParameter(i);
            if (paramName == param.getName())
            {
                _loadParameter(param, xmlParam);
                param.setChanged();
                break;
            }
        }

        xmlParam = xmlParam.nextSiblingElement("parameter");
    }
}

eBool eGuiOperator::isResizing() const
{
    return m_resizing;
}

void eGuiOperator::setViewingOp(eGuiOperator *guiOp)
{
    if (!guiOp)
        m_viewingOpId = eNOID;
    else
    {
        m_viewingOpId = guiOp->getOperator()->getId();
        guiOp->scene()->invalidate();
    }
}

void eGuiOperator::setEditingOp(eGuiOperator *guiOp)
{
    if (!guiOp)
        m_editingOpId = eNOID;
    else
    {
        m_editingOpId = guiOp->getOperator()->getId();
        guiOp->scene()->invalidate();
    }
}

QRectF eGuiOperator::boundingRect() const
{
    return QRectF(0, 0, m_op->getWidth()*eGUIOP_WIDTH, eGUIOP_HEIGHT);
}

void eGuiOperator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPixmap pixmap;
    if (!QPixmapCache::find(_cacheKey(), pixmap))
    {
        pixmap = _createPixmap();
        QPixmapCache::insert(_cacheKey(), pixmap);
    }

    painter->setClipRect(option->exposedRect); // optimizes drawing
    painter->drawPixmap(0, 0, pixmap);
    _paintStatusMarker(painter);

    // draw text on top of operator
    const QString opName = m_op->getMetaInfos().name;
    QString userName = m_op->getUserName();

    if (opName == "Load")
    {
        const eIOperator *loadedOp = eDemoData::findOperator(m_op->getParameter(0).getBaseValue().linkedOpId);
        if (loadedOp)
            userName = loadedOp->getUserName();
    }

    const QString caption = (userName != "" ? QString('"')+userName+'"' : opName);
    painter->setPen(m_op->getBypassed() || m_op->getHidden() ? Qt::gray : Qt::white);
    painter->drawText(boundingRect().adjusted(1, 3, -eGUIOP_RESIZE, -1), Qt::AlignCenter, caption);
}

void eGuiOperator::_paintStatusMarker(QPainter *painter)
{
    eASSERT(painter);

    if (m_viewingOpId == m_op->getId())
    {
        painter->setPen(Qt::red);
        painter->setBrush(QBrush(Qt::red, Qt::SolidPattern));
        painter->drawRect(2, 4, 2, 2);
    }

    if (m_editingOpId == m_op->getId())
    {
        painter->setPen(Qt::blue);
        painter->setBrush(QBrush(Qt::blue, Qt::SolidPattern));
        painter->drawRect(2, 8, 2, 2);
    }
}

static QColor mulColors(const QColor &c1, const QColor &c2)
{
    return QColor((c1.redF() * c2.redF())*255,
                  (c1.greenF() * c2.greenF())*255,
                  (c1.blueF() * c2.blueF())*255);
}

QPixmap eGuiOperator::_createPixmap() const
{
    QImage img(boundingRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);

    QPixmap pixmap = QPixmap::fromImage(img);
    QPainter painter(&pixmap);

    QColor opColor = QColor(m_op->getMetaInfos().color.toArgb());
    opColor.setHsv(opColor.hsvHue(), 160, 255);

    QColor opColorDesat = opColor;
    opColorDesat.setHsv(opColor.hsvHue(), 40, 255);

    if (m_op->getError() != eOE_OK)
        opColor = QColor(230, 0, 0);

    const QRect &r = boundingRect().toRect();
    const QRect rCol(QPoint(1, 1), QPoint(r.width()-2, 2));

    QColor grColor2 = mulColors(QColor(60, 70, 80), opColorDesat);
    QColor grColor = mulColors(QColor(110, 120, 130), opColorDesat);

    QLinearGradient gradient(0.0f, 0.0f, r.width(), eGUIOP_HEIGHT);
    QLinearGradient gradientCol(0.0f, 0.0f, r.width(), eGUIOP_HEIGHT);

    gradientCol.setColorAt(0.0, opColor.lighter(150));
    gradientCol.setColorAt(1.0, opColor.darker(150));
    gradient.setColorAt(0.0, grColor);
    gradient.setColorAt(1.0, grColor2);

    QPointF tl = boundingRect().topLeft();
    QPointF tr = boundingRect().topRight();
    QPointF bl = boundingRect().bottomLeft();
    QPointF br = boundingRect().bottomRight();

    QPointF points[] =
    {
        QPointF(tl.x()+5, tl.y()),
        QPointF(tr.x()-1-5, tr.y()),
        QPointF(tr.x()-1, tr.y()+5),
        QPointF(tr.x()-1, br.y()-1),
        QPointF(bl.x(), bl.y()-1),
        QPointF(bl.x(), tl.y()+5),
    };

    pixmap.fill(Qt::transparent);
    painter.setPen(QColor(Qt::white));
    painter.setBrush(opColor);

    QPalette pal(QColor(90, 100, 110));
    const QString opName = m_op->getMetaInfos().name;

    if (opName == "Load")
    {
        painter.save();
        painter.setBrush(QBrush(gradient));
        painter.setPen(isSelected() ? QPen(pal.dark(), 1) : QPen(pal.light(), 1));
        painter.drawPolygon(points, sizeof(points)/sizeof(QPointF), Qt::WindingFill);

        painter.setPen(isSelected() ? QPen(pal.light(), 1) : QPen(pal.dark(), 1));
        painter.drawLine(points[1], points[2]);
        painter.drawLine(points[2], points[3]);
        painter.drawLine(points[3], points[4]);
        painter.restore();

        painter.setPen(QPen(QBrush(gradientCol), 1));
        painter.drawLine(QPoint(5, 1), QPoint(r.right()-5, 1));
        painter.drawLine(QPoint(4, 2), QPoint(r.right()-4, 2));
    }
    else if (opName == "Store")
    {
        painter.save();
        painter.setTransform(QTransform().scale(1.0, -1.0).translate(0.0, -boundingRect().height()+1));
        painter.setPen(isSelected() ? QPen(pal.dark(), 1) : QPen(pal.light(), 1));
        painter.setBrush(QBrush(gradient));
        painter.drawPolygon(points, sizeof(points)/sizeof(QPointF), Qt::WindingFill);

        painter.setPen(isSelected() ? QPen(pal.light(), 1) : QPen(pal.dark(), 1));
        painter.drawLine(points[0], points[1]);
        painter.drawLine(points[1], points[2]);
        painter.drawLine(points[2], points[3]);
        painter.restore();

        qDrawShadePanel(&painter, rCol, QPalette(QColor(10,20,30)), false, 0, &QBrush(gradientCol));
    }
    else
    {
        qDrawShadePanel(&painter, boundingRect().toRect(), pal, isSelected(), 1);
        qDrawShadePanel(&painter, boundingRect().toRect().adjusted(1, 1, -1, -1), pal, false, 0, &QBrush(gradient));
        qDrawShadePanel(&painter, rCol, pal, false, 0, &QBrush(gradientCol));
    }

    // draw resize area marker
    painter.setPen(QPen(QBrush(QColor(140, 150, 160)), 1, Qt::DotLine));
    painter.drawLine(boundingRect().right()-eGUIOP_RESIZE, 4, boundingRect().right()-eGUIOP_RESIZE, eGUIOP_HEIGHT-2);
    return pixmap;
}

// returns key uniquely identifying look of
// this operator. used to find pixmap in cache.
const QString & eGuiOperator::_cacheKey() const
{
    static QString key;
    key = eIntToStr(m_op->getMetaInfos().type);
    key += m_op->getUserName();
    key += eIntToStr(m_op->getWidth());
    key += (m_op->getError() == eOE_OK ? "1" : "0");
    key += (isSelected() ? "1" : "0");
    key += (m_op->getBypassed() ? "1" : "0");
    key += (m_op->getHidden() ? "1" : "0");
    return key;
}

void eGuiOperator::_saveParameter(const eParameter &param, QDomElement &parent) const
{
    const eParamValue &baseVal = param.getBaseValue();
    parent.setAttribute("name", QString(param.getName()));

    switch (param.getType())
    {
    case ePT_PATH:
        _savePath(baseVal.path, parent);
        break;

    case ePT_LABEL:
    case ePT_STR:
    case ePT_TEXT:
    case ePT_FILE:
        parent.setAttribute("value", QString(baseVal.string));
        break;

    case ePT_BOOL:
    case ePT_FLAGS:
        parent.setAttribute("value", QString::number(baseVal.flags, 2));
        break;

    case ePT_FLOAT:
    case ePT_FXY:
    case ePT_FXYZ:
    case ePT_FXYZW:
        for (eU32 j=0; j<param.getComponentCount(); j++)
            parent.setAttribute(QString("value%1").arg(j), eVector4(baseVal.fxyzw)[j]);
        break;

    case ePT_RGB:
    case ePT_RGBA:
        for (eU32 j=0; j<param.getComponentCount(); j++)
            parent.setAttribute(QString("value%1").arg(j), baseVal.color[j]);
        break;

    default:
        for (eU32 j=0; j<param.getComponentCount(); j++)
            parent.setAttribute(QString("value%1").arg(j), eRect(baseVal.ixyxy)[j]);
        break;
    }
}

void eGuiOperator::_savePath(const ePath4 &path, QDomElement &parent) const
{
    QDomDocument &xml = parent.ownerDocument();
    QDomElement xmlPath = xml.createElement("path");

    for (eU32 i=0; i<4; i++)
    {
        const ePath &subPath = path.getSubPath(i);
        QDomElement xmlSubPath = xml.createElement("subpath");
        xmlSubPath.setAttribute("number", i);
        xmlSubPath.setAttribute("continue", subPath.getLoopMode());

        for (eU32 j=0; j<subPath.getKeyCount(); j++)
        {
            const ePathKey &key = subPath.getKeyByIndex(j);
            QDomElement xmlKey = xml.createElement("key");

            xmlSubPath.appendChild(xmlKey);
            xmlKey.setAttribute("interpol", key.interpol);
            xmlKey.setAttribute("time", key.time);
            xmlKey.setAttribute(QString("value"), key.val);
        }

        xmlPath.appendChild(xmlSubPath);
    }

    parent.appendChild(xmlPath);
}

void eGuiOperator::_loadParameter(eParameter &param, QDomElement &parent) const
{
    eParamValue &baseVal = param.getBaseValue();

    switch (param.getType())
    {
    case ePT_PATH:
        _loadPath(baseVal.path, parent.firstChildElement("path"));
        break;

    case ePT_LABEL:
    case ePT_STR:
    case ePT_TEXT:
    case ePT_FILE:
        baseVal.string = parent.attribute("value").toLocal8Bit();
        break;

    case ePT_BOOL:
    case ePT_FLAGS:
        baseVal.flags = parent.attribute("value").toInt(nullptr, 2);
        break;

    case ePT_FLOAT:
    case ePT_FXY:
    case ePT_FXYZ:
    case ePT_FXYZW:
        for (eU32 j=0; j<param.getComponentCount(); j++)
            ((eVector4 &)baseVal.fxyzw)[j] = parent.attribute(QString("value%1").arg(j)).toFloat();
        break;

    case ePT_RGB:
    case ePT_RGBA:
        for (eU32 j=0; j<param.getComponentCount(); j++)
            baseVal.color[j] = parent.attribute(QString("value%1").arg(j)).toInt();
        break;

    default:
        for (eU32 j=0; j<param.getComponentCount(); j++)
            ((eRect &)baseVal.ixyxy)[j] = parent.attribute(QString("value%1").arg(j)).toInt();
        break;
    }
}

void eGuiOperator::_loadPath(ePath4 &path, QDomElement &parent) const
{
    QDomElement xmlSubPath = parent.firstChildElement("subpath");

    for (eInt i=0; i<4; i++)
    {
        const eInt cm = xmlSubPath.attribute("continue").toInt();
        const eInt spn = xmlSubPath.attribute("number").toInt();
        ePath &subPath = path.getSubPath(spn);
        subPath.clear();
        subPath.setLoopMode((ePathLoopMode)cm);

        QDomElement xmlKey = xmlSubPath.firstChildElement("key");
        while (!xmlKey.isNull())
        {
            const eF32 time = xmlKey.attribute("time").toFloat();
            const eInt interpol = xmlKey.attribute("interpol").toInt();
            const eF32 val = xmlKey.attribute("value").toFloat();

            subPath.addKey(time, val, (ePathKeyInterpol)interpol);
            xmlKey = xmlKey.nextSiblingElement("key");
        }

        xmlSubPath = xmlSubPath.nextSiblingElement("subpath");
    }
}

void eGuiOperator::mouseReleaseEvent(QGraphicsSceneMouseEvent *me)
{
    QGraphicsItem::mouseReleaseEvent(me);
    m_resizing = eFALSE;
}

void eGuiOperator::mousePressEvent(QGraphicsSceneMouseEvent *me)
{
    QGraphicsItem::mousePressEvent(me);

    // user clicked with left mouse button into
    // resize area of operator => start resizing
    if (me->button() == Qt::LeftButton && me->pos().x() >= boundingRect().right()-eGUIOP_RESIZE)
        m_resizing = eTRUE;
}

void eGuiOperator::mouseMoveEvent(QGraphicsSceneMouseEvent *me)
{
    if (m_resizing)
    {
        eASSERT(scene());

        // calculate new width of operator
        eInt newWidth = (eInt)eRound(me->pos().x()/(eF32)eGUIOP_WIDTH);
        newWidth = eClamp(1, newWidth, eOPPAGE_WIDTH-m_op->getPosition().x);

        if (newWidth != m_op->getWidth())
        {
            prepareGeometryChange();
            ((eGuiOpPage *)scene())->getPage()->resizeOperator(m_op, newWidth);
            eDemoData::connectPages();
            scene()->invalidate();
        }
    }
    else // move operator per default implementation
        QGraphicsItem::mouseMoveEvent(me);
}

void eGuiOperator::hoverLeaveEvent(QGraphicsSceneHoverEvent *he)
{
    QGraphicsItem::hoverLeaveEvent(he);
    scene()->views().at(0)->setCursor(Qt::ArrowCursor); // has to be called on the view (QTBUG-4190)
}

void eGuiOperator::hoverMoveEvent(QGraphicsSceneHoverEvent *he)
{
    QGraphicsItem::hoverMoveEvent(he);

    const Qt::CursorShape cursor = (he->pos().x() >= boundingRect().right()-eGUIOP_RESIZE ? Qt::SizeHorCursor : Qt::ArrowCursor);
    scene()->views().at(0)->setCursor(cursor);  // has to be called on the view (QTBUG-4190)
}

static void drawArrow(QPainter *painter, const QLineF &line, eF32 length, eF32 angle)
{
    // calculate points
    const eVector2 start(line.p1().x(), line.p1().y());
    const eVector2 end(line.p2().x(), line.p2().y());
    const eF32 oa = eATan2(end.y-start.y, end.x-start.x)+ePI;
    const eVector2 l = end+length*eVector2(eCos(oa-angle), eSin(oa-angle));
    const eVector2 r = end+length*eVector2(eCos(oa+angle), eSin(oa+angle));

    // draw arrow
    const QPoint arrowPts[3] = {QPoint(l.x, l.y), QPoint(r.x, r.y), QPoint(end.x, end.y)};
    painter->drawLine(start.x, start.y, end.x, end.y);
    painter->drawConvexPolygon(arrowPts, 3);
}

eGuiOpPage::eGuiOpPage(const QString &name, QGraphicsView *parent) : QGraphicsScene(parent),
    m_colInsertAt(palette().highlight().color())
{
    _initialize();
    m_opPage = eDemoData::addPage();
    m_opPage->setUserName(eString(name.toLocal8Bit().constData()));
}

eGuiOpPage::eGuiOpPage(eOperatorPage *opPage, QGraphicsView *parent) : QGraphicsScene(parent),
    m_colInsertAt(palette().highlight().color())
{
    m_opPage = opPage;
    _initialize();
}

eGuiOpPage::~eGuiOpPage()
{
    // reverse loop because elements are removed
    // from items array when deleting them
    for (eInt i=items().size()-1; i>=0; i--)
    {
        QGraphicsItem *item = items().at(i);
        eDelete(item);
    }

    eDemoData::removePage(m_opPage->getId());
    eDemoData::connectPages();
}

void eGuiOpPage::saveToXml(QDomElement &node) const
{
    QDomDocument xml = node.ownerDocument();
    eASSERT(!xml.isNull());

    QDomElement pageEl = xml.createElement("page");
    pageEl.setAttribute("id", m_opPage->getId());
    pageEl.setAttribute("name", QString(m_opPage->getUserName()));
    node.appendChild(pageEl);

    for (eInt i=0; i<items().size(); i++)
    {
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(items().at(i));
        eASSERT(guiOp);
        guiOp->saveToXml(pageEl);
    }
}

void eGuiOpPage::loadFromXml(const QDomElement &node)
{
    m_opPage->setUserName(eString(node.attribute("name").toLocal8Bit().constData()));
    
    QDomElement xmlOp = node.firstChildElement("operator");
    while (!xmlOp.isNull())
    {
        const eID opId = xmlOp.attribute("id").toInt();
        const QByteArray category = xmlOp.attribute("category").toLocal8Bit();
        const QByteArray name = xmlOp.attribute("name").toLocal8Bit();
        const eU32 xpos = xmlOp.attribute("xpos").toInt();
        const eU32 ypos = xmlOp.attribute("ypos").toInt();
        const eU32 width = xmlOp.attribute("width").toInt();

        const eU32 opType = eOP_TYPE(category.constData(), name.constData());
        eGuiOperator *guiOp = new eGuiOperator(opType, ePoint(xpos, ypos), this, width, opId, eFALSE);

        // could operator be created?
        if (guiOp->getOperator())
        {
            addItem(guiOp);
            guiOp->loadFromXml(xmlOp);
        }
        else
        {
            eDelete(guiOp);
            eWriteToLog(eString("An operator of type '")+category+" :: "+name+"' could not created!");
        }

        xmlOp = xmlOp.nextSiblingElement("operator");
    }
}

// increments insert-at position's y-coordinate
// by one. this function is used when adding
// new operators to a page.
void eGuiOpPage::incInsertAt()
{
    if (m_insertAt.y < eOPPAGE_HEIGHT-1)
        m_insertAt.y++;
}

void eGuiOpPage::setInsertAtColor(const QColor &col)
{
    m_colInsertAt = col;
    invalidate();
}

const QColor & eGuiOpPage::getInsertAtColor() const
{
    return m_colInsertAt;
}

eOperatorPage * eGuiOpPage::getPage() const
{
    return m_opPage;
}

ePoint eGuiOpPage::getInsertAt() const
{
    return m_insertAt;
}

QPoint eGuiOpPage::getViewPosition() const
{
    return m_viewPos;
}

eGuiOperator * eGuiOpPage::getGuiOperator(eID opId) const
{
    for (eInt i=0; i<items().size(); i++)
    {
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(items().at(i));
        eASSERT(guiOp);

        if (guiOp->getOperator()->getId() == opId)
            return guiOp;
    }
    
    return nullptr;
}

void eGuiOpPage::_initialize()
{
    const eU32 width = eOPPAGE_WIDTH*eGUIOP_WIDTH;
    const eU32 height = eOPPAGE_HEIGHT*eGUIOP_HEIGHT;
    setSceneRect(0, 0, width, height);
    setItemIndexMethod(NoIndex);
}

void eGuiOpPage::_updateViewingPos()
{
    eASSERT(views().size() > 0);
    QGraphicsView *view = views().at(0);
    m_viewPos.setX(view->horizontalScrollBar()->value());
    m_viewPos.setY(view->verticalScrollBar()->value());
}

// Draw connection lines between linking
// operators (to visualize dependencies).
void eGuiOpPage::_drawLinkLines(QPainter *painter) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(palette().dark().color(), 1, Qt::DotLine));
    painter->setBrush(palette().dark().color());

    for (eU32 i=0; i<m_opPage->getOperatorCount(); i++)
    {
        const eIOperator *op = m_opPage->getOperatorByIndex(i);
        for (eU32 j=0; j<op->getLinkOutOpCount(); j++)
        {
            const eIOperator *loOp = op->getLinkOutOp(j);
            if (loOp->getOwnerPage() == m_opPage)
            {
                const QRectF &opRect = _getOperatorRect(op);
                const QRectF &loRect = _getOperatorRect(loOp);
                const QLineF line = QLineF(opRect.center(), loRect.center());
                const QPointF opNip = _getNearestIntersectionPoint(loRect, line);
                const QPointF loNip = _getNearestIntersectionPoint(opRect, line);
                drawArrow(painter, QLineF(loNip, opNip), 8, eDegToRad(30.0f));
            }
        }
    }

    painter->restore();
}

void eGuiOpPage::_drawInsertAtMarker(QPainter *painter) const
{
    // default width and height of an operator widget
    eInt w = eGUIOP_WIDTH*4;
    eInt h = eGUIOP_HEIGHT;

    // fade from half transparency to full transparency
    QLinearGradient gradient(0, 0, w, 0);
    QColor hla = m_colInsertAt;
    hla.setAlpha(127);
    gradient.setColorAt(0, hla);
    hla.setAlpha(0);
    gradient.setColorAt(1, hla);

    // draw background
    painter->save();
    painter->translate(QPoint(m_insertAt.x*eGUIOP_WIDTH, m_insertAt.y*eGUIOP_HEIGHT));
    qDrawPlainRect(painter, 0, 0, w+1, h+1, hla, 0, &QBrush(gradient));

    // fade from no transparency to full transparency
    gradient.setColorAt(0, hla);

    // draw the left, upper and lower lines
    painter->setPen(QPen(QBrush(gradient), 0));
    painter->drawLine(0, 0, 0, h);
    painter->drawLine(0, 0, w, 0);
    painter->drawLine(0, h, w, h);
    painter->restore();
}

// returns rectangle of operator in
// scene coordinates
QRectF eGuiOpPage::_getOperatorRect(const eIOperator *op) const
{
    return QRectF(op->getPosition().x*eGUIOP_WIDTH,
                  op->getPosition().y*eGUIOP_HEIGHT,
                  op->getWidth()*eGUIOP_WIDTH,
                  eGUIOP_HEIGHT);
}

QPointF eGuiOpPage::_getNearestIntersectionPoint(const QRectF &r, const QLineF &line) const
{
    // calculate intersections with rectangle sides
    const QLineF lines[4] =
    {
        QLineF(r.topLeft(),     r.topRight()),
        QLineF(r.topRight(),    r.bottomRight()),
        QLineF(r.bottomRight(), r.bottomLeft()),
        QLineF(r.bottomLeft(),  r.topLeft())
    };

    QVector<QPointF> ips;
    QPointF ip;

    for (eInt i=0; i<4; i++)
        if (lines[i].intersect(line, &ip) == QLineF::BoundedIntersection)
            ips.append(ip);

    eASSERT(!ips.isEmpty());

    // find nearest intersection point
    QPointF nip = ips[0];
    eF32 minDist = (nip-line.p2()).manhattanLength();

    for (eInt i=1; i<ips.size(); i++)
    {
        const eF32 dist = (ips[i]-line.p2()).manhattanLength();
        if (dist < minDist)
        {
            minDist = dist;
            nip = ips[i];
        }
    }

    return nip;
}

void eGuiOpPage::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);
    _updateViewingPos();
    _drawLinkLines(painter);
}

void eGuiOpPage::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);
    _drawInsertAtMarker(painter);
}

void eGuiOpPage::mousePressEvent(QGraphicsSceneMouseEvent *me)
{
    QGraphicsScene::mousePressEvent(me);
    m_pressButtons = me->buttons();
    m_pressPos = me->scenePos();
}

void eGuiOpPage::mouseReleaseEvent(QGraphicsSceneMouseEvent *me)
{
    QGraphicsScene::mouseReleaseEvent(me);

    // do not modify the insert-at cursor for drag&drop movements
    const QPointF dist = me->scenePos()-m_pressPos;
    if (eAbs((eInt)dist.x()) > eGUIOP_WIDTH || eAbs((eInt)dist.y()) > eGUIOP_HEIGHT)
        return;

    // reposition insert-at cursor on left/right mouse click
    if (m_pressButtons & (Qt::LeftButton|Qt::RightButton))
    {
        m_insertAt.x = me->scenePos().x()/eGUIOP_WIDTH;
        m_insertAt.y = me->scenePos().y()/eGUIOP_HEIGHT;
        invalidate();
    }
}

const eF32 ePageView::ZOOM_STEP = 1.15f;

ePageView::ePageView(QWidget *parent) : QGraphicsView(parent),
    m_addOpDlg(this),
    m_findOpDlg(this),
    m_showGrid(eFALSE),
    m_moving(eFALSE),
    m_connecting(eFALSE),
    m_zoom(1.0f)
{
    connect(&m_findOpDlg, SIGNAL(onGotoOperator(eID)), this, SIGNAL(onGotoOperator(eID)));

    _createActions();
    _createGotoAnimation();
}

// Scrolls graphics-view to given position.
void ePageView::scrollTo(const QPoint &pos)
{
    eASSERT(pos.x() >= 0);
    eASSERT(pos.y() >= 0);

    centerOn(0, 0); // fixes wrong initial scroll position
    horizontalScrollBar()->setValue(pos.x());
    verticalScrollBar()->setValue(pos.y());
}

void ePageView::gotoOperator(eGuiOperator *guiOp)
{
    ensureVisible(guiOp);
    m_gotoAnim.setItem(guiOp);
    m_gotoAnimTimeLine.start();
}

void ePageView::_createActions()
{
    // context menu for page view
    QAction *addOpAct = m_viewMenu.addAction("Add", this, SLOT(_onOperatorAdd()));
    addOpAct->setShortcutContext(Qt::WidgetShortcut);
    addOpAct->setShortcut(QKeySequence("A"));

    QAction *pasteAct = m_viewMenu.addAction("Paste", this, SLOT(_onOperatorPaste()));
    pasteAct->setShortcutContext(Qt::WidgetShortcut);
    pasteAct->setShortcut(QKeySequence("V"));

    QAction *removeAct = m_viewMenu.addAction("Remove", this, SLOT(_onRemoveSelected()));
    removeAct->setShortcutContext(Qt::WidgetShortcut);
    removeAct->setShortcut(QKeySequence(QKeySequence::Delete));

    QAction *act = m_viewMenu.addAction("Select all", this, SLOT(_onSelectAll()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("Ctrl+A"));

    QAction *findOpAct = m_viewMenu.addAction("Find", this, SLOT(_onFindOperators()));
    findOpAct->setShortcutContext(Qt::WidgetShortcut);
    findOpAct->setShortcut(QKeySequence("F"));

    m_viewMenu.addSeparator();

    act = m_viewMenu.addAction("Show grid", this, SLOT(_onShowGrid()));
    act->setCheckable(true);
    act->setChecked(false);

    act = m_viewMenu.addAction("Zoom in", this, SLOT(_onZoomIn()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("+"));

    act = m_viewMenu.addAction("Zoom out", this, SLOT(_onZoomOut()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("-"));

    addActions(m_viewMenu.actions());

    // context menu for operators
    act = m_opMenu.addAction("Show", this, SLOT(_onOperatorShow()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("S"));
    addAction(act);

    act = m_opMenu.addAction("Goto", this, SLOT(_onLoadOperatorGoto()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("G"));
    addAction(act);

    QAction *cutAct = m_opMenu.addAction("Cut", this, SLOT(_onOperatorCut()));
    cutAct->setShortcutContext(Qt::WidgetShortcut);
    cutAct->setShortcut(QKeySequence("X"));
    addAction(cutAct);

    QAction *copyAct = m_opMenu.addAction("Copy", this, SLOT(_onOperatorCopy()));
    copyAct->setShortcutContext(Qt::WidgetShortcut);
    copyAct->setShortcut(QKeySequence("C"));
    addAction(copyAct);

    act = m_opMenu.addAction("Edit path", this, SLOT(_onPathOperatorEdit()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("P"));
    addAction(act);

    act = m_opMenu.addAction("Edit timeline", this, SLOT(_onDemoOperatorEdit()));
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("Q"));
    addAction(act);

    m_opMenu.addSeparator();
    m_opMenu.addAction(removeAct);
    m_opMenu.addAction(cutAct);
    m_opMenu.addAction(copyAct);
}

void ePageView::_createGotoAnimation()
{
    m_gotoAnimTimeLine.setDuration(333);
    m_gotoAnimTimeLine.setLoopCount(3);
    m_gotoAnim.setTimeLine(&m_gotoAnimTimeLine);

    for (eInt i=0; i<=50; i++)
    {
        const eF32 t = (eF32)i/50.0f;
        const eF32 scale0 = eLerp(1.0f, 1.5f, t);
        const eF32 scale1 = eLerp(1.5f, 1.0f, t);

        m_gotoAnim.setScaleAt((eF32)i/100.0f, scale0, scale0);
        m_gotoAnim.setScaleAt((eF32)(i+50)/100.0f, scale1, scale1);
    }
}

// returns the new GUI operator or null if no
// operator could be create at the given position
eGuiOperator * ePageView::_addOperator(eU32 opType, const ePoint &pos, eBool selectOp)
{
    eASSERT(scene());
    eGuiOperator *guiOp = new eGuiOperator(opType, pos, (eGuiOpPage *)scene());

    // fail if we were unable to add the operator (e.g. due to overlap)
    if (!guiOp->getOperator())
    {
        eDelete(guiOp);
        return nullptr;
    }

    scene()->addItem(guiOp);
    Q_EMIT onOperatorAdded(guiOp->getOperator());

    if (selectOp)
    {
        scene()->clearSelection();
        guiOp->setSelected(true);
        guiOp->setFocus();
        eGuiOperator::setEditingOp(guiOp);
        Q_EMIT onOperatorSelected(guiOp->getOperator());
    }

    return guiOp;
}

void ePageView::_onZoomIn()
{
    if (m_zoom < 1.0f)
    {
        m_zoom *= ZOOM_STEP;
        setTransform(QTransform().scale(m_zoom, m_zoom));
    }
}

void ePageView::_onZoomOut()
{
    m_zoom /= ZOOM_STEP;
    setTransform(QTransform().scale(m_zoom, m_zoom));
}

void ePageView::_onShowGrid()
{
    m_showGrid = !m_showGrid;

    if (scene())
        scene()->invalidate();
}

void ePageView::_onSelectAll()
{
    for (eInt i=0; i<items().size(); i++)
        items().at(i)->setSelected(true);
}

void ePageView::_onFindOperators()
{
    m_findOpDlg.show();
}

void ePageView::_onOperatorAdd()
{
    // move dialog to current mouse position
    eGuiOpPage *guiPage = (eGuiOpPage *)scene();
    const ePoint insertAt = guiPage->getInsertAt();
    QPoint dlgPos(insertAt.x*eGUIOP_WIDTH, insertAt.y*eGUIOP_HEIGHT);
    dlgPos = mapToGlobal(mapFromScene(dlgPos));
    
    m_addOpDlg.move(dlgPos);

    // set filtering operator
    eIOperator *filterOp = nullptr;
    for (eU32 i=0; i<4 && !filterOp; i++)
        filterOp = guiPage->getPage()->getOperatorByPos(ePoint(insertAt.x+i, insertAt.y-1));

    m_addOpDlg.setFilterOp(filterOp);

    // execute dialog and add operator if one was selected
    if (m_addOpDlg.exec() == QDialog::Accepted)
        if (_addOperator(m_addOpDlg.getChosenOpType(), insertAt))
            guiPage->incInsertAt();
}

void ePageView::_onRemoveSelected()
{
    for (eInt i=scene()->selectedItems().size()-1; i>=0; i--)
    {
        QGraphicsItem *item = scene()->selectedItems().at(i);
        if (item->type() != QGraphicsTextItem::Type)
        {
            eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(item);
            eASSERT(guiOp);
            Q_EMIT onOperatorRemoved(guiOp->getOperator());
        }

        eDelete(item);
    }

    eDemoData::connectPages();
    scene()->invalidate();
}

void ePageView::_onOperatorShow()
{
    if (scene() && scene()->selectedItems().size() > 0)
    {
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->selectedItems().first());
        eASSERT(guiOp);
        eGuiOperator::setViewingOp(guiOp);
        Q_EMIT onOperatorShow(guiOp->getOperator());
        invalidateScene();
    }
}

void ePageView::_onOperatorRemove()
{
    _onRemoveSelected();
}

void ePageView::_onOperatorCut()
{
    _onOperatorCopy();
    _onRemoveSelected();
}

void ePageView::_onOperatorCopy()
{
    eASSERT(scene());

    QDomDocument xml;
    QDomElement rootEl = xml.createElement("operators");
    xml.appendChild(rootEl);

    for (eInt i=0; i<scene()->selectedItems().size(); i++)
    {
        const eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->selectedItems().at(i));
        eASSERT(guiOp);
        guiOp->saveToXml(rootEl);
    }

    QMimeData *md = new QMimeData;
    md->setData("enigma-operator/xml", QByteArray(xml.toString().toLocal8Bit()));
    QApplication::clipboard()->setMimeData(md);
}

void ePageView::_onOperatorPaste()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    eGuiOpPage *guiPage = (eGuiOpPage *)scene();

    // verify that clipboard format is correct
    const QMimeData *md = QApplication::clipboard()->mimeData();
    if (!md->hasFormat("enigma-operator/xml"))
        return;

    // get operator with smallest position
    ePoint minPos(eS32_MAX, eS32_MAX);
    QDomDocument xml;
    xml.setContent(md->data("enigma-operator/xml"));

    QDomElement xmlOp = xml.firstChild().firstChildElement("operator");
    while (!xmlOp.isNull())
    {
        minPos.x = eMin(minPos.x, xmlOp.attribute("xpos").toInt());
        minPos.y = eMin(minPos.y, xmlOp.attribute("ypos").toInt());
        xmlOp = xmlOp.nextSiblingElement("operator");
    }

    // add operators to page
    eIOpPtrArray pastedOps;
    eArray<eU32> pastedOpIds;

    xmlOp = xml.firstChild().firstChildElement("operator");
    while (!xmlOp.isNull())
    {
        const QByteArray category = xmlOp.attribute("category").toLocal8Bit();
        const QByteArray name = xmlOp.attribute("name").toLocal8Bit();
        const eU32 xpos = xmlOp.attribute("xpos").toInt();
        const eU32 ypos = xmlOp.attribute("ypos").toInt();
        const eU32 width = xmlOp.attribute("width").toInt();
        const eID opId = xmlOp.attribute("id").toInt();
        const ePoint newPos = guiPage->getInsertAt()+ePoint(xpos, ypos)-minPos;

        // does position lie on page?
        if (newPos.y < eOPPAGE_HEIGHT && newPos.x+width <= eOPPAGE_WIDTH)
        {
            const eU32 opType = eOP_TYPE(category.constData(), name.constData());
            eGuiOperator *guiOp = new eGuiOperator(opType, newPos, guiPage, width, eNOID, eFALSE);
            eIOperator *op = guiOp->getOperator();

            // operator was added successfully?
            if (op)
            {
                guiOp->loadFromXml(xmlOp);
                scene()->addItem(guiOp);
                pastedOps.append(op);
                pastedOpIds.append(opId);
            }
            else
                eDelete(guiOp);
        }

        xmlOp = xmlOp.nextSiblingElement("operator");
    }

    // if two operators from within the set of pasted
    // operators are referencing each other, fix those
    //references (they break as new operators get new IDs)
    for (eU32 i=0; i<pastedOps.size(); i++)
    {
        for (eU32 k=0; k<pastedOps[i]->getParameterCount(); k++)
        {
            eParameter &p = pastedOps[i]->getParameter(k);
            eParamValue &val = p.getBaseValue();

            if (p.getType() == ePT_LINK)
                for (eU32 j=0; j<pastedOps.size(); j++)
                    if (i != j && pastedOpIds[j] == val.linkedOpId)
                        val.linkedOpId = pastedOps[j]->getId();
        }
    }

    // reconnect page and repaint
    eDemoData::connectPages();
    scene()->invalidate();
    QApplication::restoreOverrideCursor();
}

void ePageView::_onPathOperatorEdit()
{
    eASSERT(scene());

    if (scene()->selectedItems().size() == 1)
    {
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->selectedItems().at(0));
        eASSERT(guiOp);
        eIOperator *op = guiOp->getOperator();

        if (op->getResultClass() == eOC_PATH)
            Q_EMIT onPathOperatorEdit(op);
    }
}

void ePageView::_onDemoOperatorEdit()
{
    eASSERT(scene());

    if (scene()->selectedItems().size() == 1)
    {
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->selectedItems().at(0));
        eASSERT(guiOp);
        eIOperator *op = guiOp->getOperator();

        if (op->getResultClass() == eOC_DEMO)
            Q_EMIT onDemoOperatorEdit(op);
    }
}

void ePageView::_onLoadOperatorGoto()
{
    eASSERT(scene());

    if (scene()->selectedItems().size() == 1)
    {
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->selectedItems().at(0));
        eASSERT(guiOp);
        eIOperator *op = guiOp->getOperator();

        if (op->getMetaInfos().name == "Load")
            Q_EMIT onGotoOperator(op->getParameter(0).getBaseValue().linkedOpId);
    }
}

void ePageView::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawForeground(painter, rect);
    if (!scene())
        return;

    painter->save();
    painter->setPen(palette().highlight().color());

    if (m_moving)
    {
        Q_FOREACH(QGraphicsItem *item, scene()->selectedItems())
        {
            const eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(item);
            eASSERT(guiOp);
            const QPoint &newPos = m_mouseMoveDist;
            const ePoint &oldPos = guiOp->getOperator()->getPosition();
            const eF32 x = (eF32)((newPos.x()/eGUIOP_WIDTH+oldPos.x)*eGUIOP_WIDTH);
            const eF32 y = (eF32)((newPos.y()/eGUIOP_HEIGHT+oldPos.y)*eGUIOP_HEIGHT);

            QRectF r = guiOp->boundingRect();
            r.moveTopLeft(QPointF(x, y));
            painter->drawRect(r);
        }
    }
    else if (m_connecting) // connect operators
    {
        const QPointF mousePos = mapToScene(mapFromGlobal(QCursor::pos()));
        QRectF r0, r1;
        r0.setTopLeft(QPointF(m_mouseDownPos.x()/eGUIOP_WIDTH*eGUIOP_WIDTH, m_mouseDownPos.y()/eGUIOP_HEIGHT*eGUIOP_HEIGHT));
        r0.setSize(QSizeF(4*eGUIOP_WIDTH, eGUIOP_HEIGHT-1));
        r1.setTopLeft(QPointF(mousePos.x()/eGUIOP_WIDTH*eGUIOP_WIDTH, mousePos.y()/eGUIOP_HEIGHT*eGUIOP_HEIGHT));
        r1.setSize(QSizeF(4*eGUIOP_WIDTH, eGUIOP_HEIGHT-1));
        painter->drawRect(r0);
        painter->drawRect(r1);
        painter->setPen(QPen(palette().highlight().color(), 1, Qt::DotLine));
        painter->setBrush(palette().highlight().color());
        drawArrow(painter, QLineF(r1.center(), r0.center()), 8, eDegToRad(30.0f));
    }

    painter->restore();
}

void ePageView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, palette().window().color());

    if (m_showGrid)
    {
        const QRect gr = rect.toRect();
        const eInt startX = gr.left()+eGUIOP_WIDTH-(gr.left()%eGUIOP_WIDTH);
        const eInt startY = gr.top()+eGUIOP_HEIGHT-(gr.top()%eGUIOP_HEIGHT);

        QVector<QLineF> lines;

        for (eInt x=startX; x<=gr.right(); x+=eGUIOP_WIDTH)
            lines.append(QLineF(x, gr.top(), x, gr.bottom()));

        for (eInt y=startY; y<=gr.bottom(); y+=eGUIOP_HEIGHT)
            lines.append(QLineF(gr.left(), y, gr.right(), y));

        painter->save();
        painter->setPen(palette().window().color().darker(110));
        painter->drawLines(lines);
        painter->restore();
    }

    // call base class' function in the end,
    // because grid should be drawn behind
    // everything else
    QGraphicsView::drawBackground(painter, rect);
}

void ePageView::mouseDoubleClickEvent(QMouseEvent *de)
{
    QGraphicsView::mouseDoubleClickEvent(de);

    if (scene())
    {
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->itemAt(mapToScene(de->pos()), QTransform()));
        if (guiOp)
        {
            eGuiOperator::setViewingOp(guiOp);
            Q_EMIT onOperatorShow(guiOp->getOperator());
            invalidateScene();
        }
    }
}

void ePageView::mouseMoveEvent(QMouseEvent *me)
{
    if (!scene())
        return;

    if (m_moving || m_connecting)
    {
        m_mouseMoveDist = mapToScene(me->pos()).toPoint()-m_mouseDownPos;
        scene()->invalidate(QRectF(), QGraphicsScene::ForegroundLayer);
    }

    m_connecting = (me->buttons()&Qt::LeftButton && me->modifiers()&Qt::ControlModifier);

    // scroll view
    if (me->buttons() & Qt::LeftButton &&
        !m_moving &&
        !m_connecting &&
        !scene()->selectedItems().size() &&
        !(me->modifiers()&Qt::ControlModifier))
    {
        const QPoint delta = me->pos()-m_lastMousePos;
        m_lastMousePos = me->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());
        setCursor(Qt::ClosedHandCursor);
        me->accept();
    }
    else if (me->buttons()&Qt::RightButton) // modify right button events to left button to allow selection on right button
    {
        QMouseEvent me2(QEvent::MouseMove, me->pos(), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QGraphicsView::mouseMoveEvent(&me2);
    }
    else
        QGraphicsView::mouseMoveEvent(me);
}

void ePageView::mousePressEvent(QMouseEvent *me)
{
    m_mouseDownPos = mapToScene(me->pos()).toPoint();
    m_lastMousePos = me->pos();

    if (me->button() == Qt::LeftButton)
    {
        if (me->modifiers() & Qt::ControlModifier) // connect mode
        {
            setDragMode(QGraphicsView::NoDrag);
            m_connecting = eTRUE;
        }
        else
        {
            QGraphicsView::mousePressEvent(me);

            // on left-click: emit operator-selected signal
            // if there's an operator under mouse
            if (scene())
            {
                eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->itemAt(mapToScene(me->pos()), QTransform()));

                if (guiOp && !guiOp->isResizing())
                {
                    m_moving = eTRUE;
                    eGuiOperator::setEditingOp(guiOp);
                    Q_EMIT onOperatorSelected(guiOp->getOperator());
                }
            }
        }
    }
    else if (me->button() == Qt::RightButton && scene())
    {
        setContextMenuPolicy(Qt::PreventContextMenu);

        // on right-click: unselect all operators
        // which do not lie within selection
        eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(scene()->itemAt(mapToScene(me->pos()), QTransform()));

        if (guiOp)
        {
            if (!scene()->selectedItems().contains(guiOp))
                scene()->clearSelection();

            guiOp->setSelected(true);
            eGuiOperator::setEditingOp(guiOp);
            Q_EMIT onOperatorSelected(guiOp->getOperator());
        }

        QMouseEvent me2(QEvent::MouseButtonPress, me->pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QGraphicsView::mousePressEvent(&me2);
    }

    mouseMoveEvent(me);
}

void ePageView::mouseReleaseEvent(QMouseEvent *me)
{
    QGraphicsView::mouseReleaseEvent(me);
    setCursor(Qt::ArrowCursor);

    if (m_moving) // resize operator
    {
        eGuiOpPage *guiPage = (eGuiOpPage *)scene();
        const ePoint moveDist(m_mouseMoveDist.x()/eGUIOP_WIDTH, m_mouseMoveDist.y()/eGUIOP_HEIGHT);
        eIOpPtrArray opsToMove;

        for (eInt i=0; i<guiPage->selectedItems().size(); i++)
        {
            const eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(guiPage->selectedItems().at(i));
            eASSERT(guiOp);

            if (guiOp->isSelected())
                opsToMove.append(guiOp->getOperator());
        }

        if (guiPage->getPage()->moveOperators(opsToMove, moveDist))
        {
            Q_FOREACH(QGraphicsItem *guiOp, guiPage->selectedItems())
                guiOp->setPos(guiOp->pos().x()+moveDist.x*eGUIOP_WIDTH, guiOp->pos().y()+moveDist.y*eGUIOP_HEIGHT);

            eDemoData::connectPages();
        }

        m_moving = eFALSE;
    }
    else if (me->button() == Qt::LeftButton && me->modifiers()&Qt::ControlModifier) // connect operators
    {
        setDragMode(QGraphicsView::RubberBandDrag); // reenable selection rectangle

        const QPoint mousePos = mapToScene(me->pos()).toPoint();
        eGuiOperator *guiLoadOp = qgraphicsitem_cast<eGuiOperator *>(scene()->itemAt(mousePos, QTransform()));

        if (guiLoadOp && guiLoadOp->getOperator()->getMetaInfos().name != "Load")
        {
            QMessageBox::information(this, "Information", "Empty page or 'Load' operator expected at drag destination!");
            return;
        }

        QByteArray name;
        bool ok = false;

        if (me->modifiers()&Qt::AltModifier)
        {
            name = QInputDialog::getText(this, "Enter name", "Storage name for source operator", QLineEdit::Normal, "", &ok).toLocal8Bit();
            if (!ok)
                return;
        }

        eGuiOperator *guiStoreOp = qgraphicsitem_cast<eGuiOperator *>(scene()->itemAt(m_mouseDownPos, QTransform()));
        
        if (!guiStoreOp)
            guiStoreOp = _addOperator(eOP_TYPE("Misc", "Store"), ePoint(m_mouseDownPos.x()/eGUIOP_WIDTH, m_mouseDownPos.y()/eGUIOP_HEIGHT), eFALSE);
        if (!guiLoadOp)
            guiLoadOp = _addOperator(eOP_TYPE("Misc", "Load"), ePoint(mousePos.x()/eGUIOP_WIDTH, mousePos.y()/eGUIOP_HEIGHT), eFALSE);

        eASSERT(guiStoreOp);
        eASSERT(guiLoadOp);

        eIOperator *loadOp = guiLoadOp->getOperator();
        eIOperator *storeOp = guiStoreOp->getOperator();
        if (ok)
            storeOp->setUserName(name.constData());
        loadOp->getParameter(0).getBaseValue().linkedOpId = storeOp->getId();

        m_connecting = eFALSE;
        eDemoData::connectPages();
    }

    if (me->button() == Qt::RightButton)
    {
        QContextMenuEvent ce(QContextMenuEvent::Mouse, me->pos());
        setContextMenuPolicy(Qt::DefaultContextMenu);
        contextMenuEvent(&ce);
    }

    scene()->invalidate();
}

void ePageView::contextMenuEvent(QContextMenuEvent *ce)
{
    QGraphicsView::contextMenuEvent(ce);

    // show menu only if the mouse was still
    if (scene() && (m_mouseDownPos-mapToScene(ce->pos())).manhattanLength() < 4)
    {
        // show operator's context menu
        QGraphicsItem *item = scene()->itemAt(mapToScene(ce->pos()), QTransform());

        if (item)
        {
            // depending on the type, menu entries
            // might have to be set to invisble
            eGuiOperator *guiOp = qgraphicsitem_cast<eGuiOperator *>(item);
            eASSERT(guiOp);
            eIOperator *op = guiOp->getOperator();
            const eBool oneOpSel = (scene()->selectedItems().size() == 1);

            m_opMenu.actions().at(0)->setVisible(oneOpSel);
            m_opMenu.actions().at(1)->setVisible(oneOpSel && op->getMetaInfos().name == "Load");
            m_opMenu.actions().at(2)->setVisible(oneOpSel && op->getResultClass() == eOC_PATH);
            m_opMenu.actions().at(3)->setVisible(oneOpSel && op->getResultClass() == eOC_DEMO);

            m_opMenu.exec(QCursor::pos());
        }
        else // show view's context menu
            m_viewMenu.exec(QCursor::pos());
    }
}