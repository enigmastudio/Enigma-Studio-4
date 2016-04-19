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

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

enum eMaterialTextureUnit
{
    eMTU_DIFFUSE,
    eMTU_NORMAL,
    eMTU_SPECULAR,
    eMTU_DEPTH,
    eMTU_ENV,
    eMTU_COUNT,
};

class eMaterial 
{
public:
    eMaterial();

    void                        activate() const;
    eU32                        getSortKey() const;

    static void                 initialize();
    static void                 shutdown();
    static void                 forceWireframe(eBool force);
    static const eMaterial *    getDefault();
    static const eMaterial *    getWireframe();

private:
    struct ShaderConsts
    {
        eVector4                diffuseCol;
        eVector4                specCol;
        eF32                    shininess;
    };

private:
    static eTexture2d *         m_whiteTex;
    static eTexture2d *         m_normalMap;
    static eMaterial            m_defaultMat;
    static eMaterial            m_wireframeMat;
    static eBool                m_forceWireframe;

public:
    // render states
    eITexture *                 textures[eMTU_COUNT];
    eInt                        texFlags[eMTU_COUNT];
    eCullMode                   cullMode;
    eDepthBias                  depthBias;
    eBlendMode                  blendSrc;
    eBlendMode                  blendDst;
    eBlendOp                    blendOp;
    eBool                       blending;
    eBool                       depthTest;
    eDepthFunc                  depthFunc;
    eBool                       depthWrite;
    ePixelShader *              ps;
    eVertexShader *             vs;

    // material properties
    eBool                       flatShaded;
    eBool                       lighted;
    eColor                      diffuseCol;
    eColor                      specCol;
    eU32                        renderPass;
    eF32                        shininess;
    eF32                        pointSize;
};

#endif // MATERIAL_HPP