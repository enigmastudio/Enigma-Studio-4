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

#ifndef TF_RECORDER_HPP
#define TF_RECORDER_HPP

#include <QtCore/QString>

class eTfRecorder 
{
public:
	eTfRecorder();
	~eTfRecorder();

	static eTfRecorder& getInstance();

	void				reset();
	void				startRecording();
	void				stopRecording();
	eBool				isRecording();

	eBool				saveToFile(QString fileName);
	void				recordEvent(eTfEvent e);
	void				setTempo(eU16 tempo);

	eS32 				addSynth(eTfVstSynth *synth);
	void				removeSynth(eTfVstSynth *synth);

private:
	eMutex				m_cs;
	eArray<eTfEvent>	m_events;
	eU16				m_tempo;
	eTfVstSynth	*		m_synths[TF_MAX_INSTR];
	eBool				m_isRecording;

	static eTfRecorder  m_recorder;
};

#endif 
