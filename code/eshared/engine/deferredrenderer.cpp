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

#ifdef ePLAYER
#include "../../eplayer4/production.hpp"
#endif

//#define SHOW_GBUFFER

eREGISTER_ENGINE_SHADER(ps_nolight);
eREGISTER_ENGINE_SHADER(ps_forward_light);
eREGISTER_ENGINE_SHADER(ps_deferred_ambient);
eREGISTER_ENGINE_SHADER(ps_deferred_geo);
eREGISTER_ENGINE_SHADER(ps_deferred_light);
eREGISTER_ENGINE_SHADER(ps_quad);
eREGISTER_ENGINE_SHADER(vs_nolight);
eREGISTER_ENGINE_SHADER(vs_instanced_geo);
eREGISTER_ENGINE_SHADER(vs_quad);
#ifndef eCFG_NO_ENGINE_DEFERRED_SHADOWS
	eREGISTER_ENGINE_SHADER(ps_shadow);
	eREGISTER_ENGINE_SHADER(vs_shadow);
	eREGISTER_ENGINE_SHADER(vs_distance);
	eREGISTER_ENGINE_SHADER(ps_distance);
#endif

eDeferredRenderer::eDeferredRenderer() :
    m_rtDiffuse(nullptr),
    m_rtNormals(nullptr),
    m_rtSpecular(nullptr),
    m_rtPosition(nullptr),
    m_rtEnvironment(nullptr),
    m_rtShadow(nullptr),
    m_texDistMap(nullptr),
    m_texIndirCm(nullptr),
    m_shadowSize(0)
{
    setShadowQuality(eSQ_LOW);
    m_geoQuad = eGfx->addGeometry(eGEO_DYNAMIC, eVTX_SIMPLE, eGPT_QUADLIST);

    // load shaders
    m_psNoLight    = eGfx->loadPixelShader(eSHADER(ps_nolight));
    m_psForwLight  = eGfx->loadPixelShader(eSHADER(ps_forward_light));
    m_psDefAmbient = eGfx->loadPixelShader(eSHADER(ps_deferred_ambient));
    m_psDefGeo     = eGfx->loadPixelShader(eSHADER(ps_deferred_geo));
    m_psDefLight   = eGfx->loadPixelShader(eSHADER(ps_deferred_light));
    m_psQuad       = eGfx->loadPixelShader(eSHADER(ps_quad));
    m_vsNoLight    = eGfx->loadVertexShader(eSHADER(vs_nolight));
    m_vsInstGeo    = eGfx->loadVertexShader(eSHADER(vs_instanced_geo));
    m_vsQuad       = eGfx->loadVertexShader(eSHADER(vs_quad));

#ifndef eCFG_NO_ENGINE_DEFERRED_SHADOWS
    m_psShadow     = eGfx->loadPixelShader(eSHADER(ps_shadow));
    m_vsShadow     = eGfx->loadVertexShader(eSHADER(vs_shadow));
    m_vsDistance   = eGfx->loadVertexShader(eSHADER(vs_distance));
    m_psDistance   = eGfx->loadPixelShader(eSHADER(ps_distance));
#endif
}

eDeferredRenderer::~eDeferredRenderer()
{
    eGfx->removeGeometry(m_geoQuad);
    eGfx->removeTextureCube(m_texIndirCm);
    eGfx->removeTexture2d(m_texDistMap);
    _freeTargets();
}

void eDeferredRenderer::renderScene(const eScene &scene, const eCamera &cam, eTexture2d *colorTarget, eTexture2d *depthTarget, eF32 time)
{
    ePROFILER_FUNC();

    eASSERT(colorTarget);
    eASSERT(colorTarget->target);
    eASSERT(time >= 0.0f);

    const eRect area(0, 0, colorTarget->width, colorTarget->height);
    scene.getRenderJobs(cam, m_allJobs);

    // take care to maintain valid (filled)
    // depth buffer among different passes!
    _setupTargets(colorTarget->width, colorTarget->height);
    _renderGeometryPass(cam, depthTarget);
    _renderAmbientPass(colorTarget, depthTarget, scene, area);
    _renderNoLightPass(colorTarget, depthTarget, cam);
    _renderAlphaLightPass(colorTarget, depthTarget, scene, cam, area);
    _renderLightPass(colorTarget, depthTarget, scene, cam, area);
   
#ifdef SHOW_GBUFFER
    _showGeometryBuffer(target, area);
#endif
}

