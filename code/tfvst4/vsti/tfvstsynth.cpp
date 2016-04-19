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

#include "tfvsti.hpp"

eTfVstSynth::eTfVstSynth (audioMasterCallback audioMaster, void* hInstance) : AudioEffectX (audioMaster, TF_VSTI_NUM_PROGRAMS, TF_PARAM_COUNT)
{
	modulePath = eTfGetModulePath((HINSTANCE)hInstance);

	// Initialize Tunefish
	// -------------------------------------------------------
	tf = new eTfSynth;
    eTfSynthInit(*tf);
    tf->instr[0] = new eTfInstrument;
    eTfInstrumentInit(*tf, *tf->instr[0]);

	// Add to recorder
	// -------------------------------------------------------
	recorderIndex = eTfRecorder::getInstance().addSynth(this);

	// initialize programs
	// -------------------------------------------------------
	for (long i = 0; i < TF_VSTI_NUM_PROGRAMS; i++)
		programs[i].loadDefault(i);

	loadProgramAll();

	for (long i = 0; i < 16; i++)
		channelPrograms[i] = i;

	if (programs)
		setProgram (0);

	// initialize editor
	// -------------------------------------------------------
    editor = new eTfEditor(this);
	
	// configure VSTi
	// -------------------------------------------------------
	if (audioMaster)
	{
		setNumInputs(0);			
		setNumOutputs(TF_VSTI_NUM_OUTPUTS);	
		canProcessReplacing();
		hasVu(false);
		hasClip(false);
		isSynth();
		setUniqueID('TF4');			
	}
	initProcess ();

	suspend ();
}

eTfVstSynth::~eTfVstSynth ()
{
	eTfRecorder::getInstance().removeSynth(this);
	eDelete(editor);
    eDelete(tf->instr[0]);
	eDelete(tf);
}

void eTfVstSynth::setProgram (long program)
{
    char str[1024];
    sprintf(str, "setProgram(%i)\n", program);
    LOG(str);

	if (program < 0 || program >= TF_VSTI_NUM_PROGRAMS)
		return;

	// write program from tunefish to program list before switching
	eTfSynthProgram *ap = &programs[curProgram];
	for(int i=0;i<TF_PARAM_COUNT;i++)
		ap->params[i] = tf->instr[0]->params[i];

	// switch program
	curProgram = program;
	
	// load new program to into tunefish
	ap = &programs[curProgram];	
	for(int i=0;i<TF_PARAM_COUNT;i++)
		tf->instr[0]->params[i] = ap->params[i];
}

void eTfVstSynth::writeProgramToPresets()
{
    eTfSynthProgram *ap = &programs[curProgram];
    for(int i=0;i<TF_PARAM_COUNT;i++)
        ap->params[i] = tf->instr[0]->params[i];
}

void eTfVstSynth::loadProgramFromPresets()
{
    eTfSynthProgram *ap = &programs[curProgram];	
    for(int i=0;i<TF_PARAM_COUNT;i++)
        tf->instr[0]->params[i] = ap->params[i];
}

void eTfVstSynth::setProgramName (char *name)
{
    LOG("setProgramName()\n");
	strcpy (programs[curProgram].name, name);
}

void eTfVstSynth::getProgramName (char *name)
{
    LOG("getProgramName()\n");
	strcpy (name, programs[curProgram].name);
}

void eTfVstSynth::getParameterLabel (long index, char *label)
{
    LOG("getParameterLabel()\n");
	strcpy(label, "");
}

void eTfVstSynth::getParameterDisplay (long index, char *text)
{
    LOG("getParameterDisplay()\n");
	text[0] = 0;
	float2string (tf->instr[0]->params[index], text);
}

void eTfVstSynth::getParameterName (long index, char *label)
{
    char str[1024];
    sprintf(str, "setParameterName(%i)\n", index);
    LOG(str);

	strcpy(label, TF_NAMES[index]);
}

void eTfVstSynth::setParameter (long index, float value)
{
    LOG("setParameter()\n");
	eTfSynthProgram *ap = &programs[curProgram];
	tf->instr[0]->params[index] = value;
    ap->params[index] = value;
}

