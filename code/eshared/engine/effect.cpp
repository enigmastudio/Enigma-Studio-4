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

#include "../system/system.hpp"
#include "../math/math.hpp"
#include "engine.hpp"

eIEffect::TargetArray eIEffect::m_targetPool;

#ifdef ePLAYER
	eBool						eIEffect::m_doPostponeShaderLoading = eFALSE;
	eArray<const eChar*>		eIEffect::m_pixelShadersToLoad;
	void						eIEffect::addPixelShader(const eChar* src) {
		if(m_doPostponeShaderLoading)
			m_pixelShadersToLoad.append(src);
		else
			eGfx->loadPixelShader(src);
	}

	void						eIEffect::loadPostponedPixelShaders() {
		for(eU32 i = 0; i < m_pixelShadersToLoad.size(); i++) 
			eGfx->loadPixelShader(m_pixelShadersToLoad[i]);
		m_pixelShadersToLoad.clear();
	}
#endif

eIEffect::eIEffect() :
    m_viewport(0.0f, 0.0f, 1.0f, 1.0f)
{
	eSHADERPTR(vs_quad);
	m_targetPool.reserve(100);
}

#ifdef eEDITOR
void eIEffect::_addShader(const eString& str, const char* ptr) {
	eEFFECT_SHADER_ENTRY& e = m_usedShaders.append();
	e.name = str;
	e.shaderContent = ptr;
}
#endif


void eIEffect::run(eF32 time, eTexture2d *target, eTexture2d *depthTarget)
{
    ePROFILER_FUNC();
    eASSERT(time >= 0.0f);

    eRenderState &rs = eGfx->freshRenderState();
    rs.vs = eGfx->loadVertexShader(eSHADER(vs_quad));
    rs.targets[0] = target;
	rs.depthTarget = depthTarget;

    const eSize &targetSize = eSize(target->width, target->height);//!eGfx->getWndSize();
    Target *resTarget = _runHierarchy(time, targetSize);

    if (m_viewport != eVector4(0.0f, 0.0f, 1.0f, 1.0f))
    {
        const eRect r = _viewportToRect(m_viewport, targetSize);
        eGfx->clear(eCM_COLOR, eCOL_BLACK);
        eRenderer->renderQuad(r, targetSize, resTarget->colTarget);
    }
    else
        eRenderer->renderQuad(eRect(0, 0, targetSize.width, targetSize.height), targetSize, resTarget->colTarget);

    resTarget->refCount--;

    _garbageCollectTargets();
}

void eIEffect::addInput(eIEffect *fx)
{
    m_inputFx.append(fx);
	fx->addOutput(this);
    m_viewport = fx->m_viewport;
}

void eIEffect::addOutput(eIEffect *fx)
{
    m_outputFx.append(fx);
}

void eIEffect::clearInputs()
{
    m_inputFx.clear();
	m_outputFx.clear();
}

void eIEffect::shutdown()
{
    for (eU32 i=0; i<m_targetPool.size(); i++)
    {
        Target &target = m_targetPool[i];
        eASSERT(!target.refCount);
        eGfx->removeTexture2d(target.colTarget);
    }

    m_targetPool.clear();
}

eIEffect::Target * eIEffect::_findTarget(ePtr owner)
{
	for (eU32 i=0; i<m_targetPool.size(); i++)
	{
		Target &target = m_targetPool[i];
		if (target.owner == owner)
		{
			return &target;
		}
	}

	return nullptr;
}

eIEffect::Target * eIEffect::_acquireTarget(const eSize &size, const eCamera *cam, eTextureFormat format)
{
    eASSERT(size.width > 0 && size.height > 0);

    // does an unused target exist?
	const eCamera *lookForCam = cam;
	while(eTRUE)
	{
		for (eU32 i=0; i<m_targetPool.size(); i++)
		{
			Target &target = m_targetPool[i];
			if (eSize(target.colTarget->width, target.colTarget->height) == size && !target.refCount && target.format == format && target.owner == lookForCam)
			{
				// yes, so clear to black if it's not a target of a camera, we need 
				if (lookForCam == nullptr)
				{
					/*eRenderState &rs = eGfx->pushRenderState();
					rs.targets[0] = target.colTarget;
					eGfx->clear(eCM_COLOR, eCOL_BLACK);
					eGfx->popRenderState();*/
				}
				target.refCount++;
				return &target;
			}
		}

		if (lookForCam == nullptr)
			break;

		lookForCam = nullptr;
	}

    // no, so create and return a new one
	Target &target = m_targetPool.append();
    target.colTarget = eGfx->addTexture2d(size.width, size.height, eTEX_TARGET, format);

	if (size != eGfx->getWndSize())
		target.depthTarget = eGfx->addTexture2d(size.width, size.height, eTEX_NOMIPMAPS, eTFO_DEPTH32F);
	else
		target.depthTarget = nullptr;

    target.refCount = 1;
    target.format = format;
    target.wasUsed = eTRUE;

    return &target;
}

