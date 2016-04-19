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

eDEF_OPERATOR_SOURCECODE(EFFECT);

// Camera (effect) operator
// ------------------------
// Converts the model input-operator to an effect
// and adds a camera to new effect.

#if defined(HAVE_OP_EFFECT_CAMERA) || defined(eEDITOR)
eOP_DEF_FX(eFxCameraOp, "Camera", 'c', 1, 1, eOP_INPUTS(eOC_SCENE))
    eOP_DEINIT()
    {
        eDelete(m_rootFx);
    }

	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_POV(eFxCameraOp, povOp, "Point-of-view", eOC_POV, eTRUE,
		eOP_PAR_END))
    {
        const eScene &scene = ((eISceneOp *)getAboveOp(0))->getResult().scene;
        eDelete(m_rootFx);
        m_rootFx = new eInputEffect(scene, povOp->getResult().cam, povOp->getResult().viewport);
    }
	eOP_EXEC2_END
eOP_END(eFxCameraOp);
#endif

// Blur (effect) operator
// ----------------------
// Blurs a render target.

#if defined(HAVE_OP_EFFECT_BLUR) || defined(eEDITOR)
eOP_DEF_FX(eFxBlurOp, "Blur", 'b', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxBlurOp, m_fxBlurH);
		eOP_REGISTER_EFFECT_VAR(eFxBlurOp, m_fxBlurV);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eFxBlurOp, iterations, "Iterations", 0, eU8_MAX, 1,
        eOP_PAR_IXY(eFxBlurOp, dist, "Distance", 1, 64, 1, 1,
        eOP_PAR_ENUM(eFxBlurOp, dir, "Direction", "Horizontal|Vertical|Both", 2,
		eOP_PAR_END))))
    {
        // add horizontal blur
        if (dir == 0 || dir == 2)
        {
            m_fxBlurH.clearInputs();
            m_fxBlurH.sc.dist.set((eF32)dist.x, 0.0f);
            m_fxBlurH.setIterations(iterations);
            _appendEffect(&m_fxBlurH);
        }
    
        // add vertical blur
        if (dir == 1 || dir == 2)
        {
            m_fxBlurV.clearInputs();
            m_fxBlurV.sc.dist.set(0.0f, (eF32)dist.y);
            m_fxBlurV.setIterations(iterations);
            _appendEffect(&m_fxBlurV);
        }
    }
	eOP_EXEC2_END

    eOP_VAR(eBlurEffect m_fxBlurH);
    eOP_VAR(eBlurEffect m_fxBlurV);
eOP_END(eFxBlurOp);
#endif

// Merge (effect) operator
// -----------------------
// Merges two or more input render-targets.

#if defined(HAVE_OP_EFFECT_MERGE) || defined(eEDITOR)
eOP_DEF_FX(eFxMergeOp, "Merge", 'm', 2, 2, eOP_INPUTS(eOC_FX, eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxMergeOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_RGBA(eFxMergeOp, clearCol, "Clear color", 0, 0, 0, 255,
        eOP_PAR_ENUM(eFxMergeOp, mode, "Mode", "Additive|Subtractive|Multiplicative|Brighter|Darker", 0,
        eOP_PAR_FXY(eFxMergeOp, ratios, "Ratios", 0.0f, 1.0f, 1.0f, 1.0f,
		eOP_PAR_BOOL(eFxMergeOp, depthTest, "Depth test", eFALSE,
		eOP_PAR_END)))))
    {
        m_fx.clearInputs();
        m_fx.sc.clearCol = clearCol;
        m_fx.sc.mergeMode = mode;
        m_fx.sc.blendRatios = ratios;
		m_fx.sc.depthTest = depthTest;
        _appendEffect(&m_fx);

        for (eU32 i=1; i<getAboveOpCount(); i++)
        {
            const eIEffectOp::Result &res = ((eIEffectOp *)getAboveOp(i))->getResult();
            m_fx.addInput(res.fx);
        }
    }
	eOP_EXEC2_END

    eOP_VAR(eMergeEffect m_fx);
eOP_END(eFxMergeOp);
#endif

// Adjust (effect) operator
// ------------------------
// Adjusts brightness/contrast/color of a render target.

#if defined(HAVE_OP_EFFECT_ADJUST) || defined(eEDITOR)
eOP_DEF_FX(eFxAdjustOp, "Adjust", 'a', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxAdjustOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eFxAdjustOp, iterations, "Iterations", 0, eU8_MAX, 1,
        eOP_PAR_FLOAT(eFxAdjustOp, brightness, "Brightness", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eFxAdjustOp, contrast, "Contrast", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_RGB(eFxAdjustOp, adjCol, "Color", 255, 255, 255,
        eOP_PAR_RGB(eFxAdjustOp, subCol, "Subtract", 0, 0, 0,
        eOP_PAR_RGB(eFxAdjustOp, addCol, "Add", 0, 0, 0,
		eOP_PAR_END)))))))
    {
        m_fx.clearInputs();
        m_fx.sc.addCol = addCol;
        m_fx.sc.subCol = subCol;
        m_fx.sc.contrast = contrast;
        m_fx.sc.brightnessAdjCol = adjCol;
        m_fx.sc.brightnessAdjCol *= brightness;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eAdjustEffect m_fx);
eOP_END(eFxAdjustOp);
#endif

// Radial blur (effect) operator
// -----------------------------
// Applies a radial blur on a render target.

