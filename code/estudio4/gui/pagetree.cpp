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

#include <QtXml/QDomDocument>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenu>

#include "pagetree.hpp"

ePageTree::ePageTree(QWidget *parent) : QTreeWidget(parent)
{
    _createActions();
    _makeConnections();
}

// adds a new page with the given name to
// the tree-widget. if parent is null the currently
// selected item is used as parent (if no item is
// selected new item is added as a top level item).
QTreeWidgetItem * ePageTree::addPage(eOperatorPage *opPage, QTreeWidgetItem *parent)
{
    eASSERT(!opPage->getOperatorCount());

    if (!parent && selectedItems().size() == 1)
        parent = selectedItems().first();

    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, QString(opPage->getUserName()));
    item->setText(1, QString::number(opPage->getOperatorCount()));
    item->setData(0, Qt::UserRole, qVariantFromValue<ePtr>(opPage));
    item->setFlags(item->flags()|Qt::ItemIsEditable);
    addTopLevelItem(item);
    
    if (parent)
        parent->setExpanded(true);

    return item;
}

void ePageTree::selectPage(eOperatorPage *opPage)
{
    QTreeWidgetItemIterator iter(this);

    while (*iter)
    {
        QTreeWidgetItem *item = *iter;
        const eOperatorPage *data = (eOperatorPage *)item->data(0, Qt::UserRole).value<ePtr>();

        if (data == opPage)
        {
            item->setSelected(true);
            return;
        }

        iter++;
    }
}

void ePageTree::updateOpCounts()
{
    QTreeWidgetItemIterator iter(this);

    while (*iter)
    {
        QTreeWidgetItem *item = *iter;
        const eOperatorPage *opPage = (eOperatorPage *)item->data(0, Qt::UserRole).value<ePtr>();
        item->setText(1, QString::number(opPage->getOperatorCount()));
        iter++;
    }
}

void ePageTree::saveToXml(QDomElement &node) const
{
    QDomDocument &xml = node.ownerDocument();
    eASSERT(!xml.isNull());
    QDomElement tv = xml.createElement("pagetree");
    node.appendChild(tv);

    for (eInt i=0; i<topLevelItemCount(); i++)
        _saveItemToXml(topLevelItem(i), tv);
}

void ePageTree::loadFromXml(const QDomElement &node)
{
    _loadItemFromXml(invisibleRootItem(), node.firstChildElement("pagetree"));
}

QSize ePageTree::sizeHint() const
{
    return QSize(150, 0);
}

void ePageTree::_onItemChanged(QTreeWidgetItem *item, int column)
{
    if (selectedItems().size() == 1 && selectedItems().first() == item)
    {
        eOperatorPage *opPage = (eOperatorPage *)item->data(0, Qt::UserRole).value<ePtr>();
        opPage->setUserName(eString(item->text(0).toLocal8Bit().data()));
        Q_EMIT onPageRenamed(opPage);
    }
}

void ePageTree::_onSelectionChanged()
{
    eOperatorPage *opPage = nullptr;
    if (selectedItems().size() > 0)
        opPage = (eOperatorPage *)selectedItems().first()->data(0, Qt::UserRole).value<ePtr>();

    Q_EMIT onPageSwitch(opPage);
}

void ePageTree::_onSortByName()
{
    sortItems(0, Qt::AscendingOrder);
}

// removes the currently selected page and all child pages
void ePageTree::_onRemovePage()
{
    QTreeWidgetItemIterator iter(this);

    while (*iter)
    {
        if ((*iter)->isSelected())
        {
            eOperatorPage *opPage = (eOperatorPage *)(*iter)->data(0, Qt::UserRole).value<ePtr>();
            Q_EMIT onPageRemove(opPage);
            delete *iter; // can't use eDelete here
        }

        iter++;
    }
}

void ePageTree::_onRenamePage()
{
    if (selectedItems().size() == 1)
        editItem(selectedItems().first(), 0);
}

void ePageTree::_onAddPage()
{
    eOperatorPage *opPage = nullptr;
    Q_EMIT onPageAdd(opPage); // emit signal to get new page
    eASSERT(opPage);
    
    QTreeWidgetItem *newItem = addPage(opPage);
    newItem->setData(0, Qt::UserRole, qVariantFromValue<ePtr>(opPage));
}

void ePageTree::_saveItemToXml(const QTreeWidgetItem *item, QDomElement &node) const
{
    QDomDocument &xml = node.ownerDocument();
    eASSERT(!xml.isNull());

    const eOperatorPage *opPage = (eOperatorPage *)item->data(0, Qt::UserRole).value<ePtr>();

    QDomElement pageEl = xml.createElement("item");
    pageEl.setAttribute("name", item->text(0));
    pageEl.setAttribute("pageid", opPage->getId());
    pageEl.setAttribute("expanded", item->isExpanded());
    node.appendChild(pageEl);

    for (eInt i=0; i<item->childCount(); i++)
        _saveItemToXml(item->child(i), pageEl);
}

void ePageTree::_loadItemFromXml(QTreeWidgetItem *parent, const QDomElement &node)
{
    QDomElement xmlItem = node.firstChildElement("item");

    while (!xmlItem.isNull())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        const eID pageId = xmlItem.attribute("pageid").toInt();
        eOperatorPage *opPage = eDemoData::getPageById(pageId);

        item->setText(0, xmlItem.attribute("name"));
        item->setText(1, QString::number(opPage->getOperatorCount()));
        item->setExpanded(xmlItem.attribute("expanded").toInt());
        item->setData(0, Qt::UserRole, qVariantFromValue<ePtr>(opPage));
        item->setFlags(item->flags()|Qt::ItemIsEditable);

        _loadItemFromXml(item, xmlItem);
        xmlItem = xmlItem.nextSiblingElement("item");
    }
}

void ePageTree::_createActions()
{
    QAction *act = new QAction("Add new", this);
    act->setShortcut(QKeySequence("a"));
    act->setShortcutContext(Qt::WidgetShortcut);
    connect(act, SIGNAL(triggered()), this, SLOT(_onAddPage()));
    addAction(act);

    act = new QAction("Remove", this);
    act->setShortcut(QKeySequence::Delete);
    act->setShortcutContext(Qt::WidgetShortcut);
    connect(act, SIGNAL(triggered()), this, SLOT(_onRemovePage()));
    addAction(act);

    act = new QAction("Rename", this);
    act->setShortcut(QKeySequence("r"));
    act->setShortcutContext(Qt::WidgetShortcut);
    connect(act, SIGNAL(triggered()), this, SLOT(_onRenamePage()));
    addAction(act);

    act = new QAction(this);
    act->setSeparator(true);
    addAction(act);

    act = new QAction("Sort by name", this);
    connect(act, SIGNAL(triggered()), this, SLOT(_onSortByName()));
    addAction(act);
}

void ePageTree::_makeConnections()
{
    connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(_onSelectionChanged()));
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
            this, SLOT(_onItemChanged(QTreeWidgetItem *, int)));
}