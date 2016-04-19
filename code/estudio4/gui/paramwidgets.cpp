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

#include "paramwidgets.hpp"
#include "linkopdlg.hpp"

#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QIntValidator>
#include <QtGui/QPainter>

#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QMenu>

eParamTrackEdit::eParamTrackEdit(eF32 &baseFlt, const eF32 &animFlt, eParameter &param, QWidget *parent) :
    eTrackEdit(baseFlt, param.getMin(), param.getMax(), parent),
    m_param(param),
    m_refBaseFlt(&baseFlt),
    m_refAnimFlt(&animFlt),
    m_refBaseInt(nullptr),
    m_refAnimInt(nullptr),
    m_refBaseByte(nullptr),
    m_refAnimByte(nullptr),
    m_indEnabled(eFALSE)
{
    eASSERT(param.getClass() == ePC_FLT);
    connect(this, SIGNAL(onValueChanged()), this, SLOT(_onValueChanged()));
}

eParamTrackEdit::eParamTrackEdit(eInt &baseInt, const eInt &animInt, eParameter &param, QWidget *parent) :
    eTrackEdit(baseInt, (eInt)param.getMin(), (eInt)param.getMax(), parent),
    m_param(param),
    m_refBaseInt(&baseInt),
    m_refAnimInt(&animInt),
    m_refBaseFlt(nullptr),
    m_refAnimFlt(nullptr),
    m_refBaseByte(nullptr),
    m_refAnimByte(nullptr),
    m_indEnabled(eFALSE)
{
    eASSERT(param.getClass() == ePC_INT);
    connect(this, SIGNAL(onValueChanged()), this, SLOT(_onValueChanged()));
}

eParamTrackEdit::eParamTrackEdit(eU8 &baseByte, const eU8 &animByte, eParameter &param, QWidget *parent) :
    eTrackEdit(baseByte, (eInt)param.getMin(), (eInt)param.getMax(), parent),
    m_param(param),
    m_refBaseByte(&baseByte),
    m_refAnimByte(&animByte),
    m_refBaseFlt(nullptr),
    m_refAnimFlt(nullptr),
    m_refBaseInt(nullptr),
    m_refAnimInt(nullptr),
    m_indEnabled(eFALSE)
{
    eASSERT(param.getClass() == ePC_COL);
    connect(this, SIGNAL(onValueChanged()), this, SLOT(_onValueChanged()));
}

void eParamTrackEdit::setIndicatorColor(const QColor &indCol)
{
    m_indCol = indCol;
    m_indEnabled = eTRUE;
    repaint();
}

void eParamTrackEdit::setIndicatorEnabled(eBool indEnabled)
{
    m_indEnabled = indEnabled;
    repaint();
}

void eParamTrackEdit::timerEvent(QTimerEvent *te)
{
    eTrackEdit::timerEvent(te);

    if (m_refAnimFlt && !eAreFloatsEqual(*m_refBaseFlt, *m_refAnimFlt))
    {
        m_text = QString::number(*m_refAnimFlt, 'f', FLOAT_PRECISION);
        repaint();
    }
    else if (m_refBaseInt && *m_refBaseInt != *m_refAnimInt)
    {
        m_text = eIntToStr(*m_refAnimInt);
        repaint();
    }
    else if (m_refBaseByte && *m_refBaseByte != *m_refAnimByte)
    {
        m_text = eIntToStr(*m_refAnimByte);
        repaint();
    }
}

void eParamTrackEdit::paintEvent(QPaintEvent *pe)
{
    eTrackEdit::paintEvent(pe);

    QPainter p(this);
    p.setPen(QPen(palette().color(QPalette::Disabled, QPalette::Text)));
    p.drawText(rect().adjusted(0, 0, -4, 0), Qt::AlignVCenter|Qt::AlignRight, m_text);

    if (m_indEnabled)
    {
        const QFontMetrics &fm = fontMetrics();
        const eInt left = fm.boundingRect(text()).width()+9;
        const eInt right = width()-(m_text == "" ? 5 : fm.boundingRect(m_text).width()+9);

        p.setPen(m_indCol);
        p.setBrush(m_indCol);
        p.drawRect(left, 4, right-left, height()-9);
    }
}

void eParamTrackEdit::_onValueChanged()
{
    m_param.setChanged();
    Q_EMIT onParameterChanged(m_param);
}

eBoolButton::eBoolButton(eParameter &param, QWidget *parent) : QToolButton(parent),
    m_param(param)
{
    eASSERT(param.getType() == ePT_BOOL);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    _updateCaption();
    connect(this, SIGNAL(clicked()), this, SLOT(_onClicked()));
}

void eBoolButton::_updateCaption()
{
    setText(m_param.getBaseValue().boolean ? "Yes" : "No");
}