#if defined(HAVE_OP_EFFECT_RADIAL_BLUR) || defined(eEDITOR)
eOP_DEF_FX(eFxRadialBlurOp, "Radial blur", 'i', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxRadialBlurOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eFxRadialBlurOp, origin, "Origin", 0.0f, 1.0f, 0.5f, 0.5f,
        eOP_PAR_FLOAT(eFxRadialBlurOp, dist, "Distance", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eFxRadialBlurOp, strength, "Strength", 0.0f, eF32_MAX, 2.0f,
		eOP_PAR_INT(eFxRadialBlurOp, steps, "Steps", 1, 64, 64,
		eOP_PAR_END)))))
    {
        m_fx.clearInputs();
        m_fx.sc.origin = origin;
        m_fx.sc.dist = dist;
        m_fx.sc.strength = strength;
		m_fx.sc.steps = steps;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eRadialBlurEffect m_fx);
eOP_END(eFxRadialBlurOp);
#endif

// Fog (effect) operator
// ---------------------
// Adds fog (linear, exponential, exponential squared)
// to a render target.

#if defined(HAVE_OP_EFFECT_FOG) || defined(eEDITOR)
eOP_DEF_FX(eFxFogOp, "Fog", 'f', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxFogOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eFxFogOp, type, "Type", "Linear|Exponential|Exponential squared", 0,
        eOP_PAR_FLOAT(eFxFogOp, start, "Start", 0.01f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eFxFogOp, end, "End", 0.01f, eF32_MAX, 10.0f,
        eOP_PAR_FLOAT(eFxFogOp, density, "Density", 0.01f, eF32_MAX, 0.1f,
        eOP_PAR_RGB(eFxFogOp, col, "Color", 255, 255, 255,
		eOP_PAR_END))))))
    {
        m_fx.clearInputs();
        m_fx.sc.type = (eFogType)type;
        m_fx.sc.start = start;
        m_fx.sc.end = end;
        m_fx.sc.density = density;
        m_fx.sc.col = col;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eFogEffect m_fx);
eOP_END(eFxFogOp);
#endif

// Save (effect) operator
// ----------------------
// Copies render target into another render target,
// which can the be used for texturing purposes
// (aka render-to-texture).

#if defined(HAVE_OP_EFFECT_SAVE) || defined(eEDITOR)
eOP_DEF_FX(eFxSaveOp, "Save", 's', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxSaveOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_R2T(eFxSaveOp, r2tOp, "Render target", eOC_R2T, eTRUE,
		eOP_PAR_END))
    {
        if (r2tOp)
        {
            const eIRenderToTextureOp::Result &res = r2tOp->getResult();
            m_fx.clearInputs();
            m_fx.renderTarget = res.renderTarget;
            m_fx.depthTarget = res.depthTarget;
            _appendEffect(&m_fx);
        }
    }
	eOP_EXEC2_END

    eOP_VAR(eSaveEffect m_fx);
eOP_END(eFxSaveOp);
#endif

// Depth of field (effect) operator
// --------------------------------
// Adds a depth of field effect to a render target.
// Requires an additional normal and blurred target
// as input operators.

#if defined(HAVE_OP_EFFECT_DOF) || defined(eEDITOR)
eOP_DEF_FX(eFxDofOp, "DOF", 'o', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxDofOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eFxDofOp, focalDepth, "Focal depth", 0.01f, eF32_MAX, 5.0f,
        eOP_PAR_FLOAT(eFxDofOp, nearDofStart, "Near DOF start", 0.01f, eF32_MAX, 10.0f,
        eOP_PAR_FLOAT(eFxDofOp, nearDofDist, "Near DOF dist", 0.01f, eF32_MAX, 5.0f,
        eOP_PAR_FLOAT(eFxDofOp, farDofStart, "Far DOF start", 0.01f, eF32_MAX, 10.0f,
        eOP_PAR_FLOAT(eFxDofOp, farDofDist, "Far DOF dist", 0.01f, eF32_MAX, 5.0f,
        eOP_PAR_FLOAT(eFxDofOp, maxBlur, "Max blur", 0.01f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eFxDofOp, threshBlur, "Threshold blur", eALMOST_ZERO, eF32_MAX, 0.01f,
//        eOP_PAR_LABEL(eFxDofOp, label0, "Rings", "Rings",
        eOP_PAR_INT(eFxDofOp, rings, "Ring count", 1, 64, 3,
        eOP_PAR_INT(eFxDofOp, ringSamples, "Ring samples", 1, 64, 10,
        eOP_PAR_LABEL(eFxDofOp, label1, "Vignetting", "Vignetting",
        eOP_PAR_BOOL(eFxDofOp, vignettingActive, "Active", false,
        eOP_PAR_FLOAT(eFxDofOp, vignettingIn, "In", 0.01f, eF32_MAX, 1.0f,
        eOP_PAR_FLOAT(eFxDofOp, vignettingOut, "Out", 0.01f, eF32_MAX, 0.0f,
        eOP_PAR_FLOAT(eFxDofOp, vignettingFade, "Fade", 0.01f, eF32_MAX, 10.0f,
        eOP_PAR_LABEL(eFxDofOp, label2, "Highlighting", "Highlighting",
        eOP_PAR_FLOAT(eFxDofOp, highlightingThreshold, "Threshold", 0.01f, eF32_MAX, 0.5f,
        eOP_PAR_FLOAT(eFxDofOp, highlightingGain, "Gain", 0.01f, eF32_MAX, 1.0f,
        eOP_PAR_LABEL(eFxDofOp, label3, "Noise", "Noise",
        eOP_PAR_BOOL(eFxDofOp, noiseActive, "Active", true,
        eOP_PAR_FLOAT(eFxDofOp, noiseAmount, "Amount", 0.01f, eF32_MAX, 1.0f,
		eOP_PAR_END)))))))))))))))))))))
    {
        m_fx.clearInputs();
        m_fx.sc.focalDepth = focalDepth;
        m_fx.sc.nearDofStart = nearDofStart;
        m_fx.sc.nearDofDist = nearDofDist;
        m_fx.sc.farDofStart = farDofStart;
        m_fx.sc.farDofDist = farDofDist;
        m_fx.sc.maxBlur = maxBlur;
        m_fx.sc.threshBlur = threshBlur;
        m_fx.sc.rings = rings; 
        m_fx.sc.ringSamples = ringSamples;
        m_fx.sc.vignettingActive = (vignettingActive ? 1 : 0);
        m_fx.sc.vignettingIn = vignettingIn;
        m_fx.sc.vignettingOut = vignettingOut;
        m_fx.sc.vignettingFade = vignettingFade;
        m_fx.sc.highlightingThreshold = highlightingThreshold;
        m_fx.sc.highlightingGain = highlightingGain;
        m_fx.sc.noiseActive = (noiseActive ? 1 : 0);
        m_fx.sc.noiseAmount = noiseAmount*0.0001f;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eDofEffect m_fx);
