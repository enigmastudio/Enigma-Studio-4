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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "../eshared/system/system.hpp"
#include "../eshared/synth/tf4.hpp"
#include "../eshared/synth/tf4dx.hpp"

#include "../../demos/tf4test/doomed_harlequin.tf4m.h"

#ifdef eDEBUG
eInt WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, eChar *cmdLine, eInt showCmd)
#else
void WinMainCRTStartup()
#endif
{
    eTfDx tfdx;
    eTfPlayer player;

    eTfDxInit(tfdx, 44100);
    eTfPlayerInit(player, 44100);
    player.volume = 0.4f;
    
    //const eU8 song[] = { 0, 0, 0, 0};  // test for an empty song
    eTfPlayerLoadSong(player, song, sizeof(song));

    eTfDxStart(tfdx, player);
    eTfPlayerStart(player, 0.0f);

    MessageBox(0, "Playing: Skyrunner - Doomed Harlequin\n\nVisit www.braincontrol.org", "Tunefish4", 0);

    eTfPlayerStop(player);
    eTfDxStop(tfdx);
    eTfDxShutdown(tfdx);

    ExitProcess(0);

#ifdef eDEBUG
    return 0;
#endif
}