float eTfVstSynth::getParameter (long index)
{
    char str[1024];
    sprintf(str, "getParameter(%i)\n", index);
    LOG(str);
	return tf->instr[0]->params[index];
}

bool eTfVstSynth::getOutputProperties (long index, VstPinProperties* properties)
{
    LOG("getOutputProperties()\n");
	if (index < TF_VSTI_NUM_OUTPUTS)
	{
		sprintf (properties->label, "Vstx %1d", index + 1);
		properties->flags = kVstPinIsActive;
		if (index < 2)
			properties->flags |= kVstPinIsStereo;	// make channel 1+2 stereo
		return true;
	}
	return false;
}

bool eTfVstSynth::getProgramNameIndexed (long category, long index, char* text)
{
    char str[1024];
    sprintf(str, "getProgramNameIndexed(%i, %i)\n", category, index);
    LOG(str);

	if (index < TF_VSTI_NUM_PROGRAMS)
	{
		strcpy (text, programs[index].name);
		return true;
	}
	return false;
}

bool eTfVstSynth::setProgramNameIndexed (long category, long index, const char* text)
{
	char str[1024];
	sprintf(str, "setProgramNameIndexed(%i, %i)\n", category, index);
	LOG(str);

	if (index < TF_VSTI_NUM_PROGRAMS)
	{
		strcpy (programs[index].name, text);
		return true;
	}
	return false;
}

void eTfVstSynth::getProgramData(long index, eTfSynthProgram *data)
{
    eMemCopy(data, &programs[index], sizeof(eTfSynthProgram));
}

void eTfVstSynth::setProgramData(long index, eTfSynthProgram *data)
{
    eMemCopy(&programs[index], data, sizeof(eTfSynthProgram));
}

bool eTfVstSynth::copyProgram (long destination)
{
    LOG("copyProgram()\n");
	if (destination < TF_VSTI_NUM_PROGRAMS)
	{
		programs[destination] = programs[curProgram];
		return true;
	}
	return false;
}

bool eTfVstSynth::getEffectName (char* name)
{
    LOG("getEffectName()\n");
	strcpy (name, "Tunefish v4");
	return true;
}

bool eTfVstSynth::getVendorString (char* text)
{
    LOG("getVendorString()\n");
	strcpy (text, "Brain Control");
	return true;
}

bool eTfVstSynth::getProductString (char* text)
{
    LOG("getProductString()\n");
	strcpy (text, "Brain Control Tunefish v4");
	return true;
}

long eTfVstSynth::canDo (char* text)
{
    LOG("canDo()\n");

	if (!strcmp (text, "receiveVstEvents"))
		return 1;
	if (!strcmp (text, "receiveVstMidiEvent"))
		return 1;
	if (!strcmp (text, "midiProgramNames"))
		return 1;
	return -1;	// explicitly can't do; 0 => don't know
}

// midi program names:
// as an example, GM names are used here. in fact, tf3Synth doesn't even support
// multi-timbral operation so it's really just for demonstration.
// a 'real' instrument would have a number of voices which use the
// programs[channelProgram[channel]] parameters when it receives
// a note on message.

long eTfVstSynth::getMidiProgramName (long channel, MidiProgramName* mpn)
{
    LOG("getMidiProgramName()\n");
	long prg = mpn->thisProgramIndex;
	if (prg < 0 || prg >= 128)
		return 0;
	fillProgram (channel, prg, mpn);
	if (channel == 9)
		return 1;
	return 128L;
}

long eTfVstSynth::getCurrentMidiProgram (long channel, MidiProgramName* mpn)
{
    LOG("getCurrentMidiProgram()\n");
	if (channel < 0 || channel >= 16 || !mpn)
		return -1;
	long prg = channelPrograms[channel];
	mpn->thisProgramIndex = prg;
	fillProgram (channel, prg, mpn);
	return prg;
}