eOP_END(eFxDofOp);
#endif

// Downsample (effect) operator
// ----------------------------
// Downsamples a render target.

#if defined(HAVE_OP_EFFECT_DOWNSAMPLE) || defined(eEDITOR)
eOP_DEF_FX(eFxDownsampleOp, "Downsample", 'd', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxDownsampleOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eFxDownsampleOp, iters, "Iterations", 0, eU8_MAX, 1,
        eOP_PAR_FXY(eFxDownsampleOp, amount, "Amount", 0.01f, 1.0f, 0.5f, 0.5f,
		eOP_PAR_END)))
    {
        m_fx.clearInputs();
        m_fx.setIterations(iters);
        m_fx.amount = amount;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eDownsampleEffect m_fx);
eOP_END(eFxDownsampleOp);
#endif

// SSAO (effect) operator
// ----------------------
// Adds screen space ambient occlusion to a render target.
// Requires an additional noise lookup texture linked.

#if defined(HAVE_OP_EFFECT_SSAO) || defined(eEDITOR)
eOP_DEF_FX(eFxSsaoOp, "SSAO", 's', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxSsaoOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eFxSsaoOp, scale, "Scale", 0.01f, eF32_MAX, 0.5f,
        eOP_PAR_FLOAT(eFxSsaoOp, intensity, "Intensity", 0.01f, eF32_MAX, 2.5f,
        eOP_PAR_FLOAT(eFxSsaoOp, bias, "Bias", 0.01f, eF32_MAX, 0.25f,
        eOP_PAR_FLOAT(eFxSsaoOp, radius, "Radius", 0.01f, eF32_MAX, 0.5f,
        eOP_PAR_FLOAT(eFxSsaoOp, noiseAmount, "Noise amount", 0.01f, eF32_MAX, 1.0f,
		eOP_PAR_END))))))
    {
        m_fx.clearInputs();
        m_fx.sc.scale = scale;
        m_fx.sc.intensity = intensity/16.0f;
        m_fx.sc.bias = bias;
        m_fx.sc.radius = radius;
        m_fx.sc.noiseAmount = noiseAmount;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eSsaoEffect m_fx);
eOP_END(eFxSsaoOp);
#endif

// FXAA (effect) operator
// ---------------------
// Adds screen space ambient occlusion to a render target.
// Requires an additional noise lookup texture linked.

#if defined(HAVE_OP_EFFECT_FXAA) || defined(eEDITOR)
eOP_DEF_FX(eFxFxaaOp, "FXAA", 'x', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxFxaaOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        m_fx.clearInputs();
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eFxaaEffect m_fx);
eOP_END(eFxFxaaOp);
#endif

// Color Grading (effect) operator
// -------------------------------
// Adds color grading to a render target.
// Requires an additional color grading
// lookup texture linked.

#if defined(HAVE_OP_EFFECT_COLOR_GRADING) || defined(eEDITOR)
eOP_DEF_FX(eFxColorGradingOp, "Color grading", ' ', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxColorGradingOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_BMP(eFxColorGradingOp, lookupMapOp, "Lookup map", eOC_BMP, eTRUE,
		eOP_PAR_END))
    {
        m_fx.clearInputs();
        m_fx.lookupMap = lookupMapOp->getResult().uav->tex;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eColorGradingEffect  m_fx);
eOP_END(eFxColorGradingOp);
#endif

// Distort (effect) operator
// -------------------------
// Adds distortion to a render target.
// Requires an additional distortion
// lookup normal texture linked.

#if defined(HAVE_OP_EFFECT_DISTORT) || defined(eEDITOR)
eOP_DEF_FX(eFxDistortOp, "Distort", 's', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxDistortOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eFxDistortOp, intensity, "Intensity", -eF32_MAX, eF32_MAX, 1.0f, 1.0f,
        eOP_PAR_FXY(eFxDistortOp, offset, "Offset", -eF32_MAX, eF32_MAX, 0.0f, 0.0f,
        eOP_PAR_LINK_BMP(eFxDistortOp, distMapOp, "Distort map", eOC_BMP, eTRUE,
		eOP_PAR_END))))
    {
        m_fx.sc.intensity = intensity;
        m_fx.sc.offset = offset;
        m_fx.distMap = distMapOp->getResult().uav->tex;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eDistortEffect m_fx);
eOP_END(eFxDistortOp);
#endif

// Ripple (effect) operator
// -------------------------

