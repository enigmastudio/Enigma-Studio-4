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

#ifndef PARAM_WIDGETS_HPP
#define PARAM_WIDGETS_HPP

#include <QtWidgets/QLabel>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QToolButton>

#include "../../eshared/eshared.hpp"
#include "trackedit.hpp"

// track edit for float, integer and byte parameter
class eParamTrackEdit : public eTrackEdit
{
    Q_OBJECT

public:
    eParamTrackEdit(eF32 &baseFlt, const eF32 &animFlt, eParameter &param, QWidget *parent=nullptr);
    eParamTrackEdit(eInt &baseInt, const eInt &animInt, eParameter &param, QWidget *parent=nullptr);
    eParamTrackEdit(eU8 &baseByte, const eU8 &animByte, eParameter &param, QWidget *parent=nullptr);

    void                setIndicatorColor(const QColor &indCol);
    void                setIndicatorEnabled(eBool indEnabled);

    virtual void        timerEvent(QTimerEvent *te);
    virtual void        paintEvent(QPaintEvent *pe);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);

private Q_SLOTS:
    void                _onValueChanged();

private:
    eParameter &        m_param;
    eF32 *              m_refBaseFlt;
    eInt *              m_refBaseInt;
    eU8 *               m_refBaseByte;
    const eF32 *        m_refAnimFlt;
    const eInt *        m_refAnimInt;
    const eU8 *         m_refAnimByte;
    QString             m_text;
    QColor              m_indCol;
    eBool               m_indEnabled;
};

class eBoolButton : public QToolButton
{
    Q_OBJECT

public:
    eBoolButton(eParameter &param, QWidget *parent=nullptr);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);

private:
    void                _updateCaption();

private Q_SLOTS:
    void                _onClicked();

private:
    eParameter &        m_param;
};

class eComboBox : public QComboBox
{
    Q_OBJECT

public:
    eComboBox(eParameter &param, QWidget *parent=nullptr);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);

private Q_SLOTS:
    void                _onActivated(int index);

private:
    eParameter &        m_param;
};

class eFlagButton : public QToolButton
{
    Q_OBJECT

public:
    eFlagButton(eParameter &param, const QString &caption, eU32 flagIndex, QWidget *parent=nullptr);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);

private:
    void                _updateDownState();

private Q_SLOTS:
    void                _onClicked();

private:
    eParameter &        m_param;
    eU32                m_flagIndex;
};

class eTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    eTextEdit(eParameter &param, QWidget *parent=nullptr);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);

private Q_SLOTS:
    void                _onTextChanged();

private:
    eParameter &        m_param;
};

class eLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    eLineEdit(eParameter &param, QWidget *parent=nullptr);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);

private Q_SLOTS:
    void                _onTextChanged(const QString &text);

private:
    eParameter &        m_param;
};

class eLinkFrame : public QWidget
{
    Q_OBJECT

public:
    eLinkFrame(eParameter &param, QWidget *parent=nullptr);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);
    void                onOperatorGoto(eID opId);

private Q_SLOTS:
    void                _onSelectGlobally();
    void                _onSelectLocally();
    void                _onClearClicked();
    void                _onGotoClicked();

private:
    void                _linkOperatorWithId(eID opId);
    void                _createWidgets();

private:
    static eBool        _sortOpsByCategoryAndUserName(eIOperator * const &op0, eIOperator * const &op1);

private:
    eParameter *        m_param;
    QLineEdit *         m_edit;
    QToolButton *       m_selLocBtn;
};

class eColorFrame : public QWidget
{
    Q_OBJECT

public:
    eColorFrame(eParameter &param, QWidget *parent=nullptr);
    virtual ~eColorFrame();

private:
    virtual void        timerEvent(QTimerEvent *te);

Q_SIGNALS:
    void                onParameterChanged(const eParameter &param);

private Q_SLOTS:
    void                _onSelectLocally();
    void                _updateEditColors();

private:
    eInt                m_timerId;
    eParameter &        m_param;
    eParamTrackEdit *   m_edits[4];
};

class eScriptEditor : public QWidget
{
    Q_OBJECT

public:
    eScriptEditor(eIOperator *op, QWidget *parent=nullptr);

Q_SIGNALS:
    void                onOperatorChanged(eIOperator *op, const eParameter *param);

private Q_SLOTS:
    void                _onScriptChanged();

private:
    void                _updateErrors();

private:
    // code taken from KDE libraries
    class NumberBar : public QWidget
    {
    public:
        NumberBar(QTextEdit *textEdit, QWidget *parent);

    private:
        virtual void    paintEvent(QPaintEvent *pe);

    private:
        QTextEdit *     m_textEdit;
    };

private:
    eIOperator *        m_op;
    QTextEdit *         m_srcEdit;
    QTextEdit *         m_bcEdit;
    QLabel *            m_lblErrors;
    NumberBar *         m_numBar;
    eScriptCompiler     m_sc;
};

#endif // PARAM_WIDGETS_HPP