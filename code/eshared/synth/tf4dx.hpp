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

#ifndef TF4DX_HPP
#define TF4DX_HPP

struct IDirectSound8;
struct IDirectSoundBuffer8;

struct eTfDx
{
	IDirectSound8 *         dsound;
	IDirectSoundBuffer8 *   dsoundBuffer;
	eU32                    nextWriteOffset;
	eU32                    bufferSize;
	eU32                    sampleRate;

	eTfPlayer *				player;

    eBool                   playing;
    eBool                   joinRequest;
    class eTfDxThread *     thread;
};

eBool	eTfDxInit(eTfDx &tfdx, eU32 sampleRate);
void    eTfDxShutdown(eTfDx &tfdx);

void    eTfDxStart(eTfDx &tfdx, eTfPlayer &player);
void    eTfDxStop(eTfDx &tfdx);
void    eTfDxFill(eTfDx &tfdx, const eU8 *data, eU32 count);
eBool   eTfDxNeedMore(eTfDx &tfdx);

#endif