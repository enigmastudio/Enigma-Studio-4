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

#include <QtGui/QMessageBox>

#include "tfvsti.hpp"

eTfInstrumentTreeWidgetItem::eTfInstrumentTreeWidgetItem(QString name, eU32 instrIndex) : QTreeWidgetItem(QStringList(name)),
	m_instrIndex(instrIndex),
	m_action(TFINSTR_ACTION_NONE),
	m_copyFrom(0)
{

}

eU32 eTfInstrumentTreeWidgetItem::getInstrIndex()
{
	return m_instrIndex;
}

void eTfInstrumentTreeWidgetItem::setAction(eTfInstrumentAction action)
{
	m_action = action;

	switch(m_action)
	{
		case TFINSTR_ACTION_NONE:		this->setText(1, ""); break;
		case TFINSTR_ACTION_RESTORE:	this->setText(1, "Restore"); break;
		case TFINSTR_ACTION_SAVE:		this->setText(1, "Save"); break;
		case TFINSTR_ACTION_COPY:		
			{
				QString text("Copy from ");
				text.append(QString::number(m_copyFrom));
				this->setText(1, text); 
				break;
			}
	}
}

eTfInstrumentAction eTfInstrumentTreeWidgetItem::getAction()
{
	return m_action;
}

void eTfInstrumentTreeWidgetItem::setCopyFrom(eU32 index)
{
	m_copyFrom = index;
}

eU32 eTfInstrumentTreeWidgetItem::getCopyFrom()
{
	return m_copyFrom;
}

eTfManage::eTfManage(eTfVstSynth* synth, QWidget *parent) : QDialog(parent),
	m_synth(synth)
{
    setupUi(this);

    // Hide context help button in caption bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(m_restore, SIGNAL(clicked(bool)), this, SLOT(_restore(bool)));
	connect(m_restoreAll, SIGNAL(clicked(bool)), this, SLOT(_restoreAll(bool)));
    connect(m_save, SIGNAL(clicked(bool)), this, SLOT(_save(bool)));
    connect(m_saveAll, SIGNAL(clicked(bool)), this, SLOT(_saveAll(bool)));
    connect(m_copy, SIGNAL(clicked(bool)), this, SLOT(_copy(bool)));
    connect(m_paste, SIGNAL(clicked(bool)), this, SLOT(_paste(bool)));
	connect(m_cancel, SIGNAL(clicked(bool)), this, SLOT(_cancel(bool)));
	connect(m_apply, SIGNAL(clicked(bool)), this, SLOT(_apply(bool)));
	connect(m_removeAction, SIGNAL(clicked(bool)), this, SLOT(_removeAction(bool)));
	connect(m_removeAllActions, SIGNAL(clicked(bool)), this, SLOT(_removeAllActions(bool)));
	connect(m_instrName, SIGNAL(textEdited(QString)), this, SLOT(_onProgNameChanged(QString)));
    connect(m_instrList, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(_onSelectionChanged(QTreeWidgetItem*,int)));

	for(eU32 i=0;i<TF_VSTI_NUM_PROGRAMS;i++)
    {
        QString name = eTfGetInstrumentName(*m_synth, i);
		eTfInstrumentTreeWidgetItem *item = new eTfInstrumentTreeWidgetItem(name, i);
	
        m_instrList->addTopLevelItem(item);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    }
}

void eTfManage::_onProgNameChanged(QString name)
{
	for (eS32 i=0;i < m_instrList->selectedItems().count(); i++)
    {
        m_instrList->selectedItems()[i]->setText(0, name);
    }
}

void eTfManage::_onSelectionChanged(QTreeWidgetItem *item, int column)
{
    m_instrName->setText(item->text(0));
}

void eTfManage::_restore(bool checked)
{
	_action(TFINSTR_ACTION_RESTORE);
}

void eTfManage::_restoreAll(bool checked)
{
	_actionAll(TFINSTR_ACTION_RESTORE);
}

void eTfManage::_save(bool checked)
{
	_action(TFINSTR_ACTION_SAVE);
}

void eTfManage::_saveAll(bool checked)
{
	_actionAll(TFINSTR_ACTION_SAVE);
}

void eTfManage::_removeAction(bool checked)
{
	_action(TFINSTR_ACTION_NONE);
}

void eTfManage::_removeAllActions(bool checked)
{
	_actionAll(TFINSTR_ACTION_NONE);
}

void eTfManage::_action(eTfInstrumentAction action)
{
	QList<QTreeWidgetItem*> items = m_instrList->selectedItems();
	for (eS32 i=0; i<items.count(); i++)
	{
		eTfInstrumentTreeWidgetItem *instrItem = (eTfInstrumentTreeWidgetItem*)items[i];
		instrItem->setAction(action);
	}
}

void eTfManage::_actionAll(eTfInstrumentAction action)
{
	for(eU32 i=0;i<TF_VSTI_NUM_PROGRAMS;i++)
	{
		eTfInstrumentTreeWidgetItem *instrItem = (eTfInstrumentTreeWidgetItem *)m_instrList->topLevelItem(i);
		instrItem->setAction(action);
	}
}

void eTfManage::_copy(bool checked)
{
	if (m_instrList->selectedItems().count() != 1)
		QMessageBox::critical(this, "Tunefish", "Select one instrument to copy from!");
	else
	{
		eTfInstrumentTreeWidgetItem *instrItem = (eTfInstrumentTreeWidgetItem*)m_instrList->selectedItems()[0];
		m_copyFrom = instrItem->getInstrIndex();
	}
}

void eTfManage::_paste(bool checked)
{
	QList<QTreeWidgetItem*> items = m_instrList->selectedItems();
	for (eS32 i=0; i<items.count(); i++)
	{
		eTfInstrumentTreeWidgetItem *instrItem = (eTfInstrumentTreeWidgetItem*)items[i];
		instrItem->setCopyFrom(m_copyFrom);
		instrItem->setAction(TFINSTR_ACTION_COPY);
	}
}

void eTfManage::_cancel(bool checked)
{
	accept();
}

void eTfManage::_apply(bool checked)
{
	_saveChanges();
	accept();
}

void eTfManage::_saveChanges()
{ 
	for(eU32 i=0;i<TF_VSTI_NUM_PROGRAMS;i++)
	{
		eTfInstrumentTreeWidgetItem *item = (eTfInstrumentTreeWidgetItem *)m_instrList->topLevelItem(i);
		int instrIndex = item->getInstrIndex();

        QString itemName = item->text(0);
		eTfSetInstrumentName(*m_synth, i, itemName);

		switch(item->getAction())
		{
			case TFINSTR_ACTION_NONE: break;
			case TFINSTR_ACTION_RESTORE: 
                {
                    m_synth->loadProgram(instrIndex);
                    break;
                }
			case TFINSTR_ACTION_SAVE: 
                {
                    m_synth->saveProgram(instrIndex);
                    break;
                }
            case TFINSTR_ACTION_COPY: 
                {
                    eTfSynthProgram prog;
                    m_synth->getProgramData(item->getCopyFrom(), &prog);
                    m_synth->setProgramData(instrIndex, &prog);
                    break;
                }
		}
	}
}
