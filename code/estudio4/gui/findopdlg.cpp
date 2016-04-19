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

#include <QtWidgets/QHeaderView>

#include "findopdlg.hpp"

eFindOpDlg::eFindOpDlg(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    setWindowFlags(windowFlags()&(~Qt::WindowContextHelpButtonHint)); // hide context help button in caption bar
    _makeConnections();
}

void eFindOpDlg::_makeConnections()
{
    connect(m_findBtn, SIGNAL(clicked()), this, SLOT(_onFindClicked()));
    connect(m_gotoBtn, SIGNAL(clicked()), this, SLOT(_onGotoClicked()));
    connect(m_cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_opTree, SIGNAL(itemSelectionChanged()), this, SLOT(_onOpTreeSelChanged()));
    connect(m_opTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(_onOpTreeDoubleClick(QTreeWidgetItem *)));
}

void eFindOpDlg::_createOpItem(const eIOperator *op, QList<QTreeWidgetItem *> &items) const
{
    QStringList strs;
    strs << QString(op->getOwnerPage()->getUserName()) <<
            QString(op->getMetaInfos().category) <<
            QString(op->getMetaInfos().name) <<
            QString(op->getUserName()) <<
            QString("%1/%2").arg(op->getPosition().x).arg(op->getPosition().y);

    QTreeWidgetItem *item = new QTreeWidgetItem(strs);
    item->setData(0, Qt::UserRole, op->getId());
    items.append(item);

    for (eInt i=0; i<item->columnCount(); i++)
    {
        const eColor &col = op->getMetaInfos().color;
        item->setBackground(i, QColor(col.toArgb()));
        item->setTextColor(i, col.grayScale() > 128 ? Qt::black : Qt::white);
    }
}

void eFindOpDlg::_onOpTreeSelChanged()
{
    if (!m_opTree->selectedItems().empty())
    {
        m_gotoBtn->setEnabled(true);
        Q_EMIT onGotoOperator(m_opTree->selectedItems().at(0)->data(0, Qt::UserRole).toInt());
    }
    else
        m_gotoBtn->setEnabled(false);
}

void eFindOpDlg::_onOpTreeDoubleClick(QTreeWidgetItem *item)
{
    _onGotoClicked();
}

void eFindOpDlg::_onFindClicked()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    const Qt::CaseSensitivity cs = (m_caseSensitiveCb->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    const eInt mode = m_modeCb->currentIndex();
    QList<QTreeWidgetItem *> items;

    for (eU32 i=0; i<eDemoData::getPageCount(); i++)
    {
        const eOperatorPage *opPage = eDemoData::getPageByIndex(i);
        for (eU32 j=0; j<opPage->getOperatorCount(); j++)
        {
            const eIOperator *op = opPage->getOperatorByIndex(j);
            const QString opType = QString(op->getMetaInfos().category+" :: "+op->getMetaInfos().name);
            eBool addOp = eFALSE;

            if (mode == FM_REFERENCES)
            {
                for (eU32 k=0; k<op->getInputOpCount(); k++)
                {
                    const eIOperator *inOp = op->getInputOp(k);
                    if (QString(inOp->getUserName()).contains(m_whatEdit->text(), cs))
                        _createOpItem(op, items);
                }
            }
            else
            {
                const QString texts[] =
                {
                    QString(op->getUserName()),
                    QString(op->getMetaInfos().name),
                    QString(op->getMetaInfos().category),
                    opType
                };

                if (texts[m_modeCb->currentIndex()].contains(m_whatEdit->text(), cs))
                    _createOpItem(op, items);
            }
        }
    }

    m_opTree->clear();
    m_opTree->addTopLevelItems(items);
    QApplication::restoreOverrideCursor();
}

void eFindOpDlg::_onGotoClicked()
{
    Q_EMIT onGotoOperator(m_opTree->selectedItems().at(0)->data(0, Qt::UserRole).toInt());
    accept();
}