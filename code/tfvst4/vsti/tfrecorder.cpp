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

#include <QtCore/QFile>

#include "tfvsti.hpp"

eTfRecorder eTfRecorder::m_recorder;

#define ePARAM_ZERO(x) params[x] = 0.0f;
#define ePARAM_ZERO_IF_ZERO(x, y) if (params[x] < 0.01f) params[y] = 0.0f;
#define ePARAM_ZERO_IF_OFF(x, y) if (params[x] < 0.5f) params[y] = 0.0f;
#define ePARAM_ZERO_ALL_IF_ONE_ZERO(x1, x2, y) if (params[x1] < 0.01f || params[x2] < 0.01f) params[y] = params[x1] = params[x2] = 0.0f; 

static eBool isMMSourceUsed(eF32 *params, eU32 source)
{
	for (eU32 i=0; i<10; i++)
	{
		if (eFtoL(eRound(params[TF_MM1_SOURCE + i*3] * (eTfModMatrix::INPUT_COUNT-1))) == source)
			return eTRUE;
	}

	return eFALSE;
}

static void optimizeParams(eF32 *params)
{
	ePARAM_ZERO(TF_GEN_RANDOM);  // not used anymore
    ePARAM_ZERO(TF_GEN_SUBOSC);  // not used anymore
    ePARAM_ZERO(TF_GEN_SHAPE);   // not used anymore
    ePARAM_ZERO(TF_GEN_TEST);    // not used anymore

	ePARAM_ZERO_IF_ZERO(TF_NOISE_AMOUNT, TF_NOISE_FREQ);
	ePARAM_ZERO_IF_ZERO(TF_NOISE_AMOUNT, TF_NOISE_BW);

	ePARAM_ZERO_IF_OFF(TF_LP_FILTER_ON, TF_LP_FILTER_CUTOFF);
	ePARAM_ZERO_IF_OFF(TF_LP_FILTER_ON, TF_LP_FILTER_RESONANCE);

	ePARAM_ZERO_IF_OFF(TF_HP_FILTER_ON, TF_HP_FILTER_CUTOFF);
	ePARAM_ZERO_IF_OFF(TF_HP_FILTER_ON, TF_HP_FILTER_RESONANCE);

	ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_PROFILE);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_BANDWIDTH);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_NUMHARMONICS);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_DAMP);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_MODULATION);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_MODULATIONTYPE);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_SLOP);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_OCTAVE);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_GLIDE);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_DETUNE);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_FREQ);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_DRIVE);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_UNISONO);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_SPREAD);
    ePARAM_ZERO_IF_ZERO(TF_GEN_VOLUME, TF_GEN_SCALE);

	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM1_SOURCE, TF_MM1_TARGET, TF_MM1_MOD);
    ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM2_SOURCE, TF_MM2_TARGET, TF_MM2_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM3_SOURCE, TF_MM3_TARGET, TF_MM3_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM4_SOURCE, TF_MM4_TARGET, TF_MM4_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM5_SOURCE, TF_MM5_TARGET, TF_MM5_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM6_SOURCE, TF_MM6_TARGET, TF_MM6_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM7_SOURCE, TF_MM7_TARGET, TF_MM7_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM8_SOURCE, TF_MM8_TARGET, TF_MM8_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM9_SOURCE, TF_MM9_TARGET, TF_MM9_MOD);
	ePARAM_ZERO_ALL_IF_ONE_ZERO(TF_MM10_SOURCE, TF_MM10_TARGET, TF_MM10_MOD);

	if (!isMMSourceUsed(params, eTfModMatrix::INPUT_LFO1))
	{
		ePARAM_ZERO(TF_LFO1_RATE);
		ePARAM_ZERO(TF_LFO1_DEPTH);
		ePARAM_ZERO(TF_LFO1_SHAPE);
		ePARAM_ZERO(TF_LFO1_SYNC);
	}

	if (!isMMSourceUsed(params, eTfModMatrix::INPUT_LFO2))
	{
		ePARAM_ZERO(TF_LFO2_RATE);
		ePARAM_ZERO(TF_LFO2_DEPTH);
		ePARAM_ZERO(TF_LFO2_SHAPE);
		ePARAM_ZERO(TF_LFO2_SYNC);
	}

	if (!isMMSourceUsed(params, eTfModMatrix::INPUT_ADSR1))
	{
		ePARAM_ZERO(TF_ADSR1_ATTACK);
		ePARAM_ZERO(TF_ADSR1_DECAY);
		ePARAM_ZERO(TF_ADSR1_SUSTAIN);
		ePARAM_ZERO(TF_ADSR1_RELEASE);
		ePARAM_ZERO(TF_ADSR1_SLOPE);
	}

	if (!isMMSourceUsed(params, eTfModMatrix::INPUT_ADSR2))
	{
		ePARAM_ZERO(TF_ADSR2_ATTACK);
		ePARAM_ZERO(TF_ADSR2_DECAY);
		ePARAM_ZERO(TF_ADSR2_SUSTAIN);
		ePARAM_ZERO(TF_ADSR2_RELEASE);
		ePARAM_ZERO(TF_ADSR2_SLOPE);
	}

	// this is specific to project "zoom"
	ePARAM_ZERO(TF_CHORUS_GAIN);
	ePARAM_ZERO(TF_CHORUS_RATE);
    ePARAM_ZERO(TF_CHORUS_DEPTH);

    ePARAM_ZERO(TF_FLANGER_LFO);
    ePARAM_ZERO(TF_FLANGER_FREQUENCY);
    ePARAM_ZERO(TF_FLANGER_AMPLITUDE);
    ePARAM_ZERO(TF_FLANGER_WET);

    ePARAM_ZERO(TF_FORMANT_MODE);
    ePARAM_ZERO(TF_FORMANT_WET);

    ePARAM_ZERO(TF_EQ_LOW);
    ePARAM_ZERO(TF_EQ_MID);
    ePARAM_ZERO(TF_EQ_HIGH);
}

