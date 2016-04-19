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

#ifndef TF_VSTSYNTH_HPP
#define TF_VSTSYNTH_HPP

class eTfVstSynth : public AudioEffectX
{
public:
	eTfVstSynth(audioMasterCallback audioMaster, void* hInstance);
	~eTfVstSynth();

	virtual void		process(float **inputs, float **outputs, long sampleframes);
	virtual void		processReplacing(float **inputs, float **outputs, long sampleframes);
	virtual long		processEvents(VstEvents* events);

	virtual void		setProgram(long program);
	virtual void		setProgramName(char *name);
	virtual bool		setProgramNameIndexed(long category, long index, const char* text);
	virtual void		getProgramName(char *name);
	virtual bool		getProgramNameIndexed(long category, long index, char* text);
	virtual bool		copyProgram(long destination);

	virtual void		setParameter(long index, float value);
	virtual float		getParameter(long index);
	virtual void		getParameterLabel(long index, char *label);
	virtual void		getParameterDisplay(long index, char *text);
	virtual void		getParameterName(long index, char *text);
	
	virtual void		setSampleRate(float sampleRate);
	virtual void		setBlockSize(long blockSize);
	
	virtual void		resume();

	virtual bool		getOutputProperties(long index, VstPinProperties* properties);
		
	virtual bool		getEffectName(char* name);
	virtual bool		getVendorString(char* text);
	virtual bool		getProductString(char* text);
	virtual long		getVendorVersion() { return 1000; }
	virtual long		canDo(char* text);

	virtual long		getMidiProgramName(long channel, MidiProgramName* midiProgramName);
	virtual long		getCurrentMidiProgram(long channel, MidiProgramName* currentProgram);
	virtual long		getMidiProgramCategory(long channel, MidiProgramCategory* category);
	virtual bool		hasMidiProgramsChanged(long channel);
	virtual bool		getMidiKeyName(long channel, MidiKeyName* keyName);

	bool				loadProgram();
	bool				loadProgram(long index);
	bool				loadProgramAll();
	bool				saveProgram();
	bool				saveProgram(long index);
	bool				saveProgramAll();
    bool				copyProgram();
    bool				pasteProgram();

    void				getProgramData(long index, eTfSynthProgram *data);
    void				setProgramData(long index, eTfSynthProgram *data);

    void				writeProgramToPresets();
    void				loadProgramFromPresets();

    eTfSynth *			getTunefish();

private:
	void				initProcess();
	void				noteOn(long note, long velocity, long delta);
	void				noteOff();
	void				fillProgram(long channel, long prg, MidiProgramName* mpn);

	eTfSynthProgram		programs[TF_VSTI_NUM_PROGRAMS];
    eTfSynthProgram		copiedProgram;
	eTfSynth *			tf;
	eU32				recorderIndex;
	long				channelPrograms[16];
	QString				modulePath;
};

#endif
