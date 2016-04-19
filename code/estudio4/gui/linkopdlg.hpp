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

#ifndef LINK_OP_DLG_HPP
#define LINK_OP_DLG_HPP

#include <QtWidgets/QDialog>

#include "ui_linkopdlg.hpp"
#include "../../eshared/eshared.hpp"

class eLinkOpDlg : public QDialog, protected Ui::LinkOpDlg
{
    Q_OBJECT

public:
    eLinkOpDlg(eInt allowedLinks, QWidget *parent=nullptr);

    eInt            getAllowedLinks() const;
    eID             getSelectedOpId() const;

private:
    void            _initPageTree();
    void            _makeConnections();

    void            _addAllStored();
    void            _addByAlpha(eChar alpha);
    void            _addByPage(const eOperatorPage *page, eChar prefix=' ');
    void            _addFound(const QString &name);

    void            _addToOpTree(const eIOperator *op);

private Q_SLOTS:    
    void            _onPageTreeSelChanged();
    void            _onOpTreeSelChanged();
    void            _onOpTreeDoubleClick(QTreeWidgetItem *item);
    void            _onFindClicked();

private:
    const eInt      m_allowedLinks;
};

#endif // LINK_OP_DLG_HPP