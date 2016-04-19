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

#ifndef PRODUCTION_HPP
#define PRODUCTION_HPP

// make sure this all goes into .data section!
#include "../../demos/zoom/zoom.e4scr.bin.h"
#include "../../demos/zoom/zoom.e4scr.ops.h"

// synth config
#define eCFG_NO_TF_FX_FORMANT
#define eCFG_NO_TF_FX_EQ
#define eCFG_NO_TF_FX_CHORUS
#define eCFG_NO_TF_FX_FLANGER

//#define eCFG_NO_TF_LFO_SINE
#define eCFG_NO_TF_LFO_SAWUP
#define eCFG_NO_TF_LFO_SAWDOWN
#define eCFG_NO_TF_LFO_PULSE
#define eCFG_NO_TF_LFO_NOISE

// engine config
#define eCFG_NO_ENGINE_DEFERRED_SHADOWS
#define eCFG_NO_CLEANUP
//#define eCFG_NO_SETUP_DIALOG

#endif // PRODUCTION_HPP