#if defined(HAVE_OP_EFFECT_RIPPLE) || defined(eEDITOR)
eOP_DEF_FX(eFxRippleOp, "Ripple", 'r', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxRippleOp, m_fxRipple);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eFxRippleOp, iterations, "Iterations", 0, eU8_MAX, 1,
        eOP_PAR_FLOAT(eFxRippleOp, ampli, "Amplitude", 0.0f, eF32_MAX, 0.5f,
        eOP_PAR_FLOAT(eFxRippleOp, length, "Length", 0.01f, eF32_MAX, 5.0f,
        eOP_PAR_FLOAT(eFxRippleOp, speed, "Speed", 0.0f, eF32_MAX, 5.0f,
        eOP_PAR_FLOAT(eFxRippleOp, time, "Time", 0.0f, eF32_MAX, 0.0f,
        eOP_PAR_FXY(eFxRippleOp, offset, "Offset", 0.0f, 1.0f, 0.5f, 0.5f,
        eOP_PAR_ENUM(eFxRippleOp, mode, "Mode", "Standard|Concentric", 0,
		eOP_PAR_END))))))))
    {
        m_fxRipple.clearInputs();
        m_fxRipple.setIterations(iterations);
        m_fxRipple.sc.ampli = ampli / 10.0f;
        m_fxRipple.sc.length = length;
        m_fxRipple.sc.speed = speed;
        m_fxRipple.sc.time = time;
        m_fxRipple.sc.offset = offset;
        m_fxRipple.sc.mode = mode;
        _appendEffect(&m_fxRipple);
    }
	eOP_EXEC2_END

    eOP_VAR(eRippleEffect m_fxRipple);
eOP_END(eFxRippleOp);
#endif

// Terrain (effect) operator
// ----------------------
// Renders raymarched terrain

