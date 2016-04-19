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

QString eTfGetModulePath(HINSTANCE hInstance)
{
	eChar mpath[512];
	eMemSet(mpath, 0, 512);
	GetModuleFileName((HMODULE)hInstance, mpath, 512);

	eChar *str = &mpath[511];
	while (str != mpath && *str!='/' && *str!='\\')
	{
		*str-- = '\0';
	}

	return QString(mpath);
}

void eTfSetInstrumentName(eTfVstSynth &synth, eU32 index, QString name)
{
    eChar cname[24]; 
    eStrNCopy(cname, name.toAscii().constData(), 24);
    cname[23] = '\0';
    synth.setProgramNameIndexed(0, index, cname);
}

QString eTfGetInstrumentName(eTfVstSynth &synth, eU32 index)
{
    eChar name[32];
    synth.getProgramNameIndexed(0, index, name);
    return QString(name);
}

