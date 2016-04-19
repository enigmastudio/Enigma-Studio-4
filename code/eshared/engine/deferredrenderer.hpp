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

#ifndef DEFERRED_RENDERER_HPP
#define DEFERRED_RENDERER_HPP

enum eShadowQuality
{
    eSQ_LOW,
    eSQ_MEDIUM,
    eSQ_HIGH
};

class eDeferredRenderer
{
public:
    eDeferredRenderer();
    ~eDeferredRenderer();

    void            renderScene(const eScene &scene, const eCamera &cam, eTexture2d *colorTarget, eTexture2d *depthTarget, eF32 time);
    void            renderQuad(const eRect &r, const eSize &size, eTexture2dDx11 *tex=nullptr, const eVector2 &tileUv=eVector2(1.0f), const eVector2 &scrollUv=eVector2()) const;

    void            setShadowQuality(eShadowQuality sq);

    eTexture2d *    getPositionMap() const;
    eTexture2d *    getNormalMap() const;

private:
    void            _renderGeometryPass(const eCamera &cam, eTexture2d *depthTarget);
    void            _renderAmbientPass(eTexture2d *target, eTexture2d *depthTarget, const eScene &scene, const eRect &area);
    void            _renderAlphaLightPass(eTexture2d *target, eTexture2d *depthTarget, const eScene &scene, const eCamera &cam, const eRect &area);
    void            _renderNoLightPass(eTexture2d *target, eTexture2d *depthTarget, const eCamera &cam);
    void            _renderLightPass(eTexture2d *target, eTexture2d *depthTarget, const eScene &scene, const eCamera &cam, const eRect &area);
    void            _renderLightDistance(const eScene &scene, const eLight &light);
    void            _renderShadowMap(const eCamera &cam, const eLight &light, eTexture2d *depthTarget);
    void            _showGeometryBuffer(eTexture2d *target, const eRect &area) const;

    void            _createIndirectionCubeMap();
    void            _setupTargets(eU32 width, eU32 height);
    void            _freeTargets();

private:
    eGeometry *     m_geoQuad;
    eRenderJobQueue m_allJobs;

    ePixelShader *  m_psForwLight;
    ePixelShader *  m_psNoLight;
    ePixelShader *  m_psDefAmbient;
    ePixelShader *  m_psDefGeo;
    ePixelShader *  m_psDefLight;
    ePixelShader *  m_psDistance;
    ePixelShader *  m_psShadow;
    ePixelShader *  m_psQuad;
    eVertexShader * m_vsNoLight;
    eVertexShader * m_vsInstGeo;
    eVertexShader * m_vsDistance;
    eVertexShader * m_vsShadow;
    eVertexShader * m_vsQuad;

    eTexture2d *    m_rtDiffuse;
    eTexture2d *    m_rtNormals;
    eTexture2d *    m_rtSpecular;
    eTexture2d *    m_rtPosition;
    eTexture2d *    m_rtEnvironment;
    eTexture2d *    m_rtShadow;
    eTexture2d *    m_texDistMap;
    eTextureCube *  m_texIndirCm;
    eU32            m_shadowSize;
};

#endif // DEFERRED_RENDERER_HPP