eTfRecorder::eTfRecorder()
{
	eMemSet(m_synths, 0, sizeof(eTfVstSynth*) * TF_MAX_INSTR);
	m_isRecording = eFALSE;
	m_tempo = 0;
}

eTfRecorder::~eTfRecorder()
{

}

eTfRecorder & eTfRecorder::getInstance()
{
	return m_recorder;
}

void eTfRecorder::reset()
{
	m_cs.enter();
	m_events.clear();
	m_cs.leave();
}

void eTfRecorder::startRecording()
{
	if (!m_isRecording)
	{
		reset();
		m_isRecording = eTRUE;
	}
}

void eTfRecorder::stopRecording()
{
	m_isRecording = eFALSE;
}

eBool eTfRecorder::isRecording()
{
	return m_isRecording;
}

eBool eTfRecorder::saveToFile(QString fileName)
{
	stopRecording();

	m_cs.enter();

	// write binary file
	// -------------------------------------------------------------------
	QFile fileLog(fileName + ".log");
	QFile fileBin(fileName);

	if (!fileBin.open(QIODevice::WriteOnly))
		return eFALSE;

	if (!fileLog.open(QIODevice::WriteOnly))
		return eFALSE;

	// count stuff
	eU16 synthCount = 0;
	eU16 eventCount[TF_MAX_INSTR];

	for(eU32 i=0;i<TF_MAX_INSTR; i++) 
	{
		eventCount[i] = 0;
		if (m_synths[i] != nullptr)
			synthCount++;
	}

	for(eU32 i=0;i<m_events.size();i++)
	{
		eventCount[m_events[i].instr]++;
	}

	// write header values
	fileBin.write((const char *)&synthCount, sizeof(eU16));
	fileBin.write((const char *)&m_tempo, sizeof(eU16));

	fileLog.write("Instruments: ");
	fileLog.write(QString::number(synthCount).toAscii().constData());
	fileLog.write("\r\n");

	fileLog.write("Tempo: ");
	fileLog.write(QString::number(m_tempo).toAscii().constData());
	fileLog.write("\r\n");

	for(eU32 i=0;i<TF_MAX_INSTR; i++) 
	{
		if (m_synths[i] != nullptr)
		{
			fileBin.write((const char *)&eventCount[i], sizeof(eU16));

			fileLog.write("Eventcount for instr ");
			fileLog.write(QString::number(i).toAscii().constData());
			fileLog.write(": ");
			fileLog.write(QString::number(eventCount[i]).toAscii().constData());
			fileLog.write("\r\n");
		}
	}

	fileLog.write("Instruments\r\n");
	fileLog.write("-----------------------------------------\r\n");

	fileBin.write("INST");

	// optimize instruments
	for(eU32 i=0;i<TF_MAX_INSTR; i++) 
	{
		eTfVstSynth *synth = m_synths[i];
		if (synth != nullptr)
		{
			eTfSynth *tf = synth->getTunefish();
			optimizeParams(tf->instr[0]->params);
		}
	}

	// write instruments  (grouped by instruments)
	for(eU32 i=0;i<TF_MAX_INSTR; i++) 
	{
		eTfVstSynth *synth = m_synths[i];

		if (synth != nullptr)
		{
			eTfSynth *tf = synth->getTunefish();

			fileLog.write("Params for instr ");
			fileLog.write(QString::number(i).toAscii().constData());
			fileLog.write("\r\n");
			fileLog.write("-----------------------------------------\r\n");

			for(eU32 j=0; j<TF_PARAM_COUNT; j++)
			{
				eF32 value = tf->instr[0]->params[j];
				eU8 ivalue = (eU8)(value * 100.0f);

				fileLog.write(TF_NAMES[j]);
				fileLog.write(": ");
				fileLog.write(QString::number(value).toAscii().constData());
				fileLog.write(" -> ");
				fileLog.write(QString::number(ivalue).toAscii().constData());
				fileLog.write("\r\n");

				fileBin.write((const char *)&ivalue, sizeof(eU8));
			}
		}
	}

	/*
	// write instruments  (grouped by paramindex)		
	for(eU32 j=0; j<TF_PARAM_COUNT; j++)
	{
		for(eU32 i=0;i<TF_MAX_INSTR; i++) 
		{
			eTfVstSynth *synth = m_synths[i];

			if (synth != nullptr)
			{
				eTfSynth *tf = synth->getTunefish();

				eF32 value = tf->instr[0]->params[j];
				eU8 ivalue = (eU8)(value * 100.0f);

				fileBin.write((const char *)&ivalue, sizeof(eU8));
			}
		}
	}
	*/
	fileLog.write("Events (Time, iTime, Instrument, Note, Velocity)\r\n");
	fileLog.write("-----------------------------------------\r\n");

	// calculate speed values
    const eU32 rows_per_beat = 4;
	const eU32 rows_per_min = m_tempo * rows_per_beat;
    const eF32 rows_per_sec = (eF32)rows_per_min / 60.0f;

    // write events
	for(eU32 i=0;i<m_events.size();i++)
	{
		eTfEvent &e = m_events[i];

		fileLog.write("Event: ");
		fileLog.write(QString::number(e.time).toAscii().constData());
		fileLog.write("\t");
		fileLog.write(QString::number((eU32)(e.time * rows_per_sec)).toAscii().constData());
		fileLog.write("\t");
		fileLog.write(QString::number(e.instr).toAscii().constData());
		fileLog.write("\t");
		fileLog.write(QString::number(e.note).toAscii().constData());
		fileLog.write("\t");
		fileLog.write(QString::number(e.velocity).toAscii().constData());
		fileLog.write("\r\n");
	}

	fileBin.write("SONG");

	for(eU32 i=0;i<TF_MAX_INSTR; i++) 
	{
		eTfVstSynth *synth = m_synths[i];

		if (synth != nullptr)
		{
			// write times
			eU16 oldRow = 0;
			for(eU32 j=0;j<m_events.size();j++)
			{
				eTfEvent &e = m_events[j];
				if (e.instr == i)
				{
                    eU16 row = eFtoL(eRound(e.time * rows_per_sec));
                    eU16 diff = row - oldRow;
                    oldRow = row;

                    fileBin.write((const char *)&diff, sizeof(eU16));
				}
			}

			// write notes
			for(eU32 j=0;j<m_events.size();j++)
			{
				eTfEvent &e = m_events[j];
				if (e.instr == i)
				{
					fileBin.write((const char *)&e.note, sizeof(eU8));
				}
			}

			// write velocities
			for(eU32 j=0;j<m_events.size();j++)
			{
				eTfEvent &e = m_events[j];
				if (e.instr == i)
				{
					eU8 vel = e.velocity;
					fileBin.write((const char *)&vel, sizeof(eU8));
				}	
			}
		}
	}

    fileBin.write("ENDS");
	fileBin.close();
	fileLog.close();

	m_cs.leave();

	// write header file
	// -------------------------------------------------------------------
	QFile headerIn(fileName);
	QFile headerOut(fileName + ".h");

	if (!headerIn.open(QIODevice::ReadOnly))
		return eFALSE;

	QByteArray input = headerIn.readAll();
	headerIn.close();

	if (!headerOut.open(QIODevice::WriteOnly))
		return eFALSE;

	headerOut.write("const unsigned char song[] = {\r\n");
	
	for(eS32 i=0;i<input.size();i++) 
	{
		eBool lastByte = i == input.size()-1;
		eBool lastInRow = i % 16 == 15;
		eBool firstInRow = i % 16 == 0;

		if (firstInRow)
			headerOut.write("\t");

		QString numstr = QString::number((const eU8)input[i], 16);
		if (numstr.length() == 1)
			numstr = "0" + numstr;

		numstr = QString("0x") + numstr;

		headerOut.write(numstr.toAscii().constData());

		if (!lastByte)
			headerOut.write(", ");

		if (lastInRow || lastByte)
			headerOut.write("\r\n");
	}

	headerOut.write("};\r\n");

	headerOut.close();

	return eTRUE;
}

void eTfRecorder::recordEvent(eTfEvent e)
{
	if (m_isRecording)
	{
		m_cs.enter();
		m_events.push(e);
		m_cs.leave();
	}
}

void eTfRecorder::setTempo(eU16 tempo)
{
	m_tempo = tempo;
}

eS32 eTfRecorder::addSynth(eTfVstSynth *synth)
{
	eS32 index = -1;

	m_cs.enter();
	for(eU32 i=0;i<TF_MAX_INSTR; i++) 
	{
		if (m_synths[i] == nullptr) 
		{
			m_synths[i] = synth;
			index = i;
			break;
		}
	}
	m_cs.leave();

	return index;
}

void eTfRecorder::removeSynth(eTfVstSynth *synth)
{
	m_cs.enter();
	for(eU32 i=0;i<TF_MAX_INSTR; i++) 
	{
		if (m_synths[i] == synth) 
		{
			m_synths[i] = nullptr;
			break;
		}
	}
	m_cs.leave();
}
