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

#ifndef ADD_OP_DLG_HPP
#define ADD_OP_DLG_HPP

#include <QtCore/QList>
#include <QtCore/QMap>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

#include "../../eshared/eshared.hpp"

class eButtonLabel : public QLabel
{
    Q_OBJECT

public:
    eButtonLabel(const QString &text, const QColor &hoverCol, QWidget *parent);

Q_SIGNALS:
    void                    clicked();

private:
    virtual void            mouseReleaseEvent(QMouseEvent *me);
    virtual void            enterEvent(QEvent *ev);
    virtual void            leaveEvent(QEvent *ev);

private:
    static QLabel *         m_lastLbl;

private:
    QColor                  m_hoverCol;
};

// Dialog used to add new operators.
class eAddOpDlg : public QDialog
{
    Q_OBJECT

public:
    eAddOpDlg(QWidget *parent=nullptr);

    void                    setFilterOp(const eIOperator *filterOp);
    eU32                    getChosenOpType() const;

private:
    virtual void            keyPressEvent(QKeyEvent *ke);
    virtual void            showEvent(QShowEvent *se);
    virtual void            resizeEvent(QResizeEvent *re);

private:
    void                    _setActiveCategory(Qt::Key key);
    void                    _moveDialogIntoScreen();

private Q_SLOTS:
    void                    _onLabelBtnClick(int opType);

private:
    static eBool            _sortMetaInfosByName(const eOpMetaInfos *mi0, const eOpMetaInfos *mi1);

private:
    typedef QList<const eOpMetaInfos *> eOpMetaInfosList;

    struct Column
    {
        eU32                index;
        Qt::Key             hotKey;
        QFrame *            frame;
        QVBoxLayout *       vbl;
        QLabel *            catLabel;
        QList<QFrame *>     btnFrames;
        eOpMetaInfosList    opMis;
    };

private:
    typedef QMap<Qt::Key, QMap<Qt::Key, eU32>> KeyOpTypeMap;
    typedef QMap<QString, Column> CategoryColumnMap;

private:
    eU32                    m_chosenOpType;
    KeyOpTypeMap            m_shortcutMap;
    CategoryColumnMap       m_catColMap;
    Qt::Key                 m_catKey;
};

#endif // ADD_OP_DLG_HPP