#if defined(HAVE_OP_EFFECT_TERRAIN) || defined(eEDITOR)
eOP_DEF_FX(eFxTerrainOp, "Terrain", 't', 1, 2, eOP_INPUTS(eOC_FX, eOC_FX))
    eOP_INIT()
    {
		eOP_REGISTER_EFFECT_VAR(eFxTerrainOp, m_fx);
        eOP_PARAM_ADD_LINK(noalloc, par0, "Point-of-view", eOC_POV, eTRUE);

        eOP_PARAM_ADD_FLOAT(par1, "Time", 0.f, eF32_MAX, 0.f);
        eOP_PARAM_ADD_FLOAT(par2, "Range", eF32_MIN, eF32_MAX, 1000.0f);

        eOP_PARAM_ADD_FXYZ(par3, "Planet center", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f);
        eOP_PARAM_ADD_FLOAT(par4, "Planet radius", 0.0f, eF32_MAX, 10.0f);
        
        eOP_PARAM_ADD_FLOAT(par5, "Fog amount", 0.0f, 1.0f, 0.0f);
        eOP_PARAM_ADD_RGB(par6, "Sky color", 178, 178, 204);

        eOP_PARAM_ADD_FLOAT(par7, "Shadow amount", 0.0f, eF32_MAX, 1.0f);

        eOP_PARAM_ADD_FXYZ(par8, "Sun position", eF32_MIN, eF32_MAX, 0.0f, -1.0f, 0.0f);
        eOP_PARAM_ADD_RGB(par9, "Sun color", 255, 255, 255);

        eOP_PARAM_ADD_FLOAT(par10, "Water level", 0.0f, eF32_MAX, 0.5f);
        eOP_PARAM_ADD_FLOAT(par11, "Water amplitude", 0.0f, 2.0f, 1.0f);
        eOP_PARAM_ADD_FLOAT(par12, "Water frequency", 0.0f, 10.0f, 1.0f);
        eOP_PARAM_ADD_FLOAT(par13, "Water shininess", 0.0f, 1.0f, 0.9f);

        eOP_PARAM_ADD_FLOAT(par14, "Sky level", 0.0f, eF32_MAX, 1.0f);
        eOP_PARAM_ADD_FLOAT(par15, "Sky horizon", 0.0f, 20.0f, 1.0f);
        eOP_PARAM_ADD_FLOAT(par16, "Cloud scale", 0.0f, 1.0f, 0.5f);
        eOP_PARAM_ADD_FLOAT(par17, "Cloud treshold", 0.0f, 1.0f, 0.5f);

		// altitude colors
//		eOP_PARAM_ADD_LABEL(par18, "Colors for Earth Grass Rock and Snow", "Colors Earth Grass Rock Snow");
        eOP_PARAM_ADD_RGB(par19, "Snow color", 255, 255, 255);
		eOP_PARAM_ADD_RGB(par20, "Rock color", 80, 80, 80);
		eOP_PARAM_ADD_RGB(par21, "Grass color", 92, 192, 92);
		eOP_PARAM_ADD_RGB(par22, "Earth color", 128, 32, 0);

		// altitude levels                                earth  grass   rock   snow
//		eOP_PARAM_ADD_LABEL(par23, "Altitudes for Earth Grass Rock and Snow", "Altitude Earth Grass Rock Snow");
		eOP_PARAM_ADD_FXYZW(par24, "EGRS level max", 0.0f, 1.0f, 0.03f, 0.12f, 1.00f, 1.00f);
		eOP_PARAM_ADD_FXYZW(par25, "EGRS level min", 0.0f, 1.0f, 0.01f, 0.00f, 0.10f, 0.60f);
		eOP_PARAM_ADD_FXYZW(par26, "EGRS level rel", 0.0f, 1.0f, 0.01f, 0.01f, 0.01f, 0.01f);

		// slope levels                                    earth  grass  rock   snow
//		eOP_PARAM_ADD_LABEL(par27, "Slopes for Earth Grass Rock and Snow", "Slope Earth Grass Rock Snow");
		eOP_PARAM_ADD_FXYZW(par28, "EGRS slope max", 0.0f, 1.57f, 0.17f, 0.17f, 1.57f, 1.04f);
		eOP_PARAM_ADD_FXYZW(par29, "EGRS slope min", 0.0f, 1.57f, 0.00f, 0.00f, 0.52f, 0.00f);
		eOP_PARAM_ADD_FXYZW(par30, "EGRS slope rel", 0.0f, 1.57f, 0.17f, 0.17f, 0.17f, 0.17f);

		// skew levels                                     earth  grass  rock   snow
//		eOP_PARAM_ADD_LABEL(par31, "Skew for Earth Grass Rock and Snow", "Skew Earth Grass Rock Snow");
		eOP_PARAM_ADD_FXYZW(par32, "EGRS skew amo", 0.0f, 9.00f,  0.00f, 0.00f, 1.00f, 3.00f);
		eOP_PARAM_ADD_FXYZW(par33, "EGRS skew azi", 0.0f, 1.57f,  1.57f, 1.57f, 1.57f, 1.57f);

//		eOP_PARAM_ADD_LABEL(par34, "Scale and warping", "Scale and warping");
        eOP_PARAM_ADD_FXYZ(par35, "Terrain scale", 0.0f, eF32_MAX, 1.0f, 1.0f, 1.0f);
        eOP_PARAM_ADD_FXYZ(par36, "Noise scale", 0.0f, eF32_MAX, 1.0f, 1.0f, 1.0f);
        eOP_PARAM_ADD_FLOAT(par37, "Terrain warping", 0.0f, eF32_MAX, 0.0f);
        
//		eOP_PARAM_ADD_LABEL(par38, "Texture maps", "Texture maps");
        eOP_PARAM_ADD_LINK(noalloc,par39, "Noise map", eOC_BMP, eTRUE);
        eOP_PARAM_ADD_LINK(noalloc,par40, "Terrain map", eOC_BMP, eTRUE);
        eOP_PARAM_ADD_LINK(noalloc,par41, "Grass texture", eOC_BMP, eFALSE);
        eOP_PARAM_ADD_LINK(noalloc,par42, "Earth texture", eOC_BMP, eFALSE);
        eOP_PARAM_ADD_LINK(noalloc,par43, "Rock texture", eOC_BMP, eFALSE);
        eOP_PARAM_ADD_LINK(noalloc,par44, "Foam texture", eOC_BMP, eFALSE);
    }

    eOP_EXEC(const eIPovOp *povOp, eF32 time, eF32 range, const eVector3 &planetCenter, eF32 planetRadius, 
             eF32 fogAmount, const eColor &skyColor,
             eF32 shadowAmount, const eVector3 &sunPosition, const eColor &sunColor,
             eF32 waterLevel, eF32 waterAmplitude, eF32 waterFrequency, eF32 waterShininess,
             eF32 skyLevel, eF32 skyHorizon, eF32 cloudScale, eF32 cloudTreshold,
			 const eColor &colSnow, const eColor &colRock, const eColor &colGrass, const eColor &colEarth,

			 const eVector4 &altitudeMax, const eVector4 &altitudeMin, const eVector4 &altitudeRel,
			 const eVector4 &slopeMax, const eVector4 &slopeMin, const eVector4 &slopeRel,
			 const eVector4 &skewAmo, const eVector4 &skewAzim,

			 const eVector3 &terrainScale,
             const eVector3 &noiseScale, eF32 terrainWarping, eIBitmapOp *noiseMapOp, eIBitmapOp *terrainMapOp,
             eIBitmapOp *grassTextureOp, eIBitmapOp *earthTextureOp, eIBitmapOp *rockTextureOp, eIBitmapOp *foamTextureOp)
    {  
        const eMatrix4x4 &view = povOp->getResult().cam.getInvViewMatrix();
		const eVector3 fov = eVector3(eCot(eDegToRad(povOp->getResult().cam.getFieldOfViewY())*0.5f), 0, povOp->getResult().cam.getAspectRatio());
        const eVector3 uu = eVector3(view.m11, view.m12, view.m13);
        const eVector3 vv = eVector3(view.m21, view.m22, view.m23);
        const eVector3 ww = eVector3(view.m31, view.m32, view.m33);

        m_fx.clearInputs();
        m_fx.noiseMap = noiseMapOp->getResult().uav->tex;
        m_fx.terrainMap = terrainMapOp->getResult().uav->tex;
        m_fx.grassTexture = grassTextureOp->getResult().uav->tex;
        m_fx.earthTexture = earthTextureOp->getResult().uav->tex;
        m_fx.rockTexture = rockTextureOp->getResult().uav->tex;
        m_fx.foamTexture = foamTextureOp->getResult().uav->tex;
        m_fx.sc.time = time;
		m_fx.sc.camPosition = povOp->getResult().cam.getWorldPos();
		m_fx.sc.camFov = fov;
        m_fx.sc.uu = uu;
        m_fx.sc.vv = vv;
        m_fx.sc.ww = ww;
        m_fx.sc.camRange = range;
        m_fx.sc.sunPosition = sunPosition;
        m_fx.sc.sunColor = sunColor;
        m_fx.sc.terrainScale = terrainScale;
        m_fx.sc.noiseScale = noiseScale;
        m_fx.sc.waterLevel = waterLevel;
        m_fx.sc.waterAmplitude = waterAmplitude;
        m_fx.sc.waterFrequency = waterFrequency;
        m_fx.sc.waterShininess = waterShininess;   
        m_fx.sc.colSnow = colSnow;
	    m_fx.sc.colRock = colRock;
        m_fx.sc.colGrass = colGrass;
        m_fx.sc.colEarth = colEarth;
        m_fx.sc.fogAmount = fogAmount;
        m_fx.sc.skyColor = skyColor;
        m_fx.sc.shadowAmount = shadowAmount;
        m_fx.sc.terrainWarping = terrainWarping;
        m_fx.sc.planetCenter = planetCenter;
        m_fx.sc.planetRadius = planetRadius;
	    m_fx.sc.altitudeMax = altitudeMax;
	    m_fx.sc.altitudeMin = altitudeMin;
	    m_fx.sc.altitudeRel = altitudeRel;
	    m_fx.sc.slopeMax = slopeMax;
	    m_fx.sc.slopeMin = slopeMin;
	    m_fx.sc.slopeRel = slopeRel;
	    m_fx.sc.skewAmo = skewAmo;
	    m_fx.sc.skewAzim = skewAzim;
        m_fx.sc.skyLevel = skyLevel;
        m_fx.sc.skyHorizon = skyHorizon;
        m_fx.sc.cloudScale = cloudScale;
        m_fx.sc.cloudTreshold = cloudTreshold;

        _appendEffect(&m_fx);

        for (eU32 i=1; i<getAboveOpCount(); i++)
        {
            const eIEffectOp::Result &res = ((eIEffectOp *)getAboveOp(i))->getResult();
            m_fx.addInput(res.fx);
        }
    }

    eOP_VAR(eTerrainEffect m_fx);