void eTfVstSynth::fillProgram (long channel, long prg, MidiProgramName* mpn)
{
	/*
    LOG("fillProgram()\n");
	mpn->midiBankMsb =
	mpn->midiBankLsb = -1;
	mpn->reserved = 0;
	mpn->flags = 0;

	if (channel == 9)	// drums
	{
		strcpy (mpn->name, "Standard");
		mpn->midiProgram = 0;
		mpn->parentCategoryIndex = 0;
	}
	else
	{
		strcpy (mpn->name, GmNames[prg]);
		mpn->midiProgram = (char)prg;
		mpn->parentCategoryIndex = -1;	// for now

		for (long i = 0; i < kNumGmCategories; i++)
		{
			if (prg >= GmCategoriesFirstIndices[i] && prg < GmCategoriesFirstIndices[i + 1])
			{
				mpn->parentCategoryIndex = i;
				break;
			}
		}
	}
	*/
}

long eTfVstSynth::getMidiProgramCategory (long channel, MidiProgramCategory* cat)
{
	/*
    LOG("getMidiProgramCategory()\n");
	cat->parentCategoryIndex = -1;	// -1:no parent category
	cat->flags = 0;					// reserved, none defined yet, zero.
	long category = cat->thisCategoryIndex;
	if (channel == 9)
	{
		strcpy (cat->name, "Drums");
		return 1;
	}
	if (category >= 0 && category < kNumGmCategories)
		strcpy (cat->name, GmCategories[category]);
	else
		cat->name[0] = 0;
		
	return kNumGmCategories;
	*/

	return 0;
}

bool eTfVstSynth::hasMidiProgramsChanged (long channel)
{
    LOG("hasMidiProgramsChanged()\n");
	return false;	// updateDisplay ()
}

bool eTfVstSynth::getMidiKeyName (long channel, MidiKeyName* key)
								// struct will be filled with information for 'thisProgramIndex' and 'thisKeyNumber'
								// if keyName is "" the standard name of the key will be displayed.
								// if false is returned, no MidiKeyNames defined for 'thisProgramIndex'.
{
    LOG("getMidiKeyName()\n");
	// key->thisProgramIndex;		// >= 0. fill struct for this program index.
	// key->thisKeyNumber;			// 0 - 127. fill struct for this key number.
	key->keyName[0] = 0;
	key->reserved = 0;				// zero
	key->flags = 0;					// reserved, none defined yet, zero.
	return false;
}

bool eTfVstSynth::loadProgramAll()
{
	for(int i=0;i<TF_VSTI_NUM_PROGRAMS;i++)
	{
		loadProgram(i);

        if (i == curProgram)
        {
            // load new program to into tunefish
            eTfSynthProgram *ap = &programs[curProgram];	
            for(int i=0;i<TF_PARAM_COUNT;i++)
                tf->instr[0]->params[i] = ap->params[i];
        }
	}

	return true;
}

bool eTfVstSynth::loadProgram()
{
	return loadProgram(curProgram);
}

bool eTfVstSynth::loadProgram(long index)
{
	QString path = modulePath + "tf4programs\\program" + QString::number(index) + ".txt";
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QString name(file.readLine());
	eStrCopy(programs[index].name, name.trimmed().toAscii().constData());

	while(true)
	{
		QString line(file.readLine());

		if (line.size() == 0)
		{
			file.close();
			return true;
		}

		QStringList parts = line.split(";");
		if (parts.size() == 2)
		{
			QString key = parts[0];
			eF32 value = parts[1].toFloat();

			for(eU32 i=0;i<TF_PARAM_COUNT;i++)
			{
				if (key == TF_NAMES[i])
				{
					programs[index].params[i] = value;
					break;
				}
			}
		}
	}
}

bool eTfVstSynth::saveProgramAll()
{
	for(int i=0;i<TF_VSTI_NUM_PROGRAMS;i++)
	{
		if (!saveProgram(i))
			return false;
	}

	return true;
}

bool eTfVstSynth::saveProgram()
{
	return saveProgram(curProgram);
}