void eIEffect::_releaseTarget(Target *target)
{
    target->refCount--;
}

eRect eIEffect::_viewportToRect(const eVector4 &vp, const eSize &size)
{
    eRect r(eFtoL(vp.x*size.width),
            eFtoL(size.height-vp.y*size.height),
            eFtoL(vp.z*size.width),
            eFtoL(size.height-vp.w*size.height));

    r.normalize();

    // avoids crash because of width or height being 0
    if (!r.getWidth())
        r.setWidth(1);
    if (!r.getHeight())
        r.setHeight(1);

    return r;
}

void eIEffect::_copyTarget(Target *dst, const eRect &dstRect, Target *src)
{
    eRenderState &rs = eGfx->getRenderState();
    rs.targets[0] = dst->colTarget;
	rs.depthTarget = dst->depthTarget;
    eRenderer->renderQuad(dstRect, eSize(dst->colTarget->width, dst->colTarget->height), src->colTarget);
}

void eIEffect::_renderQuad(eTexture2d *colTarget, eTexture2d *depthTarget, eTexture2d *tex0, eTexture2d *tex1) const
{
    eRenderState &rs = eGfx->getRenderState();
    rs.targets[0] = colTarget;
    rs.textures[0] = tex0;
    rs.textures[1] = tex1;
	rs.depthTarget = depthTarget;
    eRenderer->renderQuad(eRect(0, 0, colTarget->width, colTarget->height), eSize(colTarget->width, colTarget->height));
}

eIEffect::Target * eIEffect::_renderSimple(Target *src, ePixelShader *ps, eTextureFormat format)
{
    Target *dst = _acquireTarget(eSize(src->colTarget->width, src->colTarget->height), nullptr, format);
    eRenderState &rs = eGfx->getRenderState();
    rs.ps = ps;
    _renderQuad(dst->colTarget, dst->depthTarget, src->colTarget);
    return dst;
}

eIEffect::Target * eIEffect::_runHierarchy(eF32 time, const eSize &targetSize)
{
    TargetPtrArray inputs;

	Target *dst = _findTarget(this);
	if (dst)
		return dst;

    for (eU32 i=0; i<m_inputFx.size(); i++)
    {
        Target *rt = m_inputFx[i]->_runHierarchy(time, targetSize);
        inputs.append(rt);
    }

    eRenderState &rs = eGfx->pushRenderState();
    rs.textures[2] = eRenderer->getPositionMap();
    rs.textures[3] = eRenderer->getNormalMap();
    dst = _run(time, inputs, targetSize);
    dst->wasUsed = eTRUE;
	dst->refCount = eMax<eU32>(1, m_outputFx.size());
	dst->owner = this;
    eGfx->popRenderState();

    for (eU32 i=0; i<inputs.size(); i++)
        inputs[i]->refCount--;

    return dst;
}

void eIEffect::_garbageCollectTargets()
{
    for (eInt i=(eInt)m_targetPool.size()-1; i>=0; i--)
    {
        Target &t = m_targetPool[i];
        //eASSERT(!t.refCount);
		
#ifdef eEDITOR
        if (!t.wasUsed)
        {
            eGfx->removeTexture2d(t.colTarget);
			eGfx->removeTexture2d(t.depthTarget);
            m_targetPool.removeAt(i);
        }
        else
#endif
		{
			t.owner = nullptr;
            t.wasUsed = eFALSE;
			t.refCount = 0;
		}
    }
}

eIIterableEffect::eIIterableEffect(eU32 iters) :
    m_iters(iters)
{
}

void eIIterableEffect::setIterations(eU32 iters)
{
    m_iters = iters;
}

eU32 eIIterableEffect::getIterations() const
{
    return m_iters;
}

eIEffect::Target * eIIterableEffect::_renderPingPong(Target *src, Target *dst) const
{
    for (eU32 i=0; i<m_iters; i++)
    {
		_renderQuad(dst->colTarget, dst->depthTarget, src->colTarget);
        eSwap(src, dst);
    }

    return src;
}


