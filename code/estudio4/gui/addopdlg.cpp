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

#include <QtCore/QSignalMapper>

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QShortcut>

#include "addopdlg.hpp"

QLabel * eButtonLabel::m_lastLbl = nullptr;

eButtonLabel::eButtonLabel(const QString &text, const QColor &hoverCol, QWidget *parent) : QLabel(text, parent),
    m_hoverCol(hoverCol)
{
    eASSERT(parent);
}

void eButtonLabel::mouseReleaseEvent(QMouseEvent *me)
{
    QLabel::mouseReleaseEvent(me);

    if (underMouse())
    {
        setStyleSheet("");
        Q_EMIT clicked();
    }
}

// sets parent's frame color instead of label's color,
// because eventually existing shortcut label's background
// also has to change.
void eButtonLabel::enterEvent(QEvent *ev)
{
    QLabel::enterEvent(ev);

    if (m_lastLbl)
        parentWidget()->setStyleSheet("");

    parentWidget()->setStyleSheet(QString("color: white; background-color: ")+m_hoverCol.name());
    m_lastLbl = this;
}

void eButtonLabel::leaveEvent(QEvent *ev)
{
    QLabel::leaveEvent(ev);
    parentWidget()->setStyleSheet("");
}

eAddOpDlg::eAddOpDlg(QWidget *parent) : QDialog(parent),
    m_chosenOpType(eNOID)
{
    // hide context help button in caption bar
    setWindowFlags(Qt::CustomizeWindowHint|Qt::Dialog);
    setWindowTitle(tr("Add operator"));
    setContentsMargins(0, 0, 0, 0);

    // horizontal layout for category columns
    QHBoxLayout *hbl = new QHBoxLayout(this);
    hbl->setMargin(0);
    hbl->setContentsMargins(0, 0, 0, 0);
    hbl->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(hbl);

    // create signal mappers for button labels
    QSignalMapper *opSigMap = new QSignalMapper(this);
    connect(opSigMap, SIGNAL(mapped(int)), this, SLOT(_onLabelBtnClick(int)));

    // create operators and sort them
    eOpMetaInfosList metaInfos;

    for (eU32 i=0; i<eIOperator::getAllMetaInfos().size(); i++)
        metaInfos.append(eIOperator::getAllMetaInfos()[i]);

    qSort(metaInfos.begin(), metaInfos.end(), _sortMetaInfosByName);

    // create widgets
    for (eU32 i=0, catIndex=0; i<(eU32)metaInfos.size(); i++)
    {
        // check if the given column (category) already exists
        const eOpMetaInfos &mi = *metaInfos[i];

        if (!m_catColMap.contains(QString(mi.category)))
        {
            QFrame *frame = new QFrame(this);
            hbl->addWidget(frame);
            frame->setContentsMargins(0, 0, 0, 0);

            QVBoxLayout *vbl = new QVBoxLayout(frame);
            vbl->setMargin(0);
            vbl->setContentsMargins(0, 0, 0, 0);

            const QString caption = QString("%1\t%2").arg(QString(mi.category)).arg(++catIndex);
            QLabel *catLabel = new QLabel(caption, frame);
            catLabel->setStyleSheet(QString("color: white; background-color: ")+QColor(mi.color.toArgb()).name());
            catLabel->setContentsMargins(4, 2, 4, 2);
            vbl->addWidget(catLabel);
            vbl->addStretch(1);

            // insert column
            Column col;

            col.index    = catIndex;
            col.hotKey   = (Qt::Key)(Qt::Key_0+catIndex);
            col.catLabel = catLabel;
            col.vbl      = vbl;
            col.frame    = frame;

            m_catColMap.insert(QString(mi.category), col);
        }

        // create button label for operator
        Column &col = m_catColMap[QString(mi.category)];

        QFrame *subFrame = new QFrame(col.frame);
        QHBoxLayout *subHbl = new QHBoxLayout(subFrame);
        subHbl->setContentsMargins(0, 0, 0, 0);
        subHbl->setSpacing(0);

        const QColor hoverCol = QColor::fromRgba(mi.color.toArgb());
        eButtonLabel *lblOp = new eButtonLabel(QString(mi.name), hoverCol, subFrame);
        lblOp->setContentsMargins(4, 0, 4, 0);
        lblOp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        subHbl->addWidget(lblOp);
        
        col.vbl->insertWidget(col.vbl->count()-1, subFrame);
        col.btnFrames.append(subFrame);
        col.opMis.append(&mi);

        // connect slot to signal using mapper
        connect(lblOp, SIGNAL(clicked()), opSigMap, SLOT(map()));
        opSigMap->setMapping(lblOp, mi.type);

        // add map entry for shortcut
        if (mi.shortcut != ' ')
        {
            const QString shortcut = QString(mi.shortcut).toUpper();
            eButtonLabel *lblShortcut = new eButtonLabel(shortcut, hoverCol, subFrame);
            lblShortcut->setContentsMargins(4, 0, 4, 0);
            subHbl->addWidget(lblShortcut);

            const Qt::Key catKey = (Qt::Key)(Qt::Key_0+col.index);
            const Qt::Key opKey = (Qt::Key)QString(mi.shortcut).toUpper()[0].toLatin1();
            m_shortcutMap[catKey].insert(opKey, mi.type);
        }
        else
            subHbl->addSpacing(10);
    }

    // set first category as active
    _setActiveCategory(m_catColMap.values().at(0).hotKey);
}

