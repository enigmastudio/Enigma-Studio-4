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

#ifndef LIGHT_HPP
#define LIGHT_HPP

class eLight
{
public:
    eLight();
    eLight(const eColor &diffuseCol, const eColor &ambientCol, const eColor &specCol, eF32 range, eBool castsShadows);

    void                activate(const eMatrix4x4 &viewMtx) const;
    eBool               activateScissor(const eSize &viewport, const eCamera &cam) const;

    void                setPosition(const eVector3 &pos);
    void                setDiffuse(const eColor &diffuseCol);
    void                setAmbient(const eColor &ambientCol);
    void                setSpecular(const eColor &specCol);
    void                setRange(eF32 range);
    void                setPenumbraSize(eF32 penumbraSize);
    void                setShadowBias(eF32 shadowBias);
    void                setCastsShadows(eCubeMapFace face, eBool castsShadows);

    const eVector3 &    getPosition() const;
    const eColor &      getDiffuse() const;
    const eColor &      getAmbient() const;
    const eColor &      getSpecular() const;
    eF32                getRange() const;
    eF32                getPenumbraSize() const;
    eF32                getShadowBias() const;
    eBool               getCastsShadows(eCubeMapFace face) const;
    eBool               getCastsAnyShadows() const;

private:
    struct ShaderConsts
    {
        eFXYZ           viewPos;
        eF32            pad0;
        eFXYZ           worldPos;
        eF32            pad1;
        eVector4        diffuseCol;
        eVector4        specCol;
        eF32            invRange;
        eF32            penumbraSize;
        eF32            shadowBias;
    };

private:
    eVector3            m_pos;
    eColor              m_diffuseCol;
    eColor              m_ambientCol;
    eColor              m_specCol;
    eF32                m_range;
    eBool               m_castsShadows[eCMF_COUNT];
    eF32                m_penumbraSize;
    eF32                m_shadowBias;
};

#endif // LIGHT_HPP