bool eTfVstSynth::saveProgram(long index)
{
	QString path = modulePath + "tf4programs\\program" + QString::number(index) + ".txt";
	QFile file(path);
	
	if (!file.open(QIODevice::WriteOnly))
		return false;
	
	file.write(programs[index].name);
	file.write("\r\n");

	for(eU32 i=0;i<TF_PARAM_COUNT;i++)
	{
		file.write(TF_NAMES[i]);
		file.write(";");
		file.write(QString::number(programs[index].params[i]).toAscii().constData());
		file.write("\r\n");
	}

	file.close();

	return true;
}

bool eTfVstSynth::copyProgram()
{
    eTfSynthProgram *ap = &copiedProgram;
	for(int i=0;i<TF_PARAM_COUNT;i++)
		ap->params[i] = tf->instr[0]->params[i];

    eStrCopy(ap->name, programs[curProgram].name);

    return true;
}

bool eTfVstSynth::pasteProgram()
{
    eTfSynthProgram *ap = &copiedProgram;
	for(int i=0;i<TF_PARAM_COUNT;i++)    
        tf->instr[0]->params[i] = ap->params[i];

    eStrCopy(programs[curProgram].name, ap->name);

    return true;
}

eTfSynth * eTfVstSynth::getTunefish()
{
    return tf;
}

void eTfVstSynth::setSampleRate (eF32 sampleRate)
{
    LOG("setSampleRate()\n");
	AudioEffectX::setSampleRate (sampleRate);
	
	tf->sampleRate = sampleRate;
}

void eTfVstSynth::setBlockSize (long blockSize)
{
    LOG("setBlockSize()\n");
	AudioEffectX::setBlockSize (blockSize);
}

void eTfVstSynth::resume ()
{
    LOG("resume()\n");
	wantEvents ();
}

void eTfVstSynth::initProcess ()
{
	
}

void eTfVstSynth::process (eF32 **inputs, eF32 **outputs, long sampleFrames)
{
	// process () is required, and accumulating (out += h)
	// processReplacing () is optional, and in place (out = h). even though
	// processReplacing () is optional, it is very highly recommended to support it

    eTfInstrumentProcess(*tf, *tf->instr[0], outputs, sampleFrames);
}

void eTfVstSynth::processReplacing (eF32 **inputs, eF32 **outputs, long sampleFrames)
{
	memset (outputs[0], 0, sampleFrames * sizeof (eF32));
	memset (outputs[1], 0, sampleFrames * sizeof (eF32));
	process (inputs, outputs, sampleFrames);
}

long eTfVstSynth::processEvents (VstEvents* ev)
{
    LOG("processEvents()\n");

	VstTimeInfo *timeInfo = nullptr;
	eF32 time = 0.0f;
	eU32 tempo = 0;

	if (eTfRecorder::getInstance().isRecording())
	{
		timeInfo = getTimeInfo(kVstTempoValid);
		if (!timeInfo)
			return 1;

		time = (eF32)(timeInfo->samplePos / timeInfo->sampleRate);
		tempo = timeInfo->tempo;
	}

	for (eS32 i = 0; i < ev->numEvents; i++)
	{
		if ((ev->events[i])->type != kVstMidiType)
			continue;

		VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
		char* midiData = event->midiData;
		eS32 status = midiData[0] & 0xf0;		// ignoring channel

		if (status == 0x90 || status == 0x80)	// we only look at notes
		{
			eS32 note = midiData[1] & 0x7f;
			eS32 velocity = midiData[2] & 0x7f;

			if (status == 0x80)
				velocity = 0;	// note off by velocity 0

			if (!velocity)
			{
				if (eTfInstrumentNoteOff(*tf->instr[0], note))
				{
					eTfRecorder::getInstance().recordEvent(eTfEvent(time, recorderIndex, note, 0));
					eTfRecorder::getInstance().setTempo(tempo);
				}
			}
			else
			{
				eTfInstrumentNoteOn(*tf->instr[0], note, velocity);
				eTfRecorder::getInstance().recordEvent(eTfEvent(time, recorderIndex, note, velocity));
			}
		}
		else if (status == 0xb0)
		{
			if (midiData[1] == 0x7e || midiData[1] == 0x7b)	// all notes off
			{
				eTfInstrumentAllNotesOff(*tf->instr[0]);
			}
		}

		event++;
	}
	return 1;	// want more
}