void eBoolButton::_onClicked()
{
    m_param.getBaseValue().boolean = !m_param.getBaseValue().boolean;
    _updateCaption();
    m_param.setChanged();
    Q_EMIT onParameterChanged(m_param);
}

eComboBox::eComboBox(eParameter &param, QWidget *parent) : QComboBox(parent),
    m_param(param)
{
    const QStringList descrItems = QString(param.getDescription()).split("|");
    eASSERT(descrItems.size() > 0);
    eASSERT(param.getBaseValue().enumSel < descrItems.size());

    for (eInt i=0; i<descrItems.size(); i++)
        addItem(descrItems[i]);

    setCurrentIndex(param.getBaseValue().enumSel);
    connect(this, SIGNAL(activated(int)), this, SLOT(_onActivated(int)));
}

void eComboBox::_onActivated(int index)
{
    m_param.getBaseValue().enumSel = index;
    m_param.setChanged();
    Q_EMIT onParameterChanged(m_param);
}

eFlagButton::eFlagButton(eParameter &param, const QString &caption, eU32 flagIndex, QWidget *parent) : QToolButton(parent),
    m_param(param),
    m_flagIndex(flagIndex)
{
    eASSERT(param.getType() == ePT_FLAGS);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setText(caption);
    setCheckable(true);
    _updateDownState();

    connect(this, SIGNAL(clicked()), this, SLOT(_onClicked()));
}

void eFlagButton::_updateDownState()
{
    setChecked(eGetBit(m_param.getBaseValue().flags, m_flagIndex));
}

void eFlagButton::_onClicked()
{
    eToggleBit(m_param.getBaseValue().flags, m_flagIndex);
    _updateDownState();
    m_param.setChanged();
    Q_EMIT onParameterChanged(m_param);
}

eTextEdit::eTextEdit(eParameter &param, QWidget *parent) :
    m_param(param)
{
    eASSERT(param.getType() == ePT_TEXT);

    setFixedHeight(135);
    setPlainText(QString(m_param.getBaseValue().string));

    connect(this, SIGNAL(textChanged()), this, SLOT(_onTextChanged()));
}

void eTextEdit::_onTextChanged()
{
    m_param.getBaseValue().string = toPlainText().toLocal8Bit().constData();
    m_param.setChanged();
    Q_EMIT onParameterChanged(m_param);
}

eLineEdit::eLineEdit(eParameter &param, QWidget *parent) :
    m_param(param)
{
    eASSERT(param.getType() == ePT_STR ||
            param.getType() == ePT_FILE);

    setText(QString(m_param.getBaseValue().string));

    connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(_onTextChanged(const QString &)));
}

void eLineEdit::_onTextChanged(const QString &text)
{
    m_param.getBaseValue().string = text.toLocal8Bit().constData();
    m_param.setChanged();
    Q_EMIT onParameterChanged(m_param);
}

eLinkFrame::eLinkFrame(eParameter &param, QWidget *parent) :
    m_param(&param)
{
    _createWidgets();

    const eIOperator *linkedOp = eDemoData::findOperator(param.getBaseValue().linkedOpId);
    m_edit->setText(linkedOp ? QString(linkedOp->getUserName()) : "<< Empty >>");
}

void eLinkFrame::_createWidgets()
{
    m_edit = new QLineEdit;
    m_edit->setReadOnly(true);

    QToolButton *clearBtn = new QToolButton(this);
    clearBtn->setText("Clear");
    clearBtn->setToolTip("Clears currently linked operator");
    connect(clearBtn, SIGNAL(clicked()), this, SLOT(_onClearClicked()));

    QToolButton *selGlobBtn = new QToolButton(this);
    selGlobBtn->setText("Find");
    selGlobBtn->setToolTip("Select globally (from any page)");
    connect(selGlobBtn, SIGNAL(clicked()), this, SLOT(_onSelectGlobally()));

    m_selLocBtn = new QToolButton(this);
    m_selLocBtn->setText("Pick");
    m_selLocBtn->setToolTip("Select locally (from this page)");
    connect(m_selLocBtn, SIGNAL(clicked()), this, SLOT(_onSelectLocally()));

    QToolButton *gotoBtn = new QToolButton(this);
    gotoBtn->setText("Goto");
    gotoBtn->setToolTip("Goto linked operator");
    connect(gotoBtn, SIGNAL(clicked()), this, SLOT(_onGotoClicked()));

    QHBoxLayout *hbl = new QHBoxLayout;
    hbl->setContentsMargins(0, 0, 0, 0);
    hbl->addWidget(m_edit);
    hbl->addWidget(clearBtn);
    hbl->addWidget(selGlobBtn);
    hbl->addWidget(m_selLocBtn);
    hbl->addWidget(gotoBtn);

    setLayout(hbl);
}