eInputEffect::eInputEffect(const eScene &scene, const eCamera &cam, const eVector4 &viewport) :
    m_scene(&scene),
    m_cam(cam)
{
    m_viewport = viewport;
}

eIEffect::Target * eInputEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    const eRect &r = _viewportToRect(m_viewport, targetSize);//!eGfx->getWndSize());
    Target *dst = _acquireTarget(r.getSize(), &m_cam);

	if (dst->owner != &m_cam)
	{
		eRenderer->renderScene(*m_scene, m_cam, dst->colTarget, dst->depthTarget, time); // FIXME: put as well depth target
		dst->owner = &m_cam;
	}

    return dst;
}

eEFFECT_MAKE_CONSTRUCTOR(eDistortEffect, ps_fx_distort);
eIEffect::Target * eDistortEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.textures[4] = distMap;
    rs.texFlags[4] = eTMF_WRAP|eTMF_BILINEAR;
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_distort)));
}

eEFFECT_MAKE_CONSTRUCTOR(eAdjustEffect, ps_fx_adjust);
eIEffect::Target * eAdjustEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    Target *dst = _acquireTarget(eSize(srcs[0]->colTarget->width, srcs[0]->colTarget->height));

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.ps = eGfx->loadPixelShader(eSHADER(ps_fx_adjust));
    rs.constBufs[eCBI_FX_PARAMS] = &cb;
    return _renderPingPong(srcs[0], dst);
}

eEFFECT_MAKE_CONSTRUCTOR(eBlurEffect, ps_fx_blur);
eIEffect::Target * eBlurEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    Target *dst = _acquireTarget(eSize(srcs[0]->colTarget->width, srcs[0]->colTarget->height));

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.ps = eGfx->loadPixelShader(eSHADER(ps_fx_blur));
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderPingPong(srcs[0], dst);
}

eEFFECT_MAKE_CONSTRUCTOR(eDofEffect, ps_fx_dof);
eIEffect::Target * eDofEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_dof)));
}

eEFFECT_MAKE_CONSTRUCTOR(eSsaoEffect, ps_fx_ssao);
eIEffect::Target * eSsaoEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_ssao)));
}

eEFFECT_MAKE_CONSTRUCTOR(eFxaaEffect, ps_fx_fxaa);
eIEffect::Target * eFxaaEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);
    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_fxaa)));
}

eEFFECT_MAKE_CONSTRUCTOR(eColorGradingEffect, ps_fx_colorgrading);
eIEffect::Target * eColorGradingEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    eRenderState &rs = eGfx->getRenderState();
    rs.textures[4] = lookupMap;
    rs.texFlags[4] = eTMF_CLAMP|eTMF_BILINEAR;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_colorgrading)));
}

eEFFECT_MAKE_CONSTRUCTOR(eRadialBlurEffect, ps_fx_radialblur);
eIEffect::Target * eRadialBlurEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eGfx->getRenderState().constBufs[eCBI_FX_PARAMS] = &cb;
    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_radialblur)));
}

eEFFECT_MAKE_CONSTRUCTOR(eFogEffect, ps_fx_fog);
eIEffect::Target *eFogEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eGfx->getRenderState().constBufs[eCBI_FX_PARAMS] = &cb;
    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_fog)));
}

eEFFECT_MAKE_CONSTRUCTOR(eDownsampleEffect, ps_quad);
eIEffect::Target * eDownsampleEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);
    Target *src = srcs[0];

    for (eU32 i=0; i<m_iters; i++)
    {
        const eF32 newWidth = amount.x*(eF32)src->colTarget->width;
        const eF32 newHeight = amount.y*(eF32)src->colTarget->height;
        const eSize newSize(eMax(1, eFtoL(newWidth)), eMax(1, eFtoL(newHeight)));
        Target *dst = _acquireTarget(newSize);

        static eConstBuffer<eVector2, eST_PS> cb;
        cb.data = amount;

        eRenderState &rs = eGfx->getRenderState();
        rs.ps = eGfx->loadPixelShader(eSHADER(ps_quad));
        rs.constBufs[eCBI_FX_PARAMS] = &cb;
        _renderQuad(dst->colTarget, dst->depthTarget, src->colTarget);
        src = dst;
    }

    return src;
}

