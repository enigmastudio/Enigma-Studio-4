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

ePlane::ePlane() :
    m_normal(eVEC3_YAXIS),
    m_absNormal(m_normal),
    m_d(0.0f)
{
}

ePlane::ePlane(const eVector3 &v0, const eVector3 &v1, const eVector3 &v2) :
    m_normal((v2-v1)^(v0-v1))
{
    m_normal.normalize();
    m_absNormal = m_normal;
    m_absNormal.abs();
    m_d = -m_normal*v0;
}

ePlane::ePlane(const eVector3 &point, const eVector3 &normal) :
    m_normal(normal)
{
    m_normal.normalize();
    m_absNormal = m_normal;
    m_absNormal.abs();
    m_d = m_normal*point;
}

eBool ePlane::intersects(const eVector3 &lineSegA, const eVector3 &lineSegB, eVector3 *ip) const
{
    const eF32 da = lineSegA*m_normal+m_d;
    const eF32 db = lineSegB*m_normal+m_d;
    const eF32 t = da/(da-db);

    if (t < 0.0f || t > 1.0f)
        return eFALSE;
    else if (ip)
        *ip = (1.0f-t)*lineSegA+t*lineSegB;

    return eTRUE;
}

ePlaneSide ePlane::getSide(const eVector3 &v) const
{
    const eF32 dist = getDistance(v);

    if (dist > eALMOST_ZERO)
        return ePLS_FRONT;
    else if (dist < -eALMOST_ZERO)
        return ePLS_BACK;
    else
        return ePLS_ON;
}

eF32 ePlane::getDistance(const eVector3 &v) const
{
    return m_normal*v+m_d;
}

const eVector3 & ePlane::getNormal() const
{
    return m_normal;
}

const eVector3 & ePlane::getAbsNormal() const
{
    return m_absNormal;
}

eF32 ePlane::getCoeffD() const
{
    return m_d;
}