void eDeferredRenderer::renderQuad(const eRect &r, const eSize &size, eTexture2dDx11 *tex, const eVector2 &tileUv, const eVector2 &scrollUv) const
{
    // set render states
    eRenderState &rs = eGfx->getRenderState();
    rs.cullMode = eCULL_NONE;
    rs.depthTest = eFALSE;
    rs.viewport.set(0, 0, size.width, size.height);

    if (tex)
        rs.textures[0] = tex;

    // use default shaders if no other shaders set
    if (!rs.ps)
        rs.ps = m_psQuad;
    if (!rs.vs)
        rs.vs = m_vsQuad;

    // render quad
    const eCamera cam(0.0f, (eF32)size.width, 0.0f, (eF32)size.height, -1.0f, 1.0f);
    cam.activate();

    eSimpleVtx *vp = nullptr;
    eGfx->beginLoadGeometry(m_geoQuad, 4, (ePtr *)&vp);
    {
        vp[0].set(eVector3((eF32)r.left,  (eF32)r.top,    0.0f), eVector2(scrollUv.u,          scrollUv.v+tileUv.v));
        vp[1].set(eVector3((eF32)r.left,  (eF32)r.bottom, 0.0f), eVector2(scrollUv.u,          scrollUv.v));
        vp[2].set(eVector3((eF32)r.right, (eF32)r.bottom, 0.0f), eVector2(scrollUv.u+tileUv.u, scrollUv.v));
        vp[3].set(eVector3((eF32)r.right, (eF32)r.top,    0.0f), eVector2(scrollUv.u+tileUv.u, scrollUv.v+tileUv.v));
    }
    eGfx->endLoadGeometry(m_geoQuad);
    eGfx->renderGeometry(m_geoQuad);
}

void eDeferredRenderer::setShadowQuality(eShadowQuality sq)
{
    const eU32 shadowSizes[] = {256, 512, 1024};

    if (m_shadowSize != shadowSizes[sq])
    {
        m_shadowSize = shadowSizes[sq];
        eGfx->removeTexture2d(m_texDistMap);
        m_texDistMap = eGfx->addTexture2d(m_shadowSize*eCMF_COUNT, m_shadowSize, eTEX_TARGET, eTFO_DEPTH32F);
        _createIndirectionCubeMap();
    }
}

eTexture2d * eDeferredRenderer::getPositionMap() const
{
    return m_rtPosition;
}

eTexture2d * eDeferredRenderer::getNormalMap() const
{
    return m_rtNormals;
}

void eDeferredRenderer::_renderGeometryPass(const eCamera &cam, eTexture2d *depthTarget)
{
    eRenderState &rs = eGfx->freshRenderState();

	rs.depthTarget = depthTarget;
    rs.targets[0] = m_rtDiffuse;
    rs.targets[1] = m_rtNormals;
    rs.targets[2] = m_rtSpecular;
    rs.targets[3] = m_rtPosition;
    rs.targets[4] = m_rtEnvironment;
    rs.viewport.set(0, 0, m_rtNormals->width, m_rtNormals->height);
    eGfx->clear(eCM_COLOR|eCM_DEPTH, eCOL_BLACK);

    m_allJobs.render(cam, eRJW_RENDER_ALL & ~eRJW_ALPHA_ON & ~eRJW_LIGHTED_OFF);
}

void eDeferredRenderer::_renderAmbientPass(eTexture2d *target, eTexture2d *depthTarget, const eScene &scene, const eRect &area)
{
    // calculate the ambient color
    eColor ambient;
    for (eU32 i=0; i<scene.getLightCount(); i++)
        ambient += scene.getLight(i).getAmbient();

    // render the ambient light
    static eConstBuffer<eVector4, eST_PS> cb;
    cb.data = ambient;

    eRenderState &rs = eGfx->freshRenderState();
    rs.targets[0] = target;
	rs.depthTarget = depthTarget;
    rs.ps = m_psDefAmbient;
    rs.constBufs[eCBI_PASS_AMBIENT] = &cb;
    rs.textures[0] = m_rtDiffuse;
	rs.textures[3] = m_rtPosition;
    rs.textures[4] = m_rtEnvironment;
    rs.texFlags[0] = eTMF_CLAMP|eTMF_NEAREST;
	rs.texFlags[3] = eTMF_CLAMP|eTMF_NEAREST;
    rs.texFlags[4] = eTMF_CLAMP|eTMF_NEAREST;

    eGfx->clear(eCM_COLOR, eCOL_YELLOW);
    renderQuad(area, area.getSize(), nullptr);
}

