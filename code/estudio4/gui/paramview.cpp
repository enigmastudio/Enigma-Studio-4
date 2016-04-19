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

#include <QtCore/QMap>

#include <QtGui/QFontMetrics>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QLabel>

#include "paramview.hpp"
#include "paramwidgets.hpp"

eParameterView::eParameterView(QWidget *parent) : QScrollArea(parent),
    m_op(nullptr),
    m_layout(nullptr),
    m_nameEdit(nullptr)
{
    setWidgetResizable(true);
    setWidget(new QFrame(viewport()));
    m_layout = new QVBoxLayout(widget());
    m_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
}

eParameterView::~eParameterView()
{
    _clearWidgets(m_layout);
}

void eParameterView::setOperator(eIOperator *op)
{
    if (op != m_op)
    {
        m_op = op;
        _clearWidgets(m_layout);
        _createWidgets();
    }
}

eIOperator * eParameterView::getOperator() const
{
    return m_op;
}

void eParameterView::_createWidgets()
{
    if (m_op)
    {
        _addDefParameterWidgets();
        _addDoubleSeparators();
        _addOpParameterWidgets();
        _addLabeledSeparator("Animation script");
        _addScriptEditor();
        _addDoubleSeparators();
        _addOpErrorStateWidgets();
    }
}

// removes all widgets and layouts from main layout
void eParameterView::_clearWidgets(QLayout *layout)
{
    QLayoutItem *item = nullptr;
    
    while ((item = layout->itemAt(0)) != nullptr)
    {
        if (item->layout())
        {
            _clearWidgets(item->layout());
            delete item->layout();
        }
        else if (item->widget())
            delete item->widget();

        layout->removeItem(item);
    }
}

void eParameterView::_addDefParameterWidgets()
{
    QHBoxLayout *hbl = new QHBoxLayout;
    hbl->addWidget(new QLabel(QString("<b>")+m_op->getMetaInfos().name+" : "+m_op->getMetaInfos().category+"</b>"), 0, 0);

    QToolButton *defNameBtn = new QToolButton(widget());
    defNameBtn->setText("Name");
    hbl->addWidget(defNameBtn);

    QToolButton *bypassBtn = new QToolButton(widget());
    bypassBtn->setText("Bypass");
    bypassBtn->setCheckable(true);
    bypassBtn->setChecked(m_op->getBypassed());
    hbl->addWidget(bypassBtn);

    QToolButton *hideBtn = new QToolButton(widget());
    hideBtn->setText("Hide");
    hideBtn->setCheckable(true);
    hideBtn->setChecked(m_op->getHidden());
    hbl->addWidget(hideBtn);

    m_nameEdit = new QLineEdit(QString(m_op->getUserName()), widget());
    hbl->addWidget(m_nameEdit, 1);

    connect(defNameBtn, SIGNAL(clicked()), this, SLOT(_onDefaultNameClicked()));
    connect(bypassBtn, SIGNAL(clicked(bool)), this, SLOT(_onBypassClicked(bool)));
    connect(hideBtn, SIGNAL(clicked(bool)), this, SLOT(_onHideClicked(bool)));
    connect(m_nameEdit, SIGNAL(textEdited(const QString &)), this, SLOT(_onNameChanged(const QString &)));

    m_layout->addLayout(hbl);
}

void eParameterView::_addOpParameterWidgets()
{
    // calculate max description label width
    eInt maxLabelWidth = 0;

    for (eU32 i=0; i<m_op->getParameterCount(); i++)
    {
        const eParameter &p = m_op->getParameter(i);

        // label parameters don't have description label
        if (p.getType() != ePT_LABEL)
        {
            const eInt curWidth = fontMetrics().width(QString(p.getName()));
            maxLabelWidth = eMax(maxLabelWidth, curWidth);
        }
    }

    // create parameter widgets
    for (eU32 i=0; i<m_op->getParameterCount(); i++)
    {
        eParameter &p = m_op->getParameter(i);
        const eParamType pt = p.getType();

        if (p.getType() == ePT_LABEL)
        {
            _addLabeledSeparator(QString(p.getBaseValue().string));
            continue;
        }

        // create horz. layout and add description label
        QLabel *descrLabel = new QLabel(QString(p.getName()));
        descrLabel->setFixedWidth(maxLabelWidth);
        QHBoxLayout *hbl = new QHBoxLayout;
        m_layout->addLayout(hbl);
        hbl->setSpacing(5);
        hbl->addWidget(descrLabel);

        // add layout for parameter widgets
        if (pt == ePT_BOOL)
            hbl->addWidget(new eBoolButton(p));
        else if (pt == ePT_ENUM)
            hbl->addWidget(new eComboBox(p), 1);
        else if (pt == ePT_TEXT)
            hbl->addWidget(new eTextEdit(p), 1);
        else if (pt == ePT_FILE || pt == ePT_STR)
            hbl->addWidget(new eLineEdit(p), 1);
        else if (pt == ePT_RGB || pt == ePT_RGBA)
            hbl->addWidget(new eColorFrame(p), 1);
        else if (pt == ePT_FLAGS)
        {
            const QStringList descrItems = QString(p.getDescription()).split("|");
            for (eInt j=0; j<descrItems.size(); j++)
                hbl->addWidget(new eFlagButton(p, descrItems[j], j), 1);
        }
        else if (pt == ePT_LINK)
        {
            eLinkFrame *linkFrame = new eLinkFrame(p);
            hbl->addWidget(linkFrame, 1);
            connect(linkFrame, SIGNAL(onOperatorGoto(eID)), this, SIGNAL(onOperatorGoto(eID)));
        }
        else if (pt == ePT_PATH)
        {
            QLabel *lbl = new QLabel("Non-editable parameter, use path editor!");
            lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            hbl->addWidget(lbl, 1);
            continue; // no parameter changed connection required
        }
        else
        {
            for (eU32 j=0; j<p.getComponentCount(); j++)
            {
                switch (p.getClass())
                {
                case ePC_FLT:
                    hbl->addWidget(new eParamTrackEdit((&p.getBaseValue().flt)[j], (&p.getAnimValue().flt)[j], p), 1);
                    break;
                case ePC_INT:
                    hbl->addWidget(new eParamTrackEdit((&p.getBaseValue().integer)[j], (&p.getAnimValue().integer)[j], p), 1);
                    break;
                case ePC_COL:
                    hbl->addWidget(new eParamTrackEdit(p.getBaseValue().color[j], p.getAnimValue().color[j], p), 1);
                    break;
                }
            }
        }

        // connection for operator changed signal
        for (eInt j=1; j<hbl->count(); j++)
        {
            connect(hbl->itemAt(j)->widget(),
                    SIGNAL(onParameterChanged(const eParameter &)),
                    this, SLOT(_onParameterChanged(const eParameter &)));
        }
    }
}

