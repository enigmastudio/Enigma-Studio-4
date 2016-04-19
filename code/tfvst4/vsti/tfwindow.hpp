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

#include "../qt_winmigrate/qwinwidget.h"
#include "ui_tfwindow.hpp"
#include "tfdial.hpp"

#include "../../eshared/system/system.hpp"
#include "../vstsdk/AudioEffect.hpp"

class eTfVstSynth;

class tfWindow : public QWinWidget, protected Ui::Dialog
{
    Q_OBJECT

public:
    tfWindow(AudioEffect *fx, HWND handle);
	~tfWindow();

    void    initParameters();
    void    setParameter(eU32 index, eF32 value);
	void	updateFreqView();

private:
    eU32    toIndex(eF32 value, eU32 min, eU32 max);
    eF32    fromIndex(eU32 value, eU32 min, eU32 max);

private Q_SLOTS:
    void    onChanged(int value);
    void    onChanged(double value);
	void	onClicked(bool checked);
    void    progChanged(int value);
    void    progRestore(bool checked);
    void    progSave(bool checked);
	void	manage(bool checked);
	void	recordStart(bool checked);
	void	recordStop(bool checked);

	void	_setPresetValue(eU32 index, eF32 value);
    
private:
	void	_createIcons();
	void	_freeIcons();
    void    _updateInstrSelection(bool updateText);

    AudioEffect *   effect;
	eTfVstSynth *		m_synth;
    HWND            m_parent;

	QPixmap *		m_pmSine;
	QPixmap	*		m_pmSawUp;
	QPixmap	*		m_pmSawDown;
	QPixmap	*		m_pmPulse;
	QPixmap	*		m_pmNoise;
	QIcon *			m_icoSine;
	QIcon *			m_icoSawUp;
	QIcon *			m_icoSawDown;
	QIcon *			m_icoPulse;
	QIcon *			m_icoNoise;
};