void eLinkFrame::_onSelectGlobally()
{
    eLinkOpDlg dlg(m_param->getAllowedLinks());

    if (dlg.exec() == QDialog::Accepted)
        _linkOperatorWithId(dlg.getSelectedOpId());
}

void eLinkFrame::_onSelectLocally()
{
    // find valid operators for menu
    const eOperatorPage *page = m_param->getOwnerOp()->getOwnerPage();
    eIOpPtrArray ops;

    for (eU32 i=0; i<page->getOperatorCount(); i++)
    {
        eIOperator *op = page->getOperatorByIndex(i);
        
        if (m_param->getAllowedLinks()&op->getResultClass())
            if (op->getMetaInfos().name != "Load" && op->getUserName() != "")
                ops.append(op);
    }

    // sort and add them
    ops.sort(_sortOpsByCategoryAndUserName);

    eInt lastOpc = -1;
    QMenu menu;

    for (eU32 i=0; i<ops.size(); i++)
    {
        if (lastOpc != ops[i]->getResultClass())
        {
            QLabel *lbl = new QLabel(QString(ops[i]->getResultMetaInfos().category));
            QFrame *sep0 = new QFrame;
            QFrame *sep1 = new QFrame;
            sep0->setFrameStyle(QFrame::HLine|QFrame::Sunken);
            sep1->setFrameStyle(QFrame::HLine|QFrame::Sunken);
            QHBoxLayout *hbl = new QHBoxLayout;
            hbl->addWidget(sep0);
            hbl->addWidget(lbl);
            hbl->addWidget(sep1);
            QWidget *sep = new QWidget;
            sep->setLayout(hbl);
            
            QWidgetAction *wa = new QWidgetAction(&menu);
            wa->setDefaultWidget(sep);
            menu.addAction(wa);
        }

        menu.addAction(QString(ops[i]->getUserName()))->setData(ops[i]->getId());
        lastOpc = ops[i]->getResultClass();
    }

    // show menu below select-button
    if (menu.actions().size() > 0)
    {
        QAction *act = menu.exec(mapToGlobal(m_selLocBtn->geometry().bottomLeft()));
        if (act)
            _linkOperatorWithId(act->data().toInt());
    }
}

void eLinkFrame::_onClearClicked()
{
    _linkOperatorWithId(eNOID);
}

void eLinkFrame::_onGotoClicked()
{
    const eID opId = m_param->getBaseValue().linkedOpId;
    Q_EMIT onOperatorGoto(opId);
}

void eLinkFrame::_linkOperatorWithId(eID opId)
{
    m_param->getBaseValue().linkedOpId = opId;
    m_param->setChanged(eTRUE);
    Q_EMIT onParameterChanged(*m_param);

    const eIOperator *linkedOp = eDemoData::findOperator(opId);
    m_edit->setText(linkedOp ? QString(linkedOp->getUserName()) : "<< Empty >>");
}

eBool eLinkFrame::_sortOpsByCategoryAndUserName(eIOperator * const &op0, eIOperator * const &op1)
{
    const QString un0 = op0->getUserName();
    const QString un1 = op1->getUserName();
    const eOpClass opc0 = op0->getResultClass();
    const eOpClass opc1 = op1->getResultClass();

    if (opc0 < opc1)
        return true;
    else if (opc0 > opc1)
        return false;
    else
        return (un0.compare(un1, Qt::CaseInsensitive) > 0);
}

eColorFrame::eColorFrame(eParameter &param, QWidget *parent) : QWidget(parent),
    m_param(param)
{
    eASSERT(param.getType() == ePT_RGB || param.getType() == ePT_RGBA);

    QHBoxLayout *hbl = new QHBoxLayout;
    hbl->setContentsMargins(0, 0, 0, 0);

    for (eU32 i=0; i<param.getComponentCount(); i++)
    {
        m_edits[i] = new eParamTrackEdit(param.getBaseValue().color[i], param.getAnimValue().color[i], param);
        hbl->addWidget(m_edits[i]);
        connect(m_edits[i], SIGNAL(onParameterChanged(const eParameter &)), this, SLOT(_updateEditColors()));
        connect(m_edits[i], SIGNAL(onParameterChanged(const eParameter &)), this, SIGNAL(onParameterChanged(const eParameter &)));
    }

    QToolButton *selBtn = new QToolButton(this);
    selBtn->setText("...");
    selBtn->setFixedWidth(25);
    hbl->addWidget(selBtn);
    connect(selBtn, SIGNAL(clicked()), this, SLOT(_onSelectLocally()));

    setLayout(hbl);
    _updateEditColors();
    m_timerId = startTimer(50);
}

eColorFrame::~eColorFrame()
{
    killTimer(m_timerId);
}

