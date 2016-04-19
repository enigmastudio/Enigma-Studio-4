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

#ifndef SETUP_DLG_HPP
#define SETUP_DLG_HPP

struct eSetup
{
    eSetup() :
        vsync(eFALSE),
        fullScreen(eFALSE),
        res(1280, 720)
    {
    }

    eSize   res;
    eBool   vsync;
    eBool   fullScreen;
};

// returns false, if config dialog was canceled,
// otherwise true is returned (=> start demo)
eBool eShowSetupDialog(eSetup &setup, const eEngine &engine);

#endif // SETUP_DLG_HPP