eOP_END(eFxTerrainOp);
#endif

// Planet (effect) operator
// ----------------------
// Renders raymarched planet

#if defined(HAVE_OP_EFFECT_PLANET) || defined(eEDITOR)
eOP_DEF_FX(eFxPlanetOp, "Planet", 'p', 1, 2, eOP_INPUTS(eOC_FX, eOC_FX))

    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxPlanetOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_POV(eFxPlanetOp, povOp, "Point-of-view", eOC_POV, eTRUE,
        eOP_PAR_FLOAT(eFxPlanetOp, time, "Time", 0.f, eF32_MAX, 20.0f,
        eOP_PAR_FLOAT(eFxPlanetOp, planetRadius, "Planet radius", 0.f, eF32_MAX, 100.f,
        eOP_PAR_FLOAT(eFxPlanetOp, planetHillTop, "Planet hill top", 0.f, 1.f, 0.1f,
        eOP_PAR_FLOAT(eFxPlanetOp, planetSeed, "Planet seed", 0.f, eF32_MAX, 0.1f,
        eOP_PAR_FLOAT(eFxPlanetOp, planetFreq, "Planet freq", 0.f, eF32_MAX, 16.f,

        eOP_PAR_FXYZ(eFxPlanetOp, sunPosition, "Sun position", eF32_MIN, eF32_MAX, 0.f, -1.f, 0.f,

        eOP_PAR_RGB(eFxPlanetOp, shallowColor, "Shallow color", 255, 255, 255,
        eOP_PAR_RGB(eFxPlanetOp, shoreColor, "Low color", 255, 255, 255,
        eOP_PAR_RGB(eFxPlanetOp, sandColor, "Hi color", 255, 255, 255,
        eOP_PAR_RGB(eFxPlanetOp, grassColor, "Grass color", 255, 255, 255,
        eOP_PAR_RGB(eFxPlanetOp, dirtColor, "Dirt color", 255, 255, 255,

        eOP_PAR_FLOAT(eFxPlanetOp, cloudScale, "Cloud scale", 0.f, 1.f, 0.5f,
        eOP_PAR_FLOAT(eFxPlanetOp, cloudTreshold, "Cloud treshold", 0.f, 1.f, 0.5f,
        eOP_PAR_FXYZW(eFxPlanetOp, waterParams, "Water Lev,Amp,Frq,Tra", 0.f, eF32_MAX, 0.5f, 1.f, 1.f, 0.15f,
        eOP_PAR_FLOAT(eFxPlanetOp, waterExtinction, "Water extinction", 0.f, eF32_MAX, 1.f,
        eOP_PAR_FLOAT(eFxPlanetOp, snowFrequency, "Snow frequency", 0.f, eF32_MAX, 1.f,
        eOP_PAR_FLOAT(eFxPlanetOp, textureNoise, "Texture noise", 0.f, eF32_MAX, 0.f,
        eOP_PAR_FLOAT(eFxPlanetOp, scatterStrength, "Scatter str", 0.f, eF32_MAX, 1.f,
        eOP_PAR_FLOAT(eFxPlanetOp, fogAmount, "Fog amount", 0.f, 1.f, 0.f,
		eOP_PAR_END)))))))))))))))))))))
    {
        const eMatrix4x4 &invView = povOp->getResult().cam.getInvViewMatrix();
		const eVector3 fov = eVector3(povOp->getResult().cam.getAspectRatio(), -1, eCot(eDegToRad(povOp->getResult().cam.getFieldOfViewY())*0.5f));

        m_fx.clearInputs();
        m_fx.sc.time = time;
        m_fx.sc.invViewMatrix = invView;
		m_fx.sc.camFov = fov;

        m_fx.sc.planetRadius = planetRadius;
        m_fx.sc.planetHillTop = planetHillTop;
        m_fx.sc.planetSeed = planetSeed;
        m_fx.sc.planetFreq = planetFreq;

        m_fx.sc.sunPosition = sunPosition;

        m_fx.sc.shallowColor = shallowColor;
        m_fx.sc.shoreColor = shoreColor;
        m_fx.sc.sandColor = sandColor;
        m_fx.sc.grassColor = grassColor;
        m_fx.sc.dirtColor = dirtColor;

        m_fx.sc.waterLevel = waterParams.x;
        m_fx.sc.waterAmplitude = waterParams.y;
        m_fx.sc.waterFrequency = waterParams.z;
        m_fx.sc.waterTransparency = waterParams.w;

        m_fx.sc.cloudScale = cloudScale;
        m_fx.sc.cloudTreshold = cloudTreshold;

        m_fx.sc.waterExtinction = waterExtinction;
        m_fx.sc.snowFrequency = snowFrequency;

        m_fx.sc.textureNoise = textureNoise;

        m_fx.sc.scatterStrength = scatterStrength;
        m_fx.sc.fogAmount = fogAmount;

        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(ePlanetEffect m_fx);
eOP_END(eFxPlanetOp);
#endif

// Raytrace (effect) operator
// ----------------------
#if defined(HAVE_OP_EFFECT_RAYTRACE) || defined(eEDITOR)
eOP_DEF_FX(eFxRaytraceOp, "Raytrace", 'y', 1, 2, eOP_INPUTS(eOC_FX, eOC_FX))

    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxRaytraceOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_POV(eFxRaytraceOp, povOp, "Point-of-view", eOC_POV, eTRUE,
        eOP_PAR_FLOAT(eFxRaytraceOp, time, "Time", eF32_MIN, eF32_MAX, 1.f,
        eOP_PAR_FLOAT(eFxRaytraceOp, sphereRadius, "Sphere radius", 0.f, eF32_MAX, 100.f,
		eOP_PAR_END))))
    {
        const eMatrix4x4 &invView = povOp->getResult().cam.getInvViewMatrix();

        m_fx.clearInputs();

        m_fx.sc.invViewMatrix = invView;
		m_fx.sc.camFov = eVector3(povOp->getResult().cam.getAspectRatio(), -1, eCot(eDegToRad(povOp->getResult().cam.getFieldOfViewY())*0.5f));;

		m_fx.sc.time = time;
        m_fx.sc.sphereRadius = sphereRadius;

		_appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eRaytraceEffect m_fx);
