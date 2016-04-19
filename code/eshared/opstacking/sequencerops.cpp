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

#include "../eshared.hpp"

eDEF_OPERATOR_SOURCECODE(SEQUENCER);

// Scene (sequencer) operator
// --------------------------
// Adds a scene entry to the sequencer.

#if defined(HAVE_OP_SEQUENCER_SCENE) || defined(eEDITOR)
eOP_DEF_SEQ(eSeqSceneOp, "Scene", 's', 1, 1, eOP_INPUTS(eOC_FX))
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eSeqSceneOp, startTime, "Start time", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eSeqSceneOp, duration, "Duration", 0.01f, eF32_MAX, 10.0f,
        eOP_PAR_INT(eSeqSceneOp, track, "Track", 0, eSequencer::MAX_TRACKS-1, 0,
        eOP_PAR_LABEL(eSeqSceneOp, label0, "Extended properties", "Extended properties",
        eOP_PAR_FLOAT(eSeqSceneOp, timeOffset, "Time offset", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eSeqSceneOp, timeScale, "Time scale", eF32_MIN, eF32_MAX, 1.0f,
        eOP_PAR_ENUM(eSeqSceneOp, blendMode, "Blending", "Additive|Subtractive|Multiplicative|Brighter|Darker|None", 5,
        eOP_PAR_FXY(eSeqSceneOp, blendRatios, "Blend ratios", 0.0f, 1.0f, 1.0f, 1.0f,
		eOP_PAR_END)))))))))
    {
        eSeqEntry entry;

        entry.type = eSET_SCENE;
        entry.startTime = startTime;
        entry.duration = duration;
        entry.blendMode = (eSeqBlendMode&)blendMode;
        entry.blendRatios = blendRatios;

        entry.scene.effect = ((eIEffectOp *)getAboveOp(0))->getResult().fx;
        entry.scene.timeOffset = timeOffset;
        entry.scene.timeScale = timeScale;

        m_seq.addEntry(entry, track);
    }
	eOP_EXEC2_END
eOP_END(eSeqSceneOp);
#endif

// Overlay (sequencer) operator
// ----------------------------
// Adds an overlay entry to the sequencer.

#if defined(HAVE_OP_SEQUENCER_OVERLAY) || defined(eEDITOR)
eOP_DEF_SEQ(eSeqOverlayOp, "Overlay", 'o', 1, 1, eOP_INPUTS(eOC_BMP|eOC_MAT))
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eSeqOverlayOp, startTime, "Start time", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eSeqOverlayOp, duration, "Duration", 0.01f, eF32_MAX, 1.0f,
        eOP_PAR_INT(eSeqOverlayOp, track, "Track", 0, eSequencer::MAX_TRACKS-1, 0,
        eOP_PAR_LABEL(eSeqOverlayOp, label0, "Extended properties", "Extended properties",
        eOP_PAR_ENUM(eSeqOverlayOp, blendMode, "Blending", "Additive|Subtractive|Multiplicative|Brighter|Darker|None", 0,
        eOP_PAR_FXY(eSeqOverlayOp, blendRatios, "Blend ratios", 0.0f, 1.0f, 1.0f, 1.0f,
        eOP_PAR_FXYZW(eSeqOverlayOp, rect, "Rectangle", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 1.0f, 1.0f,
        eOP_PAR_FXY(eSeqOverlayOp, scrollUv, "Scroll U/V", eF32_MIN, eF32_MAX, 0.0f, 0.0f,
        eOP_PAR_FXY(eSeqOverlayOp, tileUv, "Tile U/V", eF32_MIN, eF32_MAX, 1.0f, 1.0f,
        eOP_PAR_ENUM(eSeqOverlayOp, texAddr, "U/V address mode", "Wrap|Clamp|Mirror", 1,
        eOP_PAR_BOOL(eSeqOverlayOp, filtered, "Filtered", eTRUE,
		eOP_PAR_END))))))))))))
    {
        eSeqEntry entry;

        entry.type = eSET_OVERLAY;
        entry.duration = duration;
        entry.startTime = startTime;
        entry.blendMode = (eSeqBlendMode)blendMode;
        entry.blendRatios = blendRatios;
    
        eTexture2d *tex = nullptr;
        if (getAboveOp(0)->getResultClass() == eOC_MAT)
            tex = (eTexture2d *)((eIMaterialOp *)getAboveOp(0))->getResult().mat.textures[eMTU_DIFFUSE];
        else
            tex = ((eIBitmapOp *)getAboveOp(0))->getResult().uav->tex;

        entry.overlay.rect = rect;
        entry.overlay.tileUv = tileUv;
        entry.overlay.texture = tex;
        entry.overlay.filtered = filtered;
        entry.overlay.scrollUv = scrollUv;
        entry.overlay.texAddr = eTMF_WRAP<<texAddr;

        m_seq.addEntry(entry, track);
    }
	eOP_EXEC2_END
eOP_END(eSeqOverlayOp);
#endif

