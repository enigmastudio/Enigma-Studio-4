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

#include "../eshared/eshared.hpp"
#include "../configinfo.hpp"
#include "setupdlg.hpp"
#include "production.hpp"
#include "clibreplace.hpp"

static eBool loadingCallback(eU32 processed, eU32 total, ePtr param)
{
    eASSERT(processed <= total);

    eIDemoOp *loadingOp = (eIDemoOp *)param;
    eMessage msg;
    
    eGfx->handleMessages(msg);

	eU32 opLimit = 1046;

    if ((msg == eMSG_IDLE) 
		&& (processed <= opLimit)) // workaround for avoiding loading bar disappearing
    {
        eGfx->beginFrame();

        if (loadingOp)
        {
//            eF32 time = (eF32)processed/(eF32)total;
            eF32 time = eClamp(0.0f, (eF32)(processed)/(eF32)opLimit, 1.0f);
			if(eIEffect::m_doPostponeShaderLoading) // force time 0.0f at first frame
				time = 0.0f;
            loadingOp->process(time);
            loadingOp->getResult().demo.render(time);
        }
        else
            eGfx->clear(eCM_ALL, eCOL_BLACK);

        eGfx->endFrame();

		// after the first displayed frame load the postponed shaders
		if(eIEffect::m_doPostponeShaderLoading) {
			eIEffect::m_doPostponeShaderLoading = eFALSE;
			eIEffect::loadPostponedPixelShaders();
		}

    }

    return (msg != eMSG_QUIT);
}

#ifdef eDEBUG
eInt WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, eChar *cmdLine, eInt showCmd)
#else
void WinMainCRTStartup()
#endif
{
    eSimdSetArithmeticFlags(eSAF_RTN|eSAF_FTZ);
    eLeakDetectorStart();
    eGlobalsStaticsInit();

    eSynth synth(44100);
    eSetup setup;
    eEngine engine;

#ifndef eCFG_NO_SETUP_DIALOG
    if (eShowSetupDialog(setup, engine))
#endif
    {
        eDemoScript ds(demoData, eDEMODATA_LENGTH);
		eDemoData::readShaders(ds);

		//setup.res = eSize(640, 272);
		//setup.fullScreen = eFALSE;

        const eU32 wndFlags = (setup.vsync ? eWF_VSYNC : 0)|(setup.fullScreen ? eWF_FULLSCREEN : 0);
        engine.openWindow(wndFlags, setup.res, nullptr);

		eIEffect::m_doPostponeShaderLoading = eTRUE;
		eIDemoOp *demoOp = eDemoData::importScript(ds);

        demoOp->setProcessAll(eTRUE);
        eGfx->setWindowTitle(demoOp->getParameter(1).getAnimValue().string);
        eIDemoOp *loadingOp = (eIDemoOp *)eDemoData::findOperator(demoOp->getParameter(2).getAnimValue().linkedOpId);
        const eDemo &demo = demoOp->getResult().demo;

        if (loadingOp)
            loadingOp->process(0.0f);

        if (demoOp->process(0.0f, loadingCallback, loadingOp) != eOPR_CANCELED)
        {
            // touch shaders
            for (eF32 t=0.0f; t<=demo.getSequencer()->getEndTime(); t+=1.0f)
                demo.render(t);

            // enter main-loop
            demoOp->setProcessAll(eFALSE);
			eTfPlayerStart(*eSfx, 0.0f);
            
            eTimer timer;
            eMessage msg = eMSG_IDLE;
            eSeqResult seqRes = eSEQ_PLAYING;

            while (msg != eMSG_QUIT && seqRes != eSEQ_FINISHED)
            {
                eGfx->handleMessages(msg);

                if (msg == eMSG_IDLE)
                {
                    const eF32 time = (eF32)timer.getElapsedMs()*0.001f;
                    eGfx->beginFrame();
                    demoOp->process(time);
                    seqRes = demo.render(time);
                    eGfx->endFrame();

					/*
					eString buf;
					buf += "@sTex2d=";
					buf += eIntToStr(eGfx->g_numAddsTex2d);
					buf += ",@sTex3d=";
					buf += eIntToStr(eGfx->g_numAddsTex3d);
					buf += ",@sTexCube=";
					buf += eIntToStr(eGfx->g_numAddsTexCube);
					buf += ",@sStaticGeo=";
					buf += eIntToStr(eGfx->g_numAddsStaticGeo);

					buf += ",RsTex2d=";
					buf += eIntToStr(eGfx->g_numRemsTex2d);
					buf += ",RsTex3d=";
					buf += eIntToStr(eGfx->g_numRemsTex3d);
					buf += ",RsTexCube=";
					buf += eIntToStr(eGfx->g_numRemsTexCube);
					buf += ",RsStaticGeo=";
					buf += eIntToStr(eGfx->g_numRemsStaticGeo);

					eGfx->setWindowTitle(buf);
					*/
                }
            }

#ifndef eCFG_NO_CLEANUP
            eTfPlayerStop(*eSfx);
#endif
        }
      
#ifndef eCFG_NO_CLEANUP
        eOpStacking::shutdown();
#endif
    }

#ifndef eCFG_NO_CLEANUP
    eGlobalsStaticsFree();
#endif

    ExitProcess(0);

#ifdef eDEBUG
    return 0;
#endif
}

// define demodata as the very last element
const eU8		eIOperator::demodata_parsCount[] = ePAR_COUNT_ARRAY;
const eU8		eIOperator::demodata_pars[] = ePAR_INITIALIZER_ARRAY;
const eU8		eIOperator::demodata_parsMinMax[] = ePAR_MINMAX_ARRAY;
const char *	eEngine::demodata_shader_indices[eSHADERS_COUNT];
eByteArray		eEngine::demodata_shaders;
