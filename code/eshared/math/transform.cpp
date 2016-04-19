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
#include "math.hpp"

eTransform::eTransform()
{
    identity();
}

eTransform::eTransform(const eQuat &rot, const eVector3 &trans, const eVector3 &scale, eTransformOrder order) :
    m_identity(eFALSE),
    m_uniformScale(scale.isUniform()),
    m_nrmMtxDirty(eTRUE)
{
    ePROFILER_FUNC();

    switch (order)
    {
    case eTO_SRT:
        m_transMtx.scale(scale);
        m_transMtx *= eMatrix4x4(rot);
        m_transMtx.translate(trans);
        break;

    case eTO_STR:
        m_transMtx.scale(scale);
        m_transMtx.translate(trans);
        m_transMtx *= eMatrix4x4(rot);
        break;

    case eTO_RTS:
        m_transMtx *= eMatrix4x4(rot);
        m_transMtx.translate(trans);
        m_transMtx.scale(scale);
        break;

    case eTO_TRS:
        m_transMtx.translate(trans);
        m_transMtx *= eMatrix4x4(rot);
        m_transMtx.scale(scale);
        break;
    }
}

void eTransform::identity()
{
    m_transMtx.identity();
    m_identity = eTRUE;
    m_uniformScale = eTRUE;
    m_nrmMtxDirty = eTRUE;
}

void eTransform::rotate(const eQuat &rot)
{
    m_transMtx *= eMatrix4x4(rot);
    m_identity = eFALSE;
    m_nrmMtxDirty = eTRUE;
}

void eTransform::scale(const eVector3 &scale)
{
    m_transMtx.scale(scale);
    m_identity = eFALSE;
    m_uniformScale &= scale.isUniform();
    m_nrmMtxDirty = eTRUE;
}

void eTransform::translate(const eVector3 &trans)
{
    m_transMtx.translate(trans);
    m_identity = eFALSE;
    m_nrmMtxDirty = eTRUE;
}

const eMatrix4x4 & eTransform::getMatrix() const
{
    return m_transMtx;
}

const eMatrix3x3 & eTransform::getNormalMatrix() const
{
    ePROFILER_FUNC();

    if (m_nrmMtxDirty)
    {
        m_nrmMtxDirty = eFALSE;
        m_normalMtx = m_transMtx.getUpper3x3(); // cancels out translation

        // uniform scale => transformation matrix orthogonal
        // => inverse=transpose => transformation=normal matrix
        if (!m_uniformScale)
        {
            m_normalMtx.transpose();
            m_normalMtx.invert();
        }
    }

    return m_normalMtx;
}

eTransform eTransform::operator * (const eTransform &t) const
{
    eTransform res = *this;
    res *= t;
    return res;
}

eTransform & eTransform::operator *= (const eTransform &t)
{
    if (!t.m_identity)
    {
        m_transMtx *= t.m_transMtx;
        m_identity = eFALSE;
        m_uniformScale &= t.m_uniformScale;
        m_nrmMtxDirty = eTRUE;
    }

    return *this;
}