eOP_END(eFxRaytraceOp);
#endif

// Microscope (effect) operator
// -------------------------
#if defined(HAVE_OP_EFFECT_MICROSCOPE) || defined(eEDITOR)
eOP_DEF_FX(eFxMicroscopeOp, "Microscope", ' ', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxMicroscopeOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_RGB(eFxMicroscopeOp, backgroundCol, "Background color", 12, 24, 32,
        eOP_PAR_RGB(eFxMicroscopeOp, foregroundCol, "Foreground color", 230, 255, 230,
		eOP_PAR_RGB(eFxMicroscopeOp, foregroundCol2, "Foreground color 2", 230, 255, 230,
		eOP_PAR_BOOL(eFxMicroscopeOp, useTwoColors, "Use two colors", eFALSE,
        eOP_PAR_FLOAT(eFxMicroscopeOp, depth, "Depth", 0.0f, eF32_MAX, 20.0f,
        eOP_PAR_FLOAT(eFxMicroscopeOp, noiseAmount, "Noise amount", 0.0f, eF32_MAX, 0.1f,
		eOP_PAR_END)))))))
    {
        m_fx.clearInputs();

        m_fx.sc.foregroundCol1 = m_fx.sc.foregroundCol2 = foregroundCol;
		if (useTwoColors)
			m_fx.sc.foregroundCol2 = foregroundCol2;

		m_fx.sc.backgroundCol = backgroundCol;
        m_fx.sc.depth = depth;
        m_fx.sc.noiseAmount = noiseAmount;

        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eMicroscopeEffect m_fx);
eOP_END(eFxMicroscopeOp);
#endif

// Interference (effect) operator
// ------------------------------

#if defined(HAVE_OP_EFFECT_INTERFERENCE) || defined(eEDITOR)
eOP_DEF_FX(eFxInterferenceOp, "Interference", ' ', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxInterferenceOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eFxInterferenceOp, amount, "Amount", 0.0f, eF32_MAX, 1.0f, 1.0f,
        eOP_PAR_FLOAT(eFxInterferenceOp, time, "Time", 0.0f, eF32_MAX, 0.0f,
		eOP_PAR_END)))
    {
        m_fx.clearInputs();
        m_fx.sc.amount = amount;
        m_fx.sc.time = time;
        m_fx.sc.moveR = eVector2(eRandomF(-1.0f, 1.0f), eRandomF(-1.0f, 1.0f));
        m_fx.sc.moveG = eVector2(eRandomF(-1.0f, 1.0f), eRandomF(-1.0f, 1.0f));
        m_fx.sc.moveB = eVector2(eRandomF(-1.0f, 1.0f), eRandomF(-1.0f, 1.0f));
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eInterferenceEffect m_fx);
eOP_END(eFxInterferenceOp);
#endif

// Space (effect) operator
// -----------------------

#if defined(HAVE_OP_EFFECT_SPACE) || defined(eEDITOR)
eOP_DEF_FX(eFxSpaceOp, "Space", ' ', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxSpaceOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        m_fx.clearInputs();
        m_fx.sc.param = 0.0f;
        _appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eSpaceEffect m_fx);
eOP_END(eFxSpaceOp);
#endif

// Motion Blur (effect) operator
// ----------------------
// Renders motion blur

