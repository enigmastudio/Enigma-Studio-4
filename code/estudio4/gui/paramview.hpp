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

#ifndef PARAM_FRAME_HPP
#define PARAM_FRAME_HPP

#include <QtWidgets/QScrollArea>

#include "../../eshared/eshared.hpp"

class QVBoxLayout;
class QLineEdit;

class eParameterView : public QScrollArea
{
    Q_OBJECT

public:
    eParameterView(QWidget *parent=nullptr);
    virtual ~eParameterView();

    void            setOperator(eIOperator *op);
    eIOperator *    getOperator() const;

Q_SIGNALS:
    void            onOperatorChanged(eIOperator *op, const eParameter *param);
    void            onOperatorGoto(eID opId);

private:
    void            _createWidgets();
    void            _clearWidgets(QLayout *layout=nullptr);

    void            _addDefParameterWidgets();
    void            _addOpParameterWidgets();
    void            _addOpErrorStateWidgets();
    void            _addScriptEditor();
    void            _addDoubleSeparators();
    void            _addLabeledSeparator(const QString &label);

private Q_SLOTS:
    void            _onDefaultNameClicked();
    void            _onBypassClicked(bool checked);
    void            _onHideClicked(bool checked);
    void            _onNameChanged(const QString &text);
    void            _onParameterChanged(const eParameter &param);

private:
    eIOperator *    m_op;
    QVBoxLayout *   m_layout;
    QLineEdit *     m_nameEdit;
};

#endif // PARAM_FRAME_HPP