void eDeferredRenderer::_renderAlphaLightPass(eTexture2d *target, eTexture2d *depthTarget, const eScene &scene, const eCamera &cam, const eRect &area)
{
    eRenderState &rs = eGfx->freshRenderState();
    rs.targets[0] = target;
	rs.depthTarget = depthTarget;
    rs.viewport.set(0, 0, m_rtNormals->width, m_rtNormals->height);

    for (eU32 i=0; i<scene.getLightCount(); i++)
    {
        const eLight &light = scene.getLight(i);
        if (light.activateScissor(area.getSize(), cam))
        {
            light.activate(cam.getViewMatrix());
            m_allJobs.render(cam, eRJW_ALPHA_ON|eRJW_LIGHTED_ON|eRJW_SHADOWS_BOTH);
        }
    }
}

void eDeferredRenderer::_renderNoLightPass(eTexture2d *target, eTexture2d *depthTarget, const eCamera &cam)
{
    eRenderState &rs = eGfx->freshRenderState();
    rs.targets[0] = target;
	rs.depthTarget = depthTarget;
    rs.viewport.set(0, 0, m_rtNormals->width, m_rtNormals->height);
    m_allJobs.render(cam, eRJW_ALPHA_BOTH|eRJW_LIGHTED_OFF|eRJW_SHADOWS_BOTH);
}

void eDeferredRenderer::_renderLightPass(eTexture2d *target, eTexture2d *depthTarget, const eScene &scene, const eCamera &cam, const eRect &area)
{
    for (eU32 i=0; i<scene.getLightCount(); i++)
    {
        const eLight &light = scene.getLight(i);
        if (light.activateScissor(area.getSize(), cam))
        {
            // create shadow map for light
#ifndef eCFG_NO_ENGINE_DEFERRED_SHADOWS
            _renderLightDistance(scene, light);
            _renderShadowMap(cam, light, depthTarget);
#endif

            // perform deferred lighting+shadowing
            eRenderState &rs = eGfx->freshRenderState();
            rs.targets[0] = target;
			rs.depthTarget = depthTarget;
            rs.textures[0] = m_rtDiffuse;
            rs.textures[1] = m_rtNormals;
            rs.textures[2] = m_rtSpecular;
            rs.textures[3] = m_rtPosition;
            rs.textures[4] = m_rtShadow;
            rs.texFlags[0] = eTMF_CLAMP|eTMF_NEAREST;
            rs.texFlags[1] = eTMF_CLAMP|eTMF_NEAREST;
            rs.texFlags[2] = eTMF_CLAMP|eTMF_NEAREST;
            rs.texFlags[3] = eTMF_CLAMP|eTMF_NEAREST;
            rs.texFlags[4] = eTMF_BILINEAR;
            rs.blending = eTRUE;
            rs.blendSrc = eBM_ONE;
            rs.blendDst = eBM_ONE;
            rs.ps = m_psDefLight;
            rs.vs = m_vsQuad;

            light.activate(cam.getViewMatrix());
            renderQuad(area, area.getSize());
        }
    }
}

void eDeferredRenderer::_renderLightDistance(const eScene &scene, const eLight &light)
{
    const eVector3 &lightPos = light.getPosition();

    eRenderState &rs = eGfx->freshRenderState();
    rs.colorWrite = eFALSE;
    rs.vs = m_vsDistance;
    rs.ps = m_psDistance;
    rs.targets[0] = nullptr;
    rs.depthTarget = m_texDistMap;

    eGfx->clear(eCM_DEPTH, eCOL_WHITE);

    eCamera cam(90.0f, 1.0f, 0.1f, light.getRange());
    eMatrix4x4 cubeMtx, viewMtx;
    static eRenderJobQueue jobs; // static to reduce memory allocations

    for (eU32 i=0; i<eCMF_COUNT; i++)
    {
        if (light.getCastsShadows((eCubeMapFace)i))
        {
            rs.viewport.set(i*m_shadowSize, 0, (i+1)*m_shadowSize, m_shadowSize);

            cubeMtx.cubemap(i);
            viewMtx.identity();
            viewMtx.translate(-lightPos);
            viewMtx *= cubeMtx;

            cam.setViewMatrix(viewMtx);
            scene.getRenderJobs(cam, jobs);
            jobs.render(cam, eRJW_RENDER_ALL & ~eRJW_SHADOWS_OFF & ~eRJW_ALPHA_ON, eRJF_MATERIALS_OFF);
        }
    }
}

