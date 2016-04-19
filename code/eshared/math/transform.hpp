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

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

// order of transformations:
// [s]cale, [r]otate, [t]ranslate
enum eTransformOrder
{
    eTO_SRT, // default
    eTO_STR,
    eTO_RTS,
    eTO_TRS
};

class eTransform
{
public:
    eTransform();
    eTransform(const eQuat &rot, const eVector3 &trans, const eVector3 &scale, eTransformOrder order=eTO_SRT);

    void                identity();
    void                rotate(const eQuat &rot);
    void                scale(const eVector3 &scale);
    void                translate(const eVector3 &trans);
    const eMatrix4x4 &  getMatrix() const;
    const eMatrix3x3 &  getNormalMatrix() const;

    eTransform          operator * (const eTransform &t) const;
    eTransform &        operator *= (const eTransform &t);

private:
    mutable eMatrix4x4  m_transMtx;
    mutable eMatrix3x3  m_normalMtx;
    mutable eBool       m_nrmMtxDirty;
    eBool               m_identity;
    eBool               m_uniformScale;
};

#endif // TRANSFORM_HPP