// Overlay (sequencer) operator
// ----------------------------
// Adds an overlay entry to the sequencer.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined(HAVE_OP_SEQUENCER_TIME_TEXT) || defined(eEDITOR)
eOP_DEF_SEQ(eSeqTimeTextOp, "Time text", 'o', 0, 0, eOP_INPUTS())
    eOP_INIT()
    {
        m_fontFaceHash = 0;
        m_fontRes = 0;
		m_fontColor = 0;
    }

	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eSeqTimeTextOp, startTime, "Start time", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eSeqTimeTextOp, duration, "Duration", 0.01f, eF32_MAX, 10.0f,
        eOP_PAR_INT(eSeqTimeTextOp, track, "Track", 0, eSequencer::MAX_TRACKS-1, 0,

        eOP_PAR_LABEL(eSeqTimeTextOp, label0, "Font", "Font",
        eOP_PAR_STRING(eSeqTimeTextOp, fontFace, "Font face", "",
        eOP_PAR_INT(eSeqTimeTextOp, fontRes, "Resolution", 4, 10000, 10,
		eOP_PAR_RGB(eSeqTimeTextOp, color, "Color", 255, 255, 255,
		eOP_PAR_BOOL(eSeqTimeTextOp, filtered, "Filtered", eTRUE,

        eOP_PAR_LABEL(eSeqTimeTextOp, label1, "Time", "Time",
        eOP_PAR_FXY(eSeqTimeTextOp, posTime, "Position 0", eF32_MIN, eF32_MAX, 0.5f, 0.5f,
        eOP_PAR_FXY(eSeqTimeTextOp, sizeTime, "Size 0", eF32_MIN, eF32_MAX, 1.0f, 1.0f,
		eOP_PAR_FLOAT(eSeqTimeTextOp, time, "Time", 0.0, eF32_MAX, 0,
        eOP_PAR_FLOAT(eSeqTimeTextOp, accel, "Accel", 1.0, eF32_MAX, 0,

        eOP_PAR_LABEL(eSeqTimeTextOp, label2, "Scale", "Scale",
        eOP_PAR_FXY(eSeqTimeTextOp, posScale, "Position 1", eF32_MIN, eF32_MAX, 0.5f, 0.5f,
        eOP_PAR_FLOAT(eSeqTimeTextOp, sizeScale, "Size 1", eF32_MIN, eF32_MAX, 1.0f,
        eOP_PAR_INT(eSeqTimeTextOp, base, "Base", 1, eU16_MAX, 10,
        eOP_PAR_INT(eSeqTimeTextOp, exp, "Exponent", 0, eU16_MAX, 0,
		eOP_PAR_END)))))))))))))))))))
    {
                //eOP_PAR_STRING(eSeqTimeTextOp, unit, "Unit", "Meters",

        const eU32 hash = eHashStr(fontFace);
        if (m_fontFaceHash != hash || m_fontRes != fontRes || m_fontColor != color.toArgb())
        {
            GetLocalTime(&m_sysTime);

            eChar letters[256];
			eMemSet(letters, 0, 256);
            for (eU32 i=32; i<125; i++)
                letters[i-32] = (eChar)i;

            m_font.create(fontFace, letters, fontRes, color);
            m_fontFaceHash = hash;
			m_fontRes = fontRes;
			m_fontColor = color.toArgb();
        }

		const eF64 seconds = ePow(time, accel);

		FILETIME f;
		SystemTimeToFileTime( &m_sysTime, &f );
		ULARGE_INTEGER u; 
		eMemCopy(&u, &f, sizeof(u));

		const eF64 secondsPer100nsInterval = 100.*1.e-9;
		const eF64 numberOf100nsIntervals = seconds / secondsPer100nsInterval;
		u.QuadPart += eDtoULL(numberOf100nsIntervals);
		eMemCopy(&f, &u, sizeof(f));
		FileTimeToSystemTime(&f, &m_sysTime);

		eU32 len = GetTimeFormat(0x0409, 0, &m_sysTime, nullptr, m_text, 100);
		m_text[len-1] = ' ';
		GetDateFormat(0x0409, 0, &m_sysTime, nullptr, m_text+len, 99-len);

        eSeqEntry entry;
        entry.type = eSET_TEXT;
        entry.duration = duration;
        entry.startTime = startTime;
        entry.blendMode = eSBM_ADD;
        entry.blendRatios = eVector2(1.0f, 1.0f);
    
        entry.text.font = &m_font;
        entry.text.text = m_text;
        entry.text.pos = posTime;
        entry.text.size = sizeTime;
		entry.text.filtered = filtered;

        m_seq.addEntry(entry, track);
    }
	eOP_EXEC2_END

    eOP_VAR(SYSTEMTIME m_sysTime);
    eOP_VAR(eU32  m_fontFaceHash);
	eOP_VAR(eU32  m_fontColor);
    eOP_VAR(eU32  m_fontRes);
    eOP_VAR(eFont m_font);
    eOP_VAR(eChar m_text[100]);
eOP_END(eSeqTimeTextOp);
#endif