void eDeferredRenderer::_renderShadowMap(const eCamera &cam, const eLight &light, eTexture2d *depthTarget)
{
    eRenderState &rs = eGfx->freshRenderState();
    rs.vs = m_vsShadow;
    rs.ps = m_psShadow;
    rs.targets[0] = m_rtShadow;
	rs.depthTarget = depthTarget;
    rs.textures[0] = m_texIndirCm;
    rs.texFlags[0] = eTMF_NEAREST|eTMF_CLAMP;
    rs.textures[1] = m_texDistMap;
    rs.texFlags[1] = eTMF_PCF;
    rs.viewport.set(0, 0, m_rtShadow->width, m_rtShadow->height);

    // depth target has to be cleared because
    // it is reused from geometry pass
    eGfx->clear(eCM_COLOR|eCM_DEPTH, eCOL_BLACK);

    if (light.getCastsAnyShadows())
    {
        const eF32 nearZ = 0.1f;
        const eF32 farZ = light.getRange();

        static eConstBuffer<eVector2, eST_PS> cb;
        cb.data.x = nearZ*farZ/(farZ-nearZ);
        cb.data.y = farZ/(farZ-nearZ);
        eGfx->getRenderState().constBufs[eCBI_PASS_SHADOW] = &cb;

        light.activate(cam.getViewMatrix());
        m_allJobs.render(cam, eRJW_RENDER_ALL & ~eRJW_SHADOWS_OFF & ~eRJW_ALPHA_ON, eRJF_MATERIALS_OFF);
    }
}

void eDeferredRenderer::_showGeometryBuffer(eTexture2d *target, const eRect &area) const
{
    const eInt w = area.getWidth();
    const eInt h = area.getHeight();

    eGfx->freshRenderState().targets[0] = target;

    renderQuad(eRect(  0, h/3, w/2,     h), area.getSize(), m_rtShadow);    // top left
    renderQuad(eRect(w/2, h/3,   w,     h), area.getSize(), m_texDistMap);  // top right
    renderQuad(eRect(  0, h/3, w/2, 2*h/3), area.getSize(), m_rtSpecular);  // middle left
    renderQuad(eRect(w/2, h/3,   w, 2*h/3), area.getSize(), m_rtDiffuse);   // middle right
    renderQuad(eRect(  0,   0, w/2,   h/3), area.getSize(), m_rtNormals);   // bottom left
    renderQuad(eRect(w/2,   0,   w,   h/3), area.getSize(), m_rtPosition);  // bottom right
}

void eDeferredRenderer::_createIndirectionCubeMap()
{
    const eU32 size = m_shadowSize*2;
    m_texIndirCm = eGfx->addTextureCube(size, 0, eTFO_GR32F);
    
    for (eInt i=0; i<eCMF_COUNT; i++)
    {
        eArray<eVector2> face(size*size);

        for (eU32 y=0, index=0; y<size; y++)
        {
            const eF32 v = (eF32)y/(eF32)(size-1);
            for (eU32 x=0; x<size; x++)
            {
                const eF32 u = (eF32)(x+i*size)/(eF32)(size*eCMF_COUNT-1);                
                face[index++].set(u, v);
            }
        }

        eGfx->updateTextureCube(m_texIndirCm, &face[0], (eCubeMapFace)i);
    }
}

void eDeferredRenderer::_setupTargets(eU32 width, eU32 height)
{
    if (!m_rtDiffuse || m_rtDiffuse->width != width || m_rtDiffuse->height != height)
    {
        _freeTargets();

        m_rtDiffuse     = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB16F);
        m_rtNormals     = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB16F);
        m_rtSpecular    = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB16F);
        m_rtPosition    = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB16F);
        m_rtEnvironment = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_ARGB8);
        m_rtShadow      = eGfx->addTexture2d(width, height, eTEX_TARGET, eTFO_R16F);
    }
}

void eDeferredRenderer::_freeTargets()
{
    eGfx->removeTexture2d(m_rtDiffuse);
    eGfx->removeTexture2d(m_rtNormals);
    eGfx->removeTexture2d(m_rtSpecular);
    eGfx->removeTexture2d(m_rtPosition);
    eGfx->removeTexture2d(m_rtEnvironment);
    eGfx->removeTexture2d(m_rtShadow);
}