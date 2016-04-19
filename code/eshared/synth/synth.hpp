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

#ifndef SYNTH_HPP
#define SYNTH_HPP

#include "tf4.hpp"
#include "tf4dx.hpp"

#ifdef ePLAYER
#include "../../eplayer4/production.hpp"
#endif

extern eTfPlayer *eSfx;

class eSynth
{
public:
    eSynth(eU32 sampleRate) 
    {
        eSfx = &m_player;
        eTfDxInit(m_tfdx, 44100);
        eTfPlayerInit(m_player, 44100);
        eTfDxStart(m_tfdx, m_player);
    }

    ~eSynth()
    {
#ifndef eCFG_NO_CLEANUP
        shutdown();
#endif
    }

    void shutdown()
    {
        eTfDxStop(m_tfdx);
        eTfDxShutdown(m_tfdx);
    }

private:
    eTfDx       m_tfdx;
    eTfPlayer   m_player;
};

#endif // SYNTH_HPP