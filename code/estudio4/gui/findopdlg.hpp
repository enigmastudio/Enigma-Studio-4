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

#ifndef FIND_OP_DLG_HPP
#define FIND_OP_DLG_HPP

#include <QtWidgets/QDialog>

#include "ui_findopdlg.hpp"
#include "../../eshared/eshared.hpp"

class eFindOpDlg : public QDialog, protected Ui::FindOpDlg
{
    Q_OBJECT

public:
    eFindOpDlg(QWidget *parent);

Q_SIGNALS:
    void        onGotoOperator(eID opId);

private:
    void        _makeConnections();
    void        _createOpItem(const eIOperator *op, QList<QTreeWidgetItem *> &items) const;

private Q_SLOTS:
    void        _onOpTreeSelChanged();
    void        _onOpTreeDoubleClick(QTreeWidgetItem *item);
    void        _onFindClicked();
    void        _onGotoClicked();

private:
    enum FindMode
    {
        FM_BYUSERNAME,
        FM_BYNAME,
        FM_BYCATEGORY,
        FM_BYTYPE,
        FM_REFERENCES
    };
};

#endif // FIND_OP_DLG_HPP