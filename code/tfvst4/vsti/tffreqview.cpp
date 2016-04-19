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

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

#include "tffreqview.hpp"

eTfFreqView::eTfFreqView(QWidget *parent) : QWidget(parent),
	m_synth(nullptr),
	m_instr(nullptr),
	m_voice(nullptr)
{
	m_voice = new eTfVoice;
}

eTfFreqView::~eTfFreqView()
{
	eDelete(m_voice);
}

void eTfFreqView::setSynth(eTfSynth *synth, eTfInstrument *instr)
{
	m_synth = synth;
	m_instr = instr;
}

void eTfFreqView::paintEvent(QPaintEvent * pe)
{
	const eU32 viewWidth = this->width()-1;
    const eU32 viewHeight = this->height()-1;
	const eU32 halfViewHeight = viewHeight / 2;
    const eU32 quarterViewHeight = viewHeight / 4;

	QPainter painter(this);
	
	QLinearGradient grad(QPointF(0.0f, 0.0f), QPointF(viewWidth, viewHeight));
	grad.setColorAt(0, QColor(20, 20, 20));
	grad.setColorAt(1, QColor(40, 40, 40));
	QBrush backgroundBrush(grad);
	QPen pen(Qt::black);

	painter.setBrush(backgroundBrush);
	painter.setPen(pen);
	painter.drawRect(this->rect());

    if (m_synth == nullptr || m_instr == nullptr)
		return;

	eTfVoiceReset(*m_voice);
	eTfGeneratorUpdate(*m_synth, *m_instr, *m_voice, m_voice->generator);
    eF32 *freqTable = m_voice->generator.freqTable;
    if (eTfGeneratorModulate(*m_synth, *m_instr, *m_voice, m_voice->generator))
        freqTable = m_voice->generator.freqModTable;
    
	QPen penRefLines(QColor(60,60,80));
	QPen penBins(QBrush(Qt::white), 1.0f);
	QPen penSep(Qt::gray, 0.5f, Qt::DashLine);
    QPen penSig(QColor(160,160,180), 1.0f);
    QPen penSigDrv(Qt::white, 1.0f);

	painter.setRenderHint(QPainter::Antialiasing, eFALSE);

	painter.setPen(penRefLines);
	painter.drawLine(0, halfViewHeight, viewWidth, halfViewHeight);

	eF32 next_sep = 0.1f;
	for (eU32 x=0; x<viewWidth; x++)
	{
		eF32 pos = (eF32)x / viewWidth;
        pos *= pos;

		if (pos > next_sep)
		{
			next_sep += 0.1f;
			painter.setPen(penSep);
			painter.drawLine(x, 0, x, viewHeight/2);
		}

		eU32 offset = (eU32)(pos * TF_IFFT_FRAMESIZE);
		eF32 value = freqTable[offset];

		painter.setPen(penBins);
		painter.drawLine(x, halfViewHeight, x, halfViewHeight - (value * halfViewHeight));
	}

    eTfGeneratorFft(*m_synth, IFFT, freqTable);
    eTfGeneratorNormalize(freqTable);

    eF32 drive = m_instr->params[TF_GEN_DRIVE];
    drive *= 32.0f;
    drive += 1.0f;

    eF32 lastValue = 0.0f;
    eF32 lastValueDrv = 0.0f;
    for (eU32 x=0; x<viewWidth; x++)
    {
        eF32 pos = (eF32)x / viewWidth;

        eU32 offset = (eU32)(pos * TF_IFFT_FRAMESIZE);
        eF32 value = freqTable[offset*2];
        eF32 valueDrv = value * drive;

        value = eClamp<eF32>(-1.0f, value, 1.0f);
        valueDrv = eClamp<eF32>(-1.0f, valueDrv, 1.0f);

        painter.setPen(penSig);
        painter.drawLine(x-1, quarterViewHeight*3 - (lastValue * quarterViewHeight), 
                         x,   quarterViewHeight*3 - (value * quarterViewHeight));

        painter.setPen(penSigDrv);
        painter.drawLine(x-1, quarterViewHeight*3 - (lastValueDrv * quarterViewHeight), 
                         x,   quarterViewHeight*3 - (valueDrv * quarterViewHeight));

        lastValue = value;
        lastValueDrv = valueDrv;
    }
}
