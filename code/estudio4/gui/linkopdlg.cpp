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

#include "linkopdlg.hpp"

eLinkOpDlg::eLinkOpDlg(eInt allowedLinks, QWidget *parent) : QDialog(parent),
    m_allowedLinks(allowedLinks)
{
    setupUi(this);
    setWindowFlags(windowFlags()&(~Qt::WindowContextHelpButtonHint)); // hide context help button in caption bar
    m_opTree->header()->setSortIndicator(1, Qt::AscendingOrder); // default sort column is operator name
    _makeConnections();
    _initPageTree();
}

eInt eLinkOpDlg::getAllowedLinks() const
{
    return m_allowedLinks;
}

eID eLinkOpDlg::getSelectedOpId() const
{
    if (m_opTree->selectedItems().isEmpty())
        return eNOID;

    return m_opTree->selectedItems()[0]->data(0, Qt::UserRole).toInt();
}

void eLinkOpDlg::_initPageTree()
{
    m_opTree->clear();
    m_pageTree->clear();
    m_pageTree->addTopLevelItem(new QTreeWidgetItem(QStringList("All stored")));
    
    // add "a"-"z" items for alphabetically sorting
    m_pageTree->addTopLevelItem(new QTreeWidgetItem(QStringList("Alphabetically")));

    for (eU32 i=0; i<='Z'-'A'; i++)
        m_pageTree->topLevelItem(1)->addChild(new QTreeWidgetItem(QStringList(QString('A'+i))));

    // add all pages to "by page" item
    QTreeWidgetItem *byPageItem = new QTreeWidgetItem(QStringList("By page"));
    m_pageTree->addTopLevelItem(byPageItem);

    for (eU32 i=0; i<eDemoData::getPageCount(); i++)
    {
        eOperatorPage *page = eDemoData::getPageByIndex(i);
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(QString(page->getUserName())));
        item->setData(0, Qt::UserRole, page->getId());
        m_pageTree->topLevelItem(2)->addChild(item);
    }

    byPageItem->setExpanded(true);

    // add tree item for found operators
    m_pageTree->addTopLevelItem(new QTreeWidgetItem(QStringList("Found")));

    // select first item in page tree
    m_pageTree->topLevelItem(0)->setSelected(true);
    _onPageTreeSelChanged();

    // sort operator list name
    m_opTree->sortItems(m_opTree->header()->sortIndicatorSection(), Qt::AscendingOrder);
}

void eLinkOpDlg::_makeConnections()
{
    connect(m_pageTree, SIGNAL(itemSelectionChanged()), this, SLOT(_onPageTreeSelChanged()));
    connect(m_opTree, SIGNAL(itemSelectionChanged()), this, SLOT(_onOpTreeSelChanged()));
    connect(m_findBtn, SIGNAL(clicked()), this, SLOT(_onFindClicked()));
    connect(m_selectBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_opTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(_onOpTreeDoubleClick(QTreeWidgetItem *)));
}

// adds all stored operators to the list widget
void eLinkOpDlg::_addAllStored()
{
    for (eU32 i=0; i<eDemoData::getPageCount(); i++)
        _addByPage(eDemoData::getPageByIndex(i));
}

void eLinkOpDlg::_addByAlpha(eChar alpha)
{
    for (eU32 i=0; i<eDemoData::getPageCount(); i++)
        _addByPage(eDemoData::getPageByIndex(i), alpha);
}

// adds all stored operators on the page with
// the given prefix to the list widget. if prefix
// is != ' ' the user-name has to begin with
// prefix.
void eLinkOpDlg::_addByPage(const eOperatorPage *page, eChar prefix)
{
    for (eU32 i=0; i<page->getOperatorCount(); i++)
    {
        const eIOperator *op = page->getOperatorByIndex(i);

        // user name must be set and if prefix is first
        // character of user-name has to be equal to prefix
        if (op->getMetaInfos().name != "Load" && op->getUserName() != "")
            if (prefix == ' ' || prefix == QString(op->getUserName()).toUpper()[0])
                _addToOpTree(op);
    }
}

// adds all operators to the list widget which
// user names contain the given string
void eLinkOpDlg::_addFound(const QString &name)
{
    if (name == "")
        return;

    for (eU32 i=0; i<eDemoData::getPageCount(); i++)
    {
        eOperatorPage *page = eDemoData::getPageByIndex(i);
        for (eU32 j=0; j<page->getOperatorCount(); j++)
        {
            // is given name sub-string of user-name?
            const eIOperator *op = page->getOperatorByIndex(j);
            if (op->getMetaInfos().type != eOP_TYPE("Misc", "Load") && QString(op->getUserName()).contains(name))
                _addToOpTree(op);
        }
    }
}

void eLinkOpDlg::_addToOpTree(const eIOperator *op)
{
    if (m_allowedLinks&op->getResultClass())
    {
        QStringList strs;
        strs << QString(op->getOwnerPage()->getUserName()) <<
                QString(op->getUserName()) <<
                QString(op->getResultMetaInfos().category) <<
                QString(op->getResultMetaInfos().name);

        QTreeWidgetItem *item = new QTreeWidgetItem(strs);
        item->setData(0, Qt::UserRole, op->getId());
        m_opTree->addTopLevelItem(item);

        for (eInt i=0; i<item->columnCount(); i++)
        {
            const eColor &col = op->getMetaInfos().color;
            item->setBackground(i, QColor(col.toArgb()));
            item->setTextColor(i, col.grayScale() > 128 ? Qt::black : Qt::white);
        }
    }
}

// add operators to list widget depending
// on selected tree item
void eLinkOpDlg::_onPageTreeSelChanged()
{
    m_opTree->clear();

    if (m_pageTree->topLevelItem(0)->isSelected())
        _addAllStored();
    else if (m_pageTree->currentItem()->parent() == m_pageTree->topLevelItem(1))
        _addByAlpha(m_pageTree->currentItem()->text(0).toLocal8Bit()[0]);
    else if (m_pageTree->topLevelItem(3)->isSelected())
        _addFound(m_nameEdit->text());
    else if (m_pageTree->currentItem()->parent() == m_pageTree->topLevelItem(2))
    {
        const eID pageId = m_pageTree->currentItem()->data(0, Qt::UserRole).toInt();
        _addByPage(eDemoData::getPageById(pageId));
    }
}

// enable select button if selected item
// in operator tree is an operator
void eLinkOpDlg::_onOpTreeSelChanged()
{
    m_selectBtn->setEnabled(!m_opTree->selectedItems().empty());
}

// user can select item by double clicking
void eLinkOpDlg::_onOpTreeDoubleClick(QTreeWidgetItem *item)
{
    if (item)
        accept();
}

void eLinkOpDlg::_onFindClicked()
{
    m_pageTree->setCurrentItem(m_pageTree->topLevelItem(3));
    _onPageTreeSelChanged();
}