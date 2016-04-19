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

#ifndef MANAGE_HPP
#define MANAGE_HPP

#include <QtGui/QDialog>

#include "../../eshared/system/system.hpp"
#include "ui_tfmanage.hpp"

class eTfVstSynth;

enum eTfInstrumentAction
{
	TFINSTR_ACTION_NONE = 0,
	TFINSTR_ACTION_RESTORE = 1,
	TFINSTR_ACTION_SAVE = 2,
	TFINSTR_ACTION_COPY = 3
};

class eTfInstrumentTreeWidgetItem : public QTreeWidgetItem
{
public:
	eTfInstrumentTreeWidgetItem(QString name, eU32 instrIndex);

	eU32				getInstrIndex();
	void				setAction(eTfInstrumentAction action);
	eTfInstrumentAction	getAction();
	void				setCopyFrom(eU32 index);
	eU32				getCopyFrom();

private:
	eU32				m_instrIndex;
	eTfInstrumentAction	m_action;
	eU32				m_copyFrom;
};

class eTfManage : public QDialog, protected Ui::Manage
{
    Q_OBJECT

public:
    eTfManage(eTfVstSynth* synth, QWidget *parent=0);

private Q_SLOTS:
	void	_onProgNameChanged(QString name);
    void    _onSelectionChanged(QTreeWidgetItem *item, int column);
	void    _restore(bool checked);
	void    _restoreAll(bool checked);
    void    _save(bool checked);
    void    _saveAll(bool checked);
	void	_removeAction(bool checked);
	void	_removeAllActions(bool checked);

	void	_action(eTfInstrumentAction action);
	void	_actionAll(eTfInstrumentAction action);

	void    _copy(bool checked);
	void    _paste(bool checked);

	void	_cancel(bool checked);
	void	_apply(bool checked);

private:
	void	_saveChanges();

	eTfVstSynth *	m_synth;
	eU32		m_copyFrom;
};

#endif // MANAGE_HPP