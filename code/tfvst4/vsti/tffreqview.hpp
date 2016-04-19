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

#ifndef TF_FREQVIEW_HPP
#define TF_FREQVIEW_HPP

#include <QtGui/QWidget>

#include "../../eshared/system/system.hpp"
#include "../../eshared/math/math.hpp"
#include "../../eshared/synth/tf4.hpp"

class eTfFreqView : public QWidget
{
    Q_OBJECT

public:
    eTfFreqView(QWidget *parent=0);
	~eTfFreqView();

	void            setSynth(eTfSynth *synth, eTfInstrument *instr);

protected:
	virtual void	paintEvent(QPaintEvent * pe);

private:
	eTfSynth *		m_synth;
	eTfInstrument * m_instr;
	eTfVoice *		m_voice;
};

#endif // TF_FREQVIEW_HPP