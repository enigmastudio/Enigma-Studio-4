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

#ifndef PAGE_TREE_HPP
#define PAGE_TREE_HPP

#include <QtWidgets/QTreeWidget>

#include "../../eshared/eshared.hpp"

class QDomElement;

// encapsulates tree view for pages
class ePageTree : public QTreeWidget
{
    Q_OBJECT

public:
    ePageTree(QWidget *parent);

    QTreeWidgetItem *   addPage(eOperatorPage *opPage, QTreeWidgetItem *parent=nullptr);
    void                selectPage(eOperatorPage *opPage);
    void                updateOpCounts();

    void                saveToXml(QDomElement &parent) const;
    void                loadFromXml(const QDomElement &parent);

public:
    virtual QSize       sizeHint() const;

Q_SIGNALS:
    void                onPageAdd(eOperatorPage *&opPage);
    void                onPageRemove(eOperatorPage *opPage);
    void                onPageSwitch(eOperatorPage *opPage);
    void                onPageRenamed(eOperatorPage *opPage);

private Q_SLOTS:
    void                _onItemChanged(QTreeWidgetItem *item, int column);
    void                _onSelectionChanged();
    void                _onSortByName();
    void                _onRemovePage();
    void                _onRenamePage();
    void                _onAddPage();

private:
    void                _saveItemToXml(const QTreeWidgetItem *item, QDomElement &node) const;
    void                _loadItemFromXml(QTreeWidgetItem *parent, const QDomElement &node);
    void                _createActions();
    void                _makeConnections();
};

#endif // PAGE_TREE_HPP