eEFFECT_MAKE_CONSTRUCTOR(eRippleEffect, ps_fx_ripple);
eIEffect::Target * eRippleEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eGfx->getRenderState().constBufs[eCBI_FX_PARAMS] = &cb;
    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_ripple)));
}

eEFFECT_MAKE_CONSTRUCTOR(eMicroscopeEffect, ps_fx_microscope);
eIEffect::Target * eMicroscopeEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_microscope)));
}

eEFFECT_MAKE_CONSTRUCTOR(eMotionBlurEffect, ps_fx_motionblur);
eIEffect::Target * eMotionBlurEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_motionblur)));
}

eEFFECT_MAKE_CONSTRUCTOR(eHaloEffect, ps_fx_halo);
eIEffect::Target * eHaloEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_halo)));
}

eEFFECT_MAKE_CONSTRUCTOR(eCausticsEffect, ps_fx_caustics);
eIEffect::Target * eCausticsEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_caustics)));
}

eEFFECT_MAKE_CONSTRUCTOR(eDebugEffect, ps_fx_debug);
eIEffect::Target * eDebugEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;
   
    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_debug)));
}

eEFFECT_MAKE_CONSTRUCTOR(eInterferenceEffect, ps_fx_interference);
eIEffect::Target * eInterferenceEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_interference)));
}

eEFFECT_MAKE_CONSTRUCTOR(eSpaceEffect, ps_fx_space);
eIEffect::Target * eSpaceEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_space)));
}

eEFFECT_MAKE_EMPTY_CONSTRUCTOR(eSaveEffect);
eIEffect::Target * eSaveEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(renderTarget);
    eASSERT(depthTarget);
    eASSERT(renderTarget->width == depthTarget->width);
    eASSERT(renderTarget->height == depthTarget->height);
    eASSERT(srcs.size() == 1);

    _renderQuad(renderTarget, depthTarget, srcs[0]->colTarget);
    return srcs[0];
}

eEFFECT_MAKE_CONSTRUCTOR(eMergeEffect, ps_fx_merge);
eIEffect::Target * eMergeEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() > 0);

    m_viewport.set(0.0f, 0.0f, 1.0f, 1.0f);

    Target *dst = _acquireTarget(targetSize);

    sc.viewport0 = m_inputFx[0]->m_viewport;
    sc.viewport1 = m_inputFx[1]->m_viewport;

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;
    eRenderState &rs = eGfx->getRenderState();
    rs.ps = eGfx->loadPixelShader(eSHADER(ps_fx_merge));
    rs.constBufs[eCBI_FX_PARAMS] = &cb;
    rs.textures[0] = srcs[0]->colTarget;
    rs.textures[1] = srcs[1]->colTarget;
    rs.targets[0] = dst->colTarget;
	rs.depthTarget = dst->depthTarget;
    eRenderer->renderQuad(eRect(0, 0, targetSize.width, targetSize.height), targetSize);

    return dst;
}

eEFFECT_MAKE_CONSTRUCTOR(eTerrainEffect, ps_fx_terrain);
eIEffect::Target * eTerrainEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();

    rs.textures[4] = noiseMap;
    rs.texFlags[4] = eTMF_MIRROR|eTMF_BILINEAR;
    rs.textures[5] = terrainMap;
    rs.texFlags[5] = eTMF_MIRROR|eTMF_BILINEAR;
    rs.textures[6] = grassTexture;
    rs.texFlags[6] = eTMF_MIRROR|eTMF_BILINEAR;
    rs.textures[7] = earthTexture;
    rs.texFlags[7] = eTMF_MIRROR|eTMF_BILINEAR;
    rs.textures[8] = rockTexture;
    rs.texFlags[8] = eTMF_MIRROR|eTMF_BILINEAR;
    rs.textures[9] = foamTexture;
    rs.texFlags[9] = eTMF_MIRROR|eTMF_BILINEAR;
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_terrain)));
}

eEFFECT_MAKE_CONSTRUCTOR(ePlanetEffect, ps_fx_planet);
eIEffect::Target * ePlanetEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_planet)));
}

eEFFECT_MAKE_CONSTRUCTOR(eRaytraceEffect, ps_fx_raytrace);
eIEffect::Target * eRaytraceEffect::_run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize)
{
    eASSERT(srcs.size() == 1);

    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data = sc;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_FX_PARAMS] = &cb;

    return _renderSimple(srcs[0], eGfx->loadPixelShader(eSHADER(ps_fx_raytrace)));
}