#if defined(HAVE_OP_EFFECT_MOTION_BLUR) || defined(eEDITOR)
eOP_DEF_FX(eFxMotionBlurOp, "Motion Blur", 'm', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxMotionBlurOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_POV(eFxMotionBlurOp, povOp, "Point-of-view", eOC_POV, eTRUE,
        eOP_PAR_FLOAT(eFxMotionBlurOp, strength, "Strength", 0.0f, eF32_MAX, 0.0f,
		eOP_PAR_END)))
    {
        const eMatrix4x4 &proj = povOp->getResult().cam.getProjMatrix();
        const eMatrix4x4 &view = povOp->getResult().cam.getViewMatrix();
        const eMatrix4x4 &invView = povOp->getResult().cam.getInvViewMatrix();

        m_fx.clearInputs();
        m_fx.sc.lastViewMatrix = m_lastView;
        m_fx.sc.invViewMatrix = invView;
        m_fx.sc.projMatrix = proj;
        m_fx.sc.strength = strength;
        _appendEffect(&m_fx);

        m_lastView = view;
    }
	eOP_EXEC2_END

    eOP_VAR(eMotionBlurEffect m_fx);
    eOP_VAR(eMatrix4x4 m_lastView);
eOP_END(eFxMotionBlurOp);
#endif

// Halo (effect) operator
// ----------------------
// Renders a halo

#if defined(HAVE_OP_EFFECT_HALO) || defined(eEDITOR)
eOP_DEF_FX(eFxHaloOp, "Halo", 'h', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxHaloOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_LINK_POV(eFxHaloOp, povOp, "Point-of-view", eOC_POV, eTRUE,
		eOP_PAR_FXYZ(eFxHaloOp, position, "Position", eF32_MIN, eF32_MAX, 0.0f, 0.0f, 0.0f,
		eOP_PAR_RGB(eFxHaloOp, color, "Color", 255, 255, 255,
		eOP_PAR_FXY(eFxHaloOp, size, "Size", 0.0f, eF32_MAX, 0.4f, 0.4f,
		eOP_PAR_FLOAT(eFxHaloOp, power, "Power", 1.0f, eF32_MAX, 1.0f,
		eOP_PAR_BOOL(eFxHaloOp, occlusionTest, "Occlusion test", 0,
		eOP_PAR_END)))))))
    {
        const eMatrix4x4 &proj = povOp->getResult().cam.getProjMatrix();
        const eMatrix4x4 &view = povOp->getResult().cam.getViewMatrix();

		eVector4 viewPos = eVector4(position, 1.0f) * view;
		eVector4 projectedPos = viewPos * proj;
		projectedPos.x = ((projectedPos.x / projectedPos.w) + 1.0f) / 2.0f;
		projectedPos.y = 1.0f - ((projectedPos.y / projectedPos.w) + 1.0f) / 2.0f;
		projectedPos.z = viewPos.z;

		if (projectedPos.x > -size.x && projectedPos.x < 1.0f + size.x &&
			projectedPos.y > -size.y && projectedPos.y < 1.0f + size.y &&
			projectedPos.z > 0.0f)
		{
			m_fx.clearInputs();
			m_fx.sc.color = color;
			m_fx.sc.position = projectedPos;
			m_fx.sc.size = size;
			m_fx.sc.power = power;
			m_fx.sc.occlusionTest = occlusionTest;
			_appendEffect(&m_fx);
		}
    }
	eOP_EXEC2_END

    eOP_VAR(eHaloEffect m_fx);
eOP_END(eFxHaloOp);
#endif

// Caustics (effect) operator
// ----------------------
// Renders underwater style caustics

#if defined(HAVE_OP_EFFECT_CAUSTICS) || defined(eEDITOR)
eOP_DEF_FX(eFxCausticsOp, "Caustics", ' ', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxCausticsOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
		eOP_PAR_LINK_POV(eFxHaloOp, povOp, "Point-of-view", eOC_POV, eTRUE,
		eOP_PAR_FLOAT(eFxCausticsOp, scale, "Scale", 0.0f, eF32_MAX, 1.0f,
		eOP_PAR_FLOAT(eFxCausticsOp, time, "Time", 0.0f, eF32_MAX, 0.0f,
		eOP_PAR_FLOAT(eFxCausticsOp, amplitude, "Amplitude", 0.0f, eF32_MAX, 0.0f,
		eOP_PAR_END)))))
    {
		const eMatrix4x4 &invView = povOp->getResult().cam.getInvViewMatrix();
		const eMatrix4x4 &transView = povOp->getResult().cam.getViewMatrix().transposed();

		m_fx.clearInputs();
		m_fx.sc.invViewMatrix = invView;
		m_fx.sc.transViewMatrix = transView;
		m_fx.sc.scale = scale;
		m_fx.sc.time = time;
		m_fx.sc.amplitude = amplitude;
		_appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eCausticsEffect m_fx);
eOP_END(eFxCausticsOp);
#endif

// Debug (effect) operator
// ----------------------
// Shows geometry buffer contents

#if defined(HAVE_OP_EFFECT_DEBUG) || defined(eEDITOR)
eOP_DEF_FX(eFxDebugOp, "Debug", ' ', 1, 1, eOP_INPUTS(eOC_FX))
    eOP_INIT() {
		eOP_REGISTER_EFFECT_VAR(eFxDebugOp, m_fx);
    }
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eFxDebugOp, depthRange, "Depth range", 0.0f, eF32_MAX, 0.0f, 100.0f,
		eOP_PAR_END))
    {
		m_fx.clearInputs();
		m_fx.sc.depthRange = depthRange;
		_appendEffect(&m_fx);
    }
	eOP_EXEC2_END

    eOP_VAR(eDebugEffect m_fx);
eOP_END(eFxDebugOp);
#endif