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

eTexture2d * eMaterial::m_whiteTex = nullptr;
eTexture2d * eMaterial::m_normalMap = nullptr;
eBool        eMaterial::m_forceWireframe = eFALSE;
eMaterial    eMaterial::m_defaultMat;
eMaterial    eMaterial::m_wireframeMat;

eMaterial::eMaterial() :
    cullMode(eCULL_BACK),
    depthBias(eDB_NONE),
    blendSrc(eBM_ONE),
    blendDst(eBM_ONE),
    blendOp(eBO_ADD),
    blending(eFALSE),
    flatShaded(eFALSE),
    depthFunc(eDF_LEQUAL),
    depthTest(eTRUE),
    depthWrite(eTRUE),
    lighted(eTRUE),
    renderPass(0),
    diffuseCol(eCOL_WHITE),
    specCol(eCOL_WHITE),
    shininess(0.25f),
    pointSize(0.1f),
    ps(nullptr),
    vs(nullptr)
{
    for (eU32 i=0; i<eMTU_COUNT; i++)
    {
        textures[i] = nullptr;
        texFlags[i] = eTMF_CLAMP|eTMF_BILINEAR;
    }
}

void eMaterial::activate() const
{
    eASSERT(renderPass < 256); // to not occupy more that 8 bit, render pass <= 255
    eASSERT(shininess >= 0.0f);
    eASSERT(pointSize > 0.0f);
    eASSERT(ps && vs);

    // set textures
    eRenderState &rs = eGfx->getRenderState();

    for (eU32 i=0; i<eMTU_COUNT; i++)
    {
        rs.textures[i] = textures[i];
        rs.texFlags[i] = texFlags[i];
    }

    // set default textures if no textures provided
    if (!textures[eMTU_DIFFUSE])
        rs.textures[eMTU_DIFFUSE] = m_whiteTex;
    if (!textures[eMTU_NORMAL])
        rs.textures[eMTU_NORMAL] = m_normalMap;

    // set shader constants
    static eConstBuffer<ShaderConsts, eST_PS> cb;
    cb.data.diffuseCol = diffuseCol;
    cb.data.specCol = specCol;
    cb.data.shininess = shininess;

    // set render states
    rs.wireframe = m_forceWireframe;
    rs.cullMode = cullMode;
    rs.depthBias = depthBias;
    rs.blendSrc = blendSrc;
    rs.blendDst = blendDst;
    rs.blendOp = blendOp;
    rs.blending = blending;
    rs.depthTest = depthTest;
    rs.depthWrite = depthWrite;
    rs.constBufs[eCBI_MATERIAL] = &cb;
    rs.vs = vs;
    rs.ps = ps;
}

eU32 eMaterial::getSortKey() const
{
    eU32 key = (renderPass<<24);
    key |= ((eU32)blending<<16);
    key |= (eU16)((eU32)textures[0]^(eU32)textures[1]);
    return key;
}

eREGISTER_ENGINE_SHADER(ps_nolight);
eREGISTER_ENGINE_SHADER(vs_instanced_geo);
eREGISTER_ENGINE_SHADER(ps_deferred_geo);
void eMaterial::initialize()
{
    // initialize wireframe material
    m_wireframeMat.cullMode = eCULL_NONE;
    m_wireframeMat.lighted = eFALSE;
    m_wireframeMat.depthBias = eDB_BEFORE;
    m_wireframeMat.ps = eGfx->loadPixelShader(eSHADER(ps_nolight));
    m_wireframeMat.vs = eGfx->loadVertexShader(eSHADER(vs_instanced_geo));

    // initialize default material
    m_defaultMat.flatShaded = eTRUE;
    m_defaultMat.cullMode = eCULL_NONE;
    m_defaultMat.ps = eGfx->loadPixelShader(eSHADER(ps_deferred_geo));
    m_defaultMat.vs = m_wireframeMat.vs;

    // create white texture (set when no diffuse
    // texture is specified, default return color
    // of empty sampler in PS is black)
    m_whiteTex = eGfx->addTexture2d(1, 1, 0, eTFO_ARGB8);
    eGfx->updateTexture2d(m_whiteTex, &eColor(eCOL_WHITE));

    // create default normal map with up-
    // pointing normal vector <0.0f, 0.0f, 1.0f>
    m_normalMap = eGfx->addTexture2d(1, 1, 0, eTFO_ARGB8);
    eGfx->updateTexture2d(m_normalMap, &eColor(128, 128, 255));
}

void eMaterial::shutdown()
{
    eGfx->removeTexture2d(m_whiteTex);
    eGfx->removeTexture2d(m_normalMap);
}

void eMaterial::forceWireframe(eBool force)
{
    m_forceWireframe = force;
}

const eMaterial * eMaterial::getDefault()
{
    return &m_defaultMat;
}

const eMaterial * eMaterial::getWireframe()
{
    return &m_wireframeMat;
}