void eAddOpDlg::setFilterOp(const eIOperator *filterOp)
{
    QVector<Qt::Key> visibleCols;

    Q_FOREACH (const Column &col, m_catColMap)
    {
        eBool colVisible = eFALSE;
        eASSERT(col.opMis.size() == col.btnFrames.size());

        for (eInt i=0; i<col.btnFrames.size(); i++)
        {
            // check if input of filtering operator is valid
            eBool catCondOk = eTRUE;
            if (filterOp && !(col.opMis[i]->above[0]&filterOp->getResultClass()))
                catCondOk = eFALSE;

            // check if input count is valid
            const eBool inCountCondOk = ((!filterOp && col.opMis[i]->minAbove == 0) ||
                                         (filterOp && col.opMis[i]->maxAbove > 0));

            // both conditions have to be true
            if (catCondOk && inCountCondOk)
            {
                colVisible = eTRUE;
                col.btnFrames[i]->show();
            }
            else
                col.btnFrames[i]->hide();
        }

        // was column made visible?
        if (colVisible)
        {
            visibleCols.append(col.hotKey);
            col.frame->show();
        }
        else
            col.frame->hide();
    }

    // set active category to first column if previous
    // active category is not visible this time
    eASSERT(visibleCols.size() > 0);

    if (!visibleCols.contains(m_catKey))
        _setActiveCategory(visibleCols.first());
}

eU32 eAddOpDlg::getChosenOpType() const
{
    return m_chosenOpType;
}

void eAddOpDlg::keyPressEvent(QKeyEvent *ke)
{
    QDialog::keyPressEvent(ke);

    const Qt::Key key = (Qt::Key)ke->key();

    if (m_shortcutMap.contains(key))
        _setActiveCategory(key);
    else if (m_shortcutMap[m_catKey].contains(key))
    {
        m_chosenOpType = m_shortcutMap[m_catKey][key];
        accept();
    }
}

void eAddOpDlg::showEvent(QShowEvent *se)
{
    QDialog::showEvent(se);
    _moveDialogIntoScreen();
}

void eAddOpDlg::resizeEvent(QResizeEvent *re)
{
    QDialog::resizeEvent(re);
    _moveDialogIntoScreen();
}

void eAddOpDlg::_setActiveCategory(Qt::Key key)
{
    m_catKey = key;

    const eInt hotCatIndex = key-Qt::Key_1+1;

    Q_FOREACH (const Column &col, m_catColMap)
    {
        QFont font = col.catLabel->font();
        font.setBold(col.index == hotCatIndex ? true : false);
        col.catLabel->setFont(font);
    }
}

void eAddOpDlg::_moveDialogIntoScreen()
{
    // if dialog would only be partially visible move it
    const QPoint screenMax = QApplication::desktop()->availableGeometry(this).bottomRight();
    const QSize dlgSize = frameGeometry().size();
    
    QPoint dlgPos = pos();

    if (dlgPos.y()+dlgSize.height() >= screenMax.y())
        dlgPos.setY(screenMax.y()-dlgSize.height());

    if (dlgPos.x()+dlgSize.width() >= screenMax.x())
        dlgPos.setX(screenMax.x()-dlgSize.width());

    move(dlgPos);
}

void eAddOpDlg::_onLabelBtnClick(int opType)
{
    m_chosenOpType = opType;
    accept();
}

eBool eAddOpDlg::_sortMetaInfosByName(const eOpMetaInfos *mi0, const eOpMetaInfos *mi1)
{
    return (QString(mi0->name).compare(QString(mi1->name), Qt::CaseInsensitive) < 0);
}