class eOpStateLabel : public QLabel
{
public:
    eOpStateLabel(const eIOperator *op, QWidget *parent) : QLabel(parent),
        m_op(op)
    {
        startTimer(100);
    }

    virtual void timerEvent(QTimerEvent *te)
    {
        QLabel::timerEvent(te);

        static const QString ERRORS_STRS[] =
        {
            "OK",
            "Error: Too many operators above!",
            "Error: Not enough operators above!",
            "Error: An operator above is not allowed!",
            "Error: An operator input is erroneous!",
            "Error: There is a cycle in the stack!",
            "Error: A required link is not specified!"
        };

        setStyleSheet(m_op->getError() != eOE_OK ? "color: red;" : "");
        setText(ERRORS_STRS[m_op->getError()]);
    }

private:
    const eIOperator * m_op;
};

void eParameterView::_addOpErrorStateWidgets()
{
    m_layout->addWidget(new eOpStateLabel(m_op, this));
}

void eParameterView::_addScriptEditor()
{
    eScriptEditor *se = new eScriptEditor(m_op, this);
    m_layout->addWidget(se);
    connect(se, SIGNAL(onOperatorChanged(eIOperator *, const eParameter *)),
            this, SIGNAL(onOperatorChanged(eIOperator *, const eParameter *)));
}

void eParameterView::_addDoubleSeparators()
{
    QFrame *sep0 = new QFrame(widget());
    QFrame *sep1 = new QFrame(widget());
    sep0->setFrameStyle(QFrame::HLine|QFrame::Sunken);
    sep1->setFrameStyle(QFrame::HLine|QFrame::Sunken);

    QVBoxLayout *vbl = new QVBoxLayout;
    vbl->setSpacing(1);
    vbl->addWidget(sep0);
    vbl->addWidget(sep1);

    m_layout->addLayout(vbl);
}

void eParameterView::_addLabeledSeparator(const QString &label)
{
    QFrame *sepLeft = new QFrame(widget());
    sepLeft->setFrameStyle(QFrame::HLine|QFrame::Sunken);
    QFrame *sepRight = new QFrame(widget());
    sepRight->setFrameStyle(QFrame::HLine|QFrame::Sunken);

    QHBoxLayout *hbl = new QHBoxLayout;
    hbl->addWidget(sepLeft, 1);
    hbl->addWidget(new QLabel(label), 0);
    hbl->addWidget(sepRight, 1);

    m_layout->addLayout(hbl);
}

void eParameterView::_onDefaultNameClicked()
{
    if (m_op->getUserName() == "")
    {
        // store individual counts for different
        // operator types in a map
        static QMap<eU32, eU32> newNameCounters;
        const eOpMetaInfos &omi = m_op->getMetaInfos();
        const eU32 count = newNameCounters[omi.type]++;
        m_op->setUserName(omi.name+"_"+eIntToStr(count));
    }
    else
        m_op->setUserName("");

    m_nameEdit->setText(QString(m_op->getUserName()));
    Q_EMIT onOperatorChanged(m_op, nullptr);
}

void eParameterView::_onBypassClicked(bool checked)
{
    m_op->setBypassed(checked);
    m_op->setChanged(eTRUE);
    Q_EMIT onOperatorChanged(m_op, nullptr);
}

void eParameterView::_onHideClicked(bool checked)
{
    m_op->setHidden(checked);
    m_op->setChanged(eTRUE);
    Q_EMIT onOperatorChanged(m_op, nullptr);
}

void eParameterView::_onNameChanged(const QString &text)
{
    m_op->setUserName(text.toLatin1().constData());
    Q_EMIT onOperatorChanged(m_op, nullptr);
}

void eParameterView::_onParameterChanged(const eParameter &param)
{
    Q_EMIT onOperatorChanged(m_op, &param);
}