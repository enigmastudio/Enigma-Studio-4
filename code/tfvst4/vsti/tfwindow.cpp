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
#include <QtGui/QMenu>
#include <QtCore/QFile>
#include <QtGui/QPainter>
#include <QtGui/QFileDialog>

#include "tfvsti.hpp"

tfWindow::tfWindow(AudioEffect *fx, HWND handle) : QWinWidget(handle, NULL)
{
  	QFile cssFile(":/.css/style.css");

    if (cssFile.open(QFile::ReadOnly))
    {
        qApp->setStyleSheet(QString(cssFile.readAll()));
    }

    m_parent = handle;
    effect = fx;
	m_synth = (eTfVstSynth*)effect;
    setAttribute(Qt::WA_DeleteOnClose);
    setupUi(this);
    initParameters();

	eTfSynth *tfsynth = m_synth->getTunefish();
	m_freqView->setSynth(tfsynth, tfsynth->instr[0]);

    _updateInstrSelection(false);

	_createIcons();

	QMenu *presetMenu = new QMenu();
	QAction *actionSine = new QAction("Sine", this);
	QAction *actionTriangle = new QAction("Triangle", this);
	QAction *actionSquare = new QAction("Square", this);
	QAction *actionSawUp = new QAction("Saw up", this);
	QAction *actionSawDown = new QAction("Saw down", this);

	presetMenu->addAction(actionSine);
	presetMenu->addAction(actionTriangle);
	presetMenu->addAction(actionSquare);
	presetMenu->addAction(actionSawUp);
	presetMenu->addAction(actionSawDown);

	connect(actionSine, SIGNAL(triggered(bool)), this, SLOT(presetSine(bool)));
	connect(actionTriangle, SIGNAL(triggered(bool)), this, SLOT(presetTriangle(bool)));
	connect(actionSquare, SIGNAL(triggered(bool)), this, SLOT(presetSquare(bool)));
	connect(actionSawUp, SIGNAL(triggered(bool)), this, SLOT(presetSawUp(bool)));
	connect(actionSawDown, SIGNAL(triggered(bool)), this, SLOT(presetSawDown(bool)));

    connect(m_instrSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(progChanged(int)));
    connect(m_instrRestore, SIGNAL(clicked(bool)), this, SLOT(progRestore(bool)));
    connect(m_instrSave, SIGNAL(clicked(bool)), this, SLOT(progSave(bool)));
	connect(m_manageInstruments, SIGNAL(clicked(bool)), this, SLOT(manage(bool)));
	connect(m_recordStart, SIGNAL(clicked(bool)), this, SLOT(recordStart(bool)));
	connect(m_recordEnd, SIGNAL(clicked(bool)), this, SLOT(recordStop(bool)));

    connect(m_gainAmount, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_oscPolyphony, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_oscUni1, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni2, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni3, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni4, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni5, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni6, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni7, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni8, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni9, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_oscUni10, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
    connect(m_oscFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_oscPanning, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_oscDetune, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_oscSpread, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_oscGlide, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_oscSlop, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_addVolume, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_addSingle, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addDetuned, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addGauss, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addSpread, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
    connect(m_addOctm4, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOctm3, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOctm2, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOctm1, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOct0, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOct1, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOct2, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOct3, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_addOct4, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
    connect(m_addBandwidth, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_addDamp, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_addHarmonics, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_addDrive, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
	connect(m_addMod, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
	connect(m_addScale, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
   
    connect(m_noiseAmount, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_noiseFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_noiseBW, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_lpFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_lpRes, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_lpOn, SIGNAL(stateChanged(int)), this, SLOT(onChanged(int)));

    connect(m_hpFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_hpRes, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_hpOn, SIGNAL(stateChanged(int)), this, SLOT(onChanged(int)));

    connect(m_bpFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_bpQ, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_bpOn, SIGNAL(stateChanged(int)), this, SLOT(onChanged(int)));

    connect(m_ntFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_ntQ, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_ntOn, SIGNAL(stateChanged(int)), this, SLOT(onChanged(int)));

    connect(m_adsr1A, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr1D, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr1S, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr1R, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr1Slope, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_adsr2A, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr2D, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr2S, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr2R, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_adsr2Slope, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_lfo1Rate, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_lfo1Depth, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
	connect(m_lfo1ShapeSine, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo1ShapeSawUp, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo1ShapeSawDown, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo1ShapePulse, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo1ShapeNoise, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
    connect(m_lfo1Sync, SIGNAL(stateChanged(int)), this, SLOT(onChanged(int)));

    connect(m_lfo2Rate, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_lfo2Depth, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
	connect(m_lfo2ShapeSine, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo2ShapeSawUp, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo2ShapeSawDown, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo2ShapePulse, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_lfo2ShapeNoise, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
    connect(m_lfo2Sync, SIGNAL(stateChanged(int)), this, SLOT(onChanged(int)));

    connect(m_mm1Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm1Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm2Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm2Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm3Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm3Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm4Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm4Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm5Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm5Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm6Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm6Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm7Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm7Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm8Src, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_mm8Dest, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));

    connect(m_mm1Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
    connect(m_mm2Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
    connect(m_mm3Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
    connect(m_mm4Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
    connect(m_mm5Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
    connect(m_mm6Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
    connect(m_mm7Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));
    connect(m_mm8Mod, SIGNAL(valueChanged(double)), this, SLOT(onChanged(double)));

    connect(m_effect1, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect2, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect3, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect4, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect5, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect6, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect7, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect8, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect9, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));
    connect(m_effect10, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged(int)));

    connect(m_distAmount, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
  
    connect(m_chorusFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_chorusDepth, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_chorusGain, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_delayLeft, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_delayRight, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_delayDecay, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_revRoomsize, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_revDamp, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_revWet, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_revWidth, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_flangLfo, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_flangFreq, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_flangAmp, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_flangWet, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_formantA, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_formantE, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_formantI, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_formantO, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
	connect(m_formantU, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
    connect(m_formantWet, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));

    connect(m_eqLow, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_eqMid, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
    connect(m_eqHigh, SIGNAL(valueChanged(int)), this, SLOT(onChanged(int)));
}

tfWindow::~tfWindow()
{
	this->close();
	effect = nullptr;
	_freeIcons();
}

void tfWindow::initParameters()
{
    for(eU32 i=0;i<TF_PARAM_COUNT;i++)
    {
        eF32 value = effect->getParameter(i);
        setParameter(i, value);
    }
}

eU32 tfWindow::toIndex(eF32 value, eU32 min, eU32 max)
{
    return (eU32)eRound(value * (max - min)) + min;
}

eF32 tfWindow::fromIndex(eU32 value, eU32 min, eU32 max)
{
    return (eF32)(value - min) / (max - min);
}

void tfWindow::setParameter(eU32 index, eF32 value)
{
    eU32 knob = value*99;

    switch(index)
    {
        case TF_GLOBAL_GAIN:                m_gainAmount->setValue(knob); break;
        case TF_GEN_FREQ:                   m_oscFreq->setValue(knob); break;
        case TF_GEN_PANNING:                m_oscPanning->setValue(knob); break;
        case TF_GEN_DETUNE:                 m_oscDetune->setValue(knob); break;
        case TF_GEN_POLYPHONY:              m_oscPolyphony->setCurrentIndex(toIndex(value, 0, TF_MAXVOICES-1)); break;
        case TF_GEN_UNISONO:                
			{
				switch(toIndex(value, 0, TF_MAXUNISONO-1))
				{
					case 0: m_oscUni1->setChecked(true); break;
					case 1: m_oscUni2->setChecked(true); break;
					case 2: m_oscUni3->setChecked(true); break;
					case 3: m_oscUni4->setChecked(true); break;
					case 4: m_oscUni5->setChecked(true); break;
					case 5: m_oscUni6->setChecked(true); break;
					case 6: m_oscUni7->setChecked(true); break;
					case 7: m_oscUni8->setChecked(true); break;
					case 8: m_oscUni9->setChecked(true); break;
					case 9: m_oscUni10->setChecked(true); break;
				}
				break;
			}
        case TF_GEN_SPREAD:                 m_oscSpread->setValue(knob); break;
        case TF_GEN_GLIDE:                  m_oscGlide->setValue(knob); break;
        case TF_GEN_SLOP:                   m_oscSlop->setValue(knob); break;
        case TF_GEN_VOLUME:                 m_addVolume->setValue(knob); break;
        case TF_GEN_PROFILE:                
			{
				switch (toIndex(value, 0, TF_NUMGENPROFILES-1))
				{
					case 0: m_addSingle->setChecked(true); break;
					case 1: m_addDetuned->setChecked(true); break;
					case 3: m_addGauss->setChecked(true); break;
					case 2: m_addSpread->setChecked(true); break;
				}
				break;
			}
        case TF_GEN_OCTAVE:                 
			{
				switch (toIndex(value, 0, TF_MAXOCTAVES-1))
				{
					case 8:	m_addOctm4->setChecked(true); break;
					case 7: m_addOctm3->setChecked(true); break;
					case 6: m_addOctm2->setChecked(true); break;
					case 5: m_addOctm1->setChecked(true); break;
					case 4: m_addOct0->setChecked(true); break;
					case 3: m_addOct1->setChecked(true); break;
					case 2: m_addOct2->setChecked(true); break;
					case 1: m_addOct3->setChecked(true); break;
					case 0: m_addOct4->setChecked(true); break;
				}
				break;
			}
        case TF_GEN_BANDWIDTH:              m_addBandwidth->setValue(knob); break;
        case TF_GEN_DAMP:                   m_addDamp->setValue(knob); break;
        case TF_GEN_NUMHARMONICS:           m_addHarmonics->setValue(knob); break;
        case TF_GEN_DRIVE:                  m_addDrive->setValue(knob); break;
		case TF_GEN_MODULATION:				m_addMod->setValue(knob); break;
		case TF_GEN_SCALE:					m_addScale->setValue(knob); break;

        case TF_NOISE_AMOUNT:               m_noiseAmount->setValue(knob); break;
        case TF_NOISE_FREQ:                 m_noiseFreq->setValue(knob); break;
        case TF_NOISE_BW:                   m_noiseBW->setValue(knob); break;

        case TF_LP_FILTER_ON:               m_lpOn->setChecked(value > 0.5f); break;
        case TF_LP_FILTER_CUTOFF:           m_lpFreq->setValue(knob); break;
        case TF_LP_FILTER_RESONANCE:        m_lpRes->setValue(knob); break;

        case TF_HP_FILTER_ON:               m_hpOn->setChecked(value > 0.5f); break;
        case TF_HP_FILTER_CUTOFF:           m_hpFreq->setValue(knob); break;
        case TF_HP_FILTER_RESONANCE:        m_hpRes->setValue(knob); break;

        case TF_ADSR1_ATTACK:               m_adsr1A->setValue(knob); break;            
        case TF_ADSR1_DECAY:                m_adsr1D->setValue(knob); break;    
        case TF_ADSR1_SUSTAIN:              m_adsr1S->setValue(knob); break;    
        case TF_ADSR1_RELEASE:              m_adsr1R->setValue(knob); break;    
        case TF_ADSR1_SLOPE:                m_adsr1Slope->setValue(knob); break;    

        case TF_ADSR2_ATTACK:               m_adsr2A->setValue(knob); break;    
        case TF_ADSR2_DECAY:                m_adsr2D->setValue(knob); break;    
        case TF_ADSR2_SUSTAIN:              m_adsr2S->setValue(knob); break;    
        case TF_ADSR2_RELEASE:              m_adsr2R->setValue(knob); break;    
        case TF_ADSR2_SLOPE:                m_adsr2Slope->setValue(knob); break;    
    
        case TF_LFO1_RATE:                  m_lfo1Rate->setValue(knob); break;    
        case TF_LFO1_DEPTH:                 m_lfo1Depth->setValue(knob); break;       
		case TF_LFO1_SHAPE:
			{
				switch (toIndex(value, 0, TF_LFOSHAPECOUNT))
				{
				case 0:	m_lfo1ShapeSine->setChecked(true); break;
				case 1: m_lfo1ShapeSawUp->setChecked(true); break;
				case 2: m_lfo1ShapeSawDown->setChecked(true); break;
				case 3: m_lfo1ShapePulse->setChecked(true); break;
				case 4: m_lfo1ShapeNoise->setChecked(true); break;
				}
				break;
			}
        case TF_LFO1_SYNC:                  m_lfo1Sync->setChecked(value > 0.5f); break;

        case TF_LFO2_RATE:                  m_lfo2Rate->setValue(knob); break;    
        case TF_LFO2_DEPTH:                 m_lfo2Depth->setValue(knob); break;    
        case TF_LFO2_SHAPE:  
			{
				switch (toIndex(value, 0, TF_LFOSHAPECOUNT))
				{
				case 0:	m_lfo2ShapeSine->setChecked(true); break;
				case 1: m_lfo2ShapeSawUp->setChecked(true); break;
				case 2: m_lfo2ShapeSawDown->setChecked(true); break;
				case 3: m_lfo2ShapePulse->setChecked(true); break;
				case 4: m_lfo2ShapeNoise->setChecked(true); break;
				}
				break;
			}
        case TF_LFO2_SYNC:                  m_lfo2Sync->setChecked(value > 0.5f); break;

        case TF_MM1_SOURCE:                 m_mm1Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM1_MOD:                    m_mm1Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM1_TARGET:                 m_mm1Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        case TF_MM2_SOURCE:                 m_mm2Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM2_MOD:                    m_mm2Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM2_TARGET:                 m_mm2Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        case TF_MM3_SOURCE:                 m_mm3Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM3_MOD:                    m_mm3Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM3_TARGET:                 m_mm3Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        case TF_MM4_SOURCE:                 m_mm4Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM4_MOD:                    m_mm4Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM4_TARGET:                 m_mm4Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        case TF_MM5_SOURCE:                 m_mm5Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM5_MOD:                    m_mm5Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM5_TARGET:                 m_mm5Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        case TF_MM6_SOURCE:                 m_mm6Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM6_MOD:                    m_mm6Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM6_TARGET:                 m_mm6Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        case TF_MM7_SOURCE:                 m_mm7Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM7_MOD:                    m_mm7Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM7_TARGET:                 m_mm7Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        case TF_MM8_SOURCE:                 m_mm8Src->setCurrentIndex(toIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); break;
        case TF_MM8_MOD:                    m_mm8Mod->setValue((value - 0.5f) * TF_MM_MODRANGE); break;
        case TF_MM8_TARGET:                 m_mm8Dest->setCurrentIndex(toIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); break;
        
        case TF_EFFECT_1:                   m_effect1->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_2:                   m_effect2->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_3:                   m_effect3->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_4:                   m_effect4->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_5:                   m_effect5->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_6:                   m_effect6->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_7:                   m_effect7->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_8:                   m_effect8->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_9:                   m_effect9->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;
        case TF_EFFECT_10:                  m_effect10->setCurrentIndex(toIndex(value, 0, FX_COUNT-1)); break;

        case TF_DISTORT_AMOUNT:             m_distAmount->setValue(knob); break; 

        case TF_CHORUS_RATE:                m_chorusFreq->setValue(knob); break; 
        case TF_CHORUS_DEPTH:               m_chorusDepth->setValue(knob); break; 
        case TF_CHORUS_GAIN:                m_chorusGain->setValue(knob); break; 
    
        case TF_DELAY_LEFT:                 m_delayLeft->setValue(knob); break; 
        case TF_DELAY_RIGHT:                m_delayRight->setValue(knob); break; 
        case TF_DELAY_DECAY:                m_delayDecay->setValue(knob); break; 

        case TF_REVERB_ROOMSIZE:            m_revRoomsize->setValue(knob); break; 
        case TF_REVERB_DAMP:                m_revDamp->setValue(knob); break; 
        case TF_REVERB_WET:                 m_revWet->setValue(knob); break; 
        case TF_REVERB_WIDTH:               m_revWidth->setValue(knob); break; 

        case TF_FLANGER_LFO:                m_flangLfo->setValue(knob); break; 
        case TF_FLANGER_FREQUENCY:          m_flangFreq->setValue(knob); break; 
        case TF_FLANGER_AMPLITUDE:          m_flangAmp->setValue(knob); break; 
        case TF_FLANGER_WET:                m_flangWet->setValue(knob); break; 

        case TF_FORMANT_MODE:               
			{
				switch (toIndex(value, 0, 4))
				{
					case 0: m_formantA->setChecked(true); break;
					case 1: m_formantE->setChecked(true); break;
					case 2: m_formantI->setChecked(true); break;
					case 3: m_formantO->setChecked(true); break;
					case 4: m_formantU->setChecked(true); break;
				}
			}
        case TF_FORMANT_WET:                m_formantWet->setValue(knob); break; 

        case TF_EQ_LOW:                     m_eqLow->setValue(knob); break; 
        case TF_EQ_MID:                     m_eqMid->setValue(knob); break; 
        case TF_EQ_HIGH:                    m_eqHigh->setValue(knob); break; 
    }
}

void tfWindow::onChanged(double value)
{
    QObject *obj = sender();

    if (obj == m_mm1Mod)            { effect->setParameter(TF_MM1_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
    if (obj == m_mm2Mod)            { effect->setParameter(TF_MM2_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
    if (obj == m_mm3Mod)            { effect->setParameter(TF_MM3_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
    if (obj == m_mm4Mod)            { effect->setParameter(TF_MM4_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
    if (obj == m_mm5Mod)            { effect->setParameter(TF_MM5_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
    if (obj == m_mm6Mod)            { effect->setParameter(TF_MM6_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
    if (obj == m_mm7Mod)            { effect->setParameter(TF_MM7_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
    if (obj == m_mm8Mod)            { effect->setParameter(TF_MM8_MOD, (value / TF_MM_MODRANGE) + 0.5f); return; }
}

void tfWindow::onClicked(bool checked)
{
	QObject *obj = sender();

	if (obj == m_oscUni1)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(0, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni2)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(1, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni3)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(2, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni4)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(3, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni5)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(4, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni6)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(5, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni7)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(6, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni8)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(7, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni9)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(8, 0, TF_MAXUNISONO-1)); return; }
	if (obj == m_oscUni10)			{ effect->setParameter(TF_GEN_UNISONO, fromIndex(9, 0, TF_MAXUNISONO-1)); return; }

    if (obj == m_addOctm4)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(8, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOctm3)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(7, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOctm2)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(6, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOctm1)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(5, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOct0)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(4, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOct1)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(3, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOct2)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(2, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOct3)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(1, 0, TF_MAXOCTAVES-1)); return; }
    if (obj == m_addOct4)			{ effect->setParameter(TF_GEN_OCTAVE, fromIndex(0, 0, TF_MAXOCTAVES-1)); return; }

	if (obj == m_addSingle)         { effect->setParameter(TF_GEN_PROFILE, fromIndex(0, 0, TF_NUMGENPROFILES-1)); return; }
	if (obj == m_addDetuned)        { effect->setParameter(TF_GEN_PROFILE, fromIndex(1, 0, TF_NUMGENPROFILES-1)); return; }
	if (obj == m_addGauss)          { effect->setParameter(TF_GEN_PROFILE, fromIndex(3, 0, TF_NUMGENPROFILES-1)); return; }
	if (obj == m_addSpread)         { effect->setParameter(TF_GEN_PROFILE, fromIndex(2, 0, TF_NUMGENPROFILES-1)); return; }

	if (obj == m_lfo1ShapeSine)     { effect->setParameter(TF_LFO1_SHAPE, fromIndex(0, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo1ShapeSawUp)    { effect->setParameter(TF_LFO1_SHAPE, fromIndex(1, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo1ShapeSawDown)  { effect->setParameter(TF_LFO1_SHAPE, fromIndex(2, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo1ShapePulse)    { effect->setParameter(TF_LFO1_SHAPE, fromIndex(3, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo1ShapeNoise)    { effect->setParameter(TF_LFO1_SHAPE, fromIndex(4, 0, TF_LFOSHAPECOUNT)); return; }

	if (obj == m_lfo2ShapeSine)     { effect->setParameter(TF_LFO2_SHAPE, fromIndex(0, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo2ShapeSawUp)    { effect->setParameter(TF_LFO2_SHAPE, fromIndex(1, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo2ShapeSawDown)  { effect->setParameter(TF_LFO2_SHAPE, fromIndex(2, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo2ShapePulse)    { effect->setParameter(TF_LFO2_SHAPE, fromIndex(3, 0, TF_LFOSHAPECOUNT)); return; }
	if (obj == m_lfo2ShapeNoise)    { effect->setParameter(TF_LFO2_SHAPE, fromIndex(4, 0, TF_LFOSHAPECOUNT)); return; }

	if (obj == m_formantA)          { effect->setParameter(TF_FORMANT_MODE, fromIndex(0, 0, 4)); return; }
	if (obj == m_formantE)          { effect->setParameter(TF_FORMANT_MODE, fromIndex(1, 0, 4)); return; }
	if (obj == m_formantI)          { effect->setParameter(TF_FORMANT_MODE, fromIndex(2, 0, 4)); return; }
	if (obj == m_formantO)          { effect->setParameter(TF_FORMANT_MODE, fromIndex(3, 0, 4)); return; }
	if (obj == m_formantU)          { effect->setParameter(TF_FORMANT_MODE, fromIndex(4, 0, 4)); return; }
}

void tfWindow::onChanged(int value)
{
    QObject *obj = sender();
    eF32 knob = (eF32)value / 99;

    if (obj == m_gainAmount)        { effect->setParameter(TF_GLOBAL_GAIN, knob); return; }
    if (obj == m_oscFreq)           { effect->setParameter(TF_GEN_FREQ, knob); return; }
    if (obj == m_oscPanning)        { effect->setParameter(TF_GEN_PANNING, knob); return; }
    if (obj == m_oscDetune)         { effect->setParameter(TF_GEN_DETUNE, knob); return; }
    if (obj == m_oscPolyphony)      { effect->setParameter(TF_GEN_POLYPHONY, fromIndex(value, 0, TF_MAXVOICES-1)); return; }
    if (obj == m_oscSpread)         { effect->setParameter(TF_GEN_SPREAD, knob); return; }
    if (obj == m_oscGlide)          { effect->setParameter(TF_GEN_GLIDE, knob); return; }
    if (obj == m_oscSlop)           { effect->setParameter(TF_GEN_SLOP, knob); return; }

    if (obj == m_addVolume)         { effect->setParameter(TF_GEN_VOLUME, knob); return; }
    if (obj == m_addBandwidth)      { effect->setParameter(TF_GEN_BANDWIDTH, knob); updateFreqView(); return; }
    if (obj == m_addDamp)           { effect->setParameter(TF_GEN_DAMP, knob); updateFreqView(); return; }
    if (obj == m_addHarmonics)      { effect->setParameter(TF_GEN_NUMHARMONICS, knob); updateFreqView(); return; }
    if (obj == m_addDrive)          { effect->setParameter(TF_GEN_DRIVE, knob); updateFreqView(); return; }
	if (obj == m_addMod)			{ effect->setParameter(TF_GEN_MODULATION, knob); updateFreqView(); return; }
	if (obj == m_addScale)			{ effect->setParameter(TF_GEN_SCALE, knob); updateFreqView(); return; }

    if (obj == m_noiseAmount)       { effect->setParameter(TF_NOISE_AMOUNT, knob); return; }
    if (obj == m_noiseFreq)         { effect->setParameter(TF_NOISE_FREQ, knob); return; }
    if (obj == m_noiseBW)           { effect->setParameter(TF_NOISE_BW, knob); return; }

    if (obj == m_lpFreq)            { effect->setParameter(TF_LP_FILTER_CUTOFF, knob); return; }
    if (obj == m_lpRes)             { effect->setParameter(TF_LP_FILTER_RESONANCE, knob); return; }
    if (obj == m_lpOn)              { effect->setParameter(TF_LP_FILTER_ON, value ? 1.0f : 0.0f); return; }

    if (obj == m_hpFreq)            { effect->setParameter(TF_HP_FILTER_CUTOFF, knob); return; }
    if (obj == m_hpRes)             { effect->setParameter(TF_HP_FILTER_RESONANCE, knob); return; }
    if (obj == m_hpOn)              { effect->setParameter(TF_HP_FILTER_ON, value ? 1.0f : 0.0f); return; }

    if (obj == m_adsr1A)            { effect->setParameter(TF_ADSR1_ATTACK, knob); return; }
    if (obj == m_adsr1D)            { effect->setParameter(TF_ADSR1_DECAY, knob); return; }
    if (obj == m_adsr1S)            { effect->setParameter(TF_ADSR1_SUSTAIN, knob); return; }
    if (obj == m_adsr1R)            { effect->setParameter(TF_ADSR1_RELEASE, knob); return; }
    if (obj == m_adsr1Slope)        { effect->setParameter(TF_ADSR1_SLOPE, knob); return; }

    if (obj == m_adsr2A)            { effect->setParameter(TF_ADSR2_ATTACK, knob); return; }
    if (obj == m_adsr2D)            { effect->setParameter(TF_ADSR2_DECAY, knob); return; }
    if (obj == m_adsr2S)            { effect->setParameter(TF_ADSR2_SUSTAIN, knob); return; }
    if (obj == m_adsr2R)            { effect->setParameter(TF_ADSR2_RELEASE, knob); return; }
    if (obj == m_adsr2Slope)        { effect->setParameter(TF_ADSR2_SLOPE, knob); return; }

    if (obj == m_lfo1Rate)          { effect->setParameter(TF_LFO1_RATE, knob); return; }
    if (obj == m_lfo1Depth)         { effect->setParameter(TF_LFO1_DEPTH, knob); return; }
    if (obj == m_lfo1Sync)          { effect->setParameter(TF_LFO1_SYNC, value ? 1.0f : 0.0f); return; }

    if (obj == m_lfo2Rate)          { effect->setParameter(TF_LFO2_RATE, knob); return; }
    if (obj == m_lfo2Depth)         { effect->setParameter(TF_LFO2_DEPTH, knob); return; }
    if (obj == m_lfo2Sync)          { effect->setParameter(TF_LFO2_SYNC, value ? 1.0f : 0.0f); return; }

    if (obj == m_mm1Src)            { effect->setParameter(TF_MM1_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm1Dest)           { effect->setParameter(TF_MM1_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    if (obj == m_mm2Src)            { effect->setParameter(TF_MM2_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm2Dest)           { effect->setParameter(TF_MM2_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    if (obj == m_mm3Src)            { effect->setParameter(TF_MM3_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm3Dest)           { effect->setParameter(TF_MM3_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    if (obj == m_mm4Src)            { effect->setParameter(TF_MM4_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm4Dest)           { effect->setParameter(TF_MM4_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    if (obj == m_mm5Src)            { effect->setParameter(TF_MM5_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm5Dest)           { effect->setParameter(TF_MM5_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    if (obj == m_mm6Src)            { effect->setParameter(TF_MM6_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm6Dest)           { effect->setParameter(TF_MM6_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    if (obj == m_mm7Src)            { effect->setParameter(TF_MM7_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm7Dest)           { effect->setParameter(TF_MM7_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    if (obj == m_mm8Src)            { effect->setParameter(TF_MM8_SOURCE, fromIndex(value, 0, eTfModMatrix::INPUT_COUNT-1)); return; }
    if (obj == m_mm8Dest)           { effect->setParameter(TF_MM8_TARGET, fromIndex(value, 0, eTfModMatrix::OUTPUT_COUNT-1)); return; }
    
    if (obj == m_effect1)           { effect->setParameter(TF_EFFECT_1, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect2)           { effect->setParameter(TF_EFFECT_2, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect3)           { effect->setParameter(TF_EFFECT_3, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect4)           { effect->setParameter(TF_EFFECT_4, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect5)           { effect->setParameter(TF_EFFECT_5, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect6)           { effect->setParameter(TF_EFFECT_6, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect7)           { effect->setParameter(TF_EFFECT_7, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect8)           { effect->setParameter(TF_EFFECT_8, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect9)           { effect->setParameter(TF_EFFECT_9, fromIndex(value, 0, FX_COUNT-1)); return; }
    if (obj == m_effect10)          { effect->setParameter(TF_EFFECT_10, fromIndex(value, 0, FX_COUNT-1)); return; }

    if (obj == m_distAmount)        { effect->setParameter(TF_DISTORT_AMOUNT, knob); return; }

    if (obj == m_chorusFreq)        { effect->setParameter(TF_CHORUS_RATE, knob); return; }
    if (obj == m_chorusDepth)       { effect->setParameter(TF_CHORUS_DEPTH, knob); return; }
    if (obj == m_chorusGain)        { effect->setParameter(TF_CHORUS_GAIN, knob); return; }

    if (obj == m_delayLeft)         { effect->setParameter(TF_DELAY_LEFT, knob); return; }
    if (obj == m_delayRight)        { effect->setParameter(TF_DELAY_RIGHT, knob); return; }
    if (obj == m_delayDecay)        { effect->setParameter(TF_DELAY_DECAY, knob); return; }

    if (obj == m_revRoomsize)       { effect->setParameter(TF_REVERB_ROOMSIZE, knob); return; }
    if (obj == m_revDamp)           { effect->setParameter(TF_REVERB_DAMP, knob); return; }
    if (obj == m_revWet)            { effect->setParameter(TF_REVERB_WET, knob); return; }
    if (obj == m_revWidth)          { effect->setParameter(TF_REVERB_WIDTH, knob); return; }

    if (obj == m_flangLfo)          { effect->setParameter(TF_FLANGER_LFO, knob); return; }
    if (obj == m_flangFreq)         { effect->setParameter(TF_FLANGER_FREQUENCY, knob); return; }
    if (obj == m_flangAmp)          { effect->setParameter(TF_FLANGER_AMPLITUDE, knob); return; }
    if (obj == m_flangWet)          { effect->setParameter(TF_FLANGER_WET, knob); return; }

    if (obj == m_formantWet)        { effect->setParameter(TF_FORMANT_WET, knob); return; }

    if (obj == m_eqLow)             { effect->setParameter(TF_EQ_LOW, knob); return; }
    if (obj == m_eqMid)             { effect->setParameter(TF_EQ_MID, knob); return; }
    if (obj == m_eqHigh)            { effect->setParameter(TF_EQ_HIGH, knob); return; }
}

void tfWindow::updateFreqView()
{
	m_freqView->update();
}

void tfWindow::_setPresetValue(eU32 index, eF32 value)
{
	m_synth->setParameter(index, value);
	setParameter(index, value);
}

void tfWindow::progChanged(int value)
{
    effect->setProgram(value);
    initParameters();
}

void tfWindow::progRestore(bool checked)
{
    if (!m_synth->loadProgram())
    {
        QMessageBox::critical(this, "TF3", "Program could not be restored!");
    }
    else
    {
        m_synth->loadProgramFromPresets();
        initParameters();
    }
}

void tfWindow::progSave(bool checked)
{
    m_synth->writeProgramToPresets();
    if (!m_synth->saveProgram())
        QMessageBox::critical(this, "TF3", "Program could not be saved!");
}

void tfWindow::manage(bool checked)
{
    m_synth->writeProgramToPresets();
    eTfManage dlg(m_synth);
    dlg.exec();
    _updateInstrSelection(true);
    m_synth->loadProgramFromPresets();
    initParameters();
}

void tfWindow::recordStart(bool checked)
{
	eTfRecorder::getInstance().startRecording();
}

void tfWindow::recordStop(bool checked)
{
	eTfRecorder::getInstance().stopRecording();

	const QString filePath = QFileDialog::getSaveFileName(this, "", "", "Tunefish songs (*.tf4m)");
	if (filePath != "")
		eTfRecorder::getInstance().saveToFile(filePath);
}

void tfWindow::_createIcons()
{
	const eU32 PIXWIDTH = 14;
	const eU32 PIXHEIGHT = 14;
	const eU32 MAX_X = PIXWIDTH-1;
	const eU32 MAX_Y = PIXHEIGHT-1;
	const QColor bgCol(0, 0, 0, 0);
	const QPen pen(Qt::white);

	m_pmSine = new QPixmap(PIXWIDTH, PIXHEIGHT);
	m_pmSawUp = new QPixmap(PIXWIDTH, PIXHEIGHT);
	m_pmSawDown = new QPixmap(PIXWIDTH, PIXHEIGHT);
	m_pmPulse = new QPixmap(PIXWIDTH, PIXHEIGHT);
	m_pmNoise = new QPixmap(PIXWIDTH, PIXHEIGHT);

	m_pmSine->fill(bgCol);
	m_pmSawUp->fill(bgCol);
	m_pmSawDown->fill(bgCol);
	m_pmPulse->fill(bgCol);
	m_pmNoise->fill(bgCol);

	// noise
	// ------------------------------------------
	QPainter painterSine(m_pmSine);
	painterSine.setPen(pen);
	//painterSine.setRenderHint(QPainter::Antialiasing);
	eU32 old = 0;
	for (eU32 i=0;i<PIXWIDTH;i++)
	{
		eU32 sine = eSin((eF32)i / PIXWIDTH * ePI*2) * PIXHEIGHT/2;
		if (i>0)
			painterSine.drawLine(i-1, old+PIXHEIGHT/2, i, sine+PIXHEIGHT/2);
		old = sine;
	}

	// saw down
	// ------------------------------------------
	QPainter painterSawDown(m_pmSawDown);
	painterSawDown.setPen(pen);
	//painterSawDown.setRenderHint(QPainter::Antialiasing);
	painterSawDown.drawLine(0, 0, MAX_X, MAX_Y);
	painterSawDown.drawLine(0, 0, 0, MAX_Y);
	
	// saw up
	// ------------------------------------------
	QPainter painterSawUp(m_pmSawUp);
	painterSawUp.setPen(pen);
	//painterSawUp.setRenderHint(QPainter::Antialiasing);
	painterSawUp.drawLine(0, MAX_Y, MAX_X, 0);
	painterSawUp.drawLine(MAX_X, 0, MAX_X, MAX_Y);

	// pulse
	// ------------------------------------------
	QPainter painterPulse(m_pmPulse);
	painterPulse.setPen(pen);
	//painterPulse.setRenderHint(QPainter::Antialiasing);
	painterPulse.drawLine(0, 0, MAX_X/2, 0);
	painterPulse.drawLine(MAX_X/2, MAX_Y, MAX_X, MAX_Y);
	painterPulse.drawLine(MAX_X/2, 0, MAX_X/2, MAX_Y);

	// noise
	// ------------------------------------------
	QPainter painterNoise(m_pmNoise);
	painterNoise.setPen(pen);
	//painterNoise.setRenderHint(QPainter::Antialiasing);
	for (eU32 i=0;i<PIXWIDTH;i++)
	{
		painterNoise.drawLine(i, MAX_Y/2, i, eRandom(0, MAX_Y));
	}
	
	m_icoSine = new QIcon(*m_pmSine);
	m_icoSawUp = new QIcon(*m_pmSawUp);
	m_icoSawDown = new QIcon(*m_pmSawDown);
	m_icoPulse = new QIcon(*m_pmPulse);
	m_icoNoise = new QIcon(*m_pmNoise);

	m_lfo1ShapeSine->setIcon(*m_icoSine);
	m_lfo2ShapeSine->setIcon(*m_icoSine);
	m_lfo1ShapeSawUp->setIcon(*m_icoSawUp);
	m_lfo2ShapeSawUp->setIcon(*m_icoSawUp);
	m_lfo1ShapeSawDown->setIcon(*m_icoSawDown);
	m_lfo2ShapeSawDown->setIcon(*m_icoSawDown);
	m_lfo1ShapePulse->setIcon(*m_icoPulse);
	m_lfo2ShapePulse->setIcon(*m_icoPulse);
	m_lfo1ShapeNoise->setIcon(*m_icoNoise);
	m_lfo2ShapeNoise->setIcon(*m_icoNoise);

	m_lfo1ShapeSine->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo2ShapeSine->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo1ShapeSawUp->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo2ShapeSawUp->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo1ShapeSawDown->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo2ShapeSawDown->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo1ShapePulse->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo2ShapePulse->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo1ShapeNoise->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
	m_lfo2ShapeNoise->setIconSize(QSize(PIXWIDTH, PIXHEIGHT));
}

void tfWindow::_freeIcons()
{
	eDelete(m_icoSine);
	eDelete(m_icoSawUp);
	eDelete(m_icoSawDown);
	eDelete(m_icoPulse);
	eDelete(m_icoNoise);

	eDelete(m_pmSine);
	eDelete(m_pmSawUp);
	eDelete(m_pmSawDown);
	eDelete(m_pmPulse);
	eDelete(m_pmNoise);
}

void tfWindow::_updateInstrSelection(bool updateText)
{
    eChar name[128];

    for(eU32 i=0;i<TF_VSTI_NUM_PROGRAMS;i++)
    {
        ((eTfVstSynth*)effect)->getProgramNameIndexed(0, i, name);

        if (updateText)
        {
            m_instrSelection->setItemText(i, QString(name));
        }
        else
        {
            m_instrSelection->addItem(QString(name));
        }
    }
}