void eColorFrame::timerEvent(QTimerEvent *te)
{
    QWidget::timerEvent(te);
    _updateEditColors();
}

void eColorFrame::_onSelectLocally()
{
    eColor &pc = m_param.getBaseValue().color;
    const QColor col = QColorDialog::getColor(QColor(pc.toArgb()));

    if (col.isValid())
    {
        pc.r = col.red();
        pc.g = col.green();
        pc.b = col.blue();

        m_edits[0]->setText(eIntToStr(pc.r));
        m_edits[1]->setText(eIntToStr(pc.g));
        m_edits[2]->setText(eIntToStr(pc.b));

        m_param.setChanged();
        _updateEditColors();
        Q_EMIT onParameterChanged(m_param);
    }
}

void eColorFrame::_updateEditColors()
{
    for (eU32 i=0; i<m_param.getComponentCount(); i++)
        m_edits[i]->setIndicatorColor(QColor(m_param.getAnimValue().color.toArgb()));
}

eScriptEditor::NumberBar::NumberBar(QTextEdit *textEdit, QWidget *parent) : QWidget(parent),
    m_textEdit(textEdit)
{
    setFixedWidth(fontMetrics().width("000")+6);

    connect(textEdit->document()->documentLayout(), SIGNAL(update(const QRectF &)), this, SLOT(update()));
    connect(textEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(update()));
}

void eScriptEditor::NumberBar::paintEvent(QPaintEvent *pe)
{
    const QTextDocument *doc = m_textEdit->document();
    const eInt contentsY = m_textEdit->verticalScrollBar()->value();
    const eF32 pageBottom = contentsY+m_textEdit->viewport()->height();
    
    eInt lineCount = 1;
    QPainter p(this);

    for (QTextBlock block=doc->begin(); block.isValid(); block=block.next(), lineCount++)
    {
        const QRectF br = doc->documentLayout()->blockBoundingRect(block);
        const eInt blockY = br.topLeft().y();

        if (blockY+br.height() >= contentsY && blockY <= pageBottom)
        {
            const QString text = QString::number(lineCount);
            const eInt x = width()-fontMetrics().width(text)-3;
            const eInt y = (eInt)blockY-contentsY+fontMetrics().ascent()+1;
            p.drawText(x, y, text);
        }
    }
}

eScriptEditor::eScriptEditor(eIOperator *op, QWidget *parent) : QWidget(parent),
    m_op(op)
{
    QFrame *frame = new QFrame(this);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    frame->setFrameStyle(QFrame::Panel|QFrame::Sunken);

    m_srcEdit = new QTextEdit(frame);
    m_srcEdit->setFont(QFont("Courier New", 8, QFont::Normal));
    m_srcEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_srcEdit->setFrameStyle(QFrame::NoFrame);
    m_srcEdit->setPlainText(QString(op->getScript().source));

    m_bcEdit = new QTextEdit(frame);
    m_bcEdit->setFont(QFont("Courier New", 8, QFont::Normal));
    m_bcEdit->setFrameStyle(QFrame::NoFrame);
    m_bcEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_bcEdit->setReadOnly(true);
    m_bcEdit->setTextColor(m_bcEdit->palette().color(QPalette::Disabled, QPalette::Text));
    m_bcEdit->setPlainText(QString(m_sc.disassemble(m_op->getScript())));
    m_bcEdit->setFixedWidth(200);

    m_lblErrors = new QLabel("No errors!");
    m_lblErrors->setFrameStyle(QFrame::Panel|QFrame::Sunken);

    QHBoxLayout *hbl = new QHBoxLayout(frame);
    hbl->setMargin(0);
    hbl->addWidget(new NumberBar(m_srcEdit, this));
    hbl->addWidget(m_srcEdit);
    hbl->addWidget(m_bcEdit);

    QVBoxLayout *vbl = new QVBoxLayout(this);
    vbl->setMargin(0);
    vbl->setSpacing(0);
    vbl->addWidget(frame);
    vbl->addWidget(m_lblErrors);

    connect(m_srcEdit, SIGNAL(textChanged()), this, SLOT(_onScriptChanged()));
    _updateErrors();
}

void eScriptEditor::_onScriptChanged()
{
    // compile script sets operator to changed
    const eString &errors = m_op->compileScript(m_srcEdit->toPlainText().toLocal8Bit().constData());
    _updateErrors();
    m_bcEdit->setPlainText(QString(m_sc.disassemble(m_op->getScript())));
    Q_EMIT onOperatorChanged(m_op, nullptr);
}

void eScriptEditor::_updateErrors()
{
    m_lblErrors->setText(QString(m_op->getScript().errors));
    m_lblErrors->setVisible(m_op->getScript().errors != "");
}