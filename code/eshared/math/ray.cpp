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

eRay::eRay(const eVector3 &origin, const eVector3 &dir) :
    m_origin(origin),
    m_dir(dir)
{
    eASSERT(!eIsFloatZero(dir.sqrLength()));
    eASSERT(eAreFloatsEqual(dir.sqrLength(), 1.0f));
}

eVector3 eRay::evaluate(eF32 t) const
{
    return m_origin+m_dir*t;
}

// ray-plane intersection
eBool eRay::intersects(const ePlane &p, eVector3 *ip) const
{
    const eF32 denom = p.getNormal()*m_dir;
    if (eIsFloatZero(denom))
        return eFALSE;

    const eF32 nom = -(p.getNormal()*m_origin+p.getCoeffD());
    const eF32 t = nom/denom;
        
    if (t < 0.0f)
        return eFALSE;
    if (ip)
        *ip = evaluate(t);

    return eTRUE;
}

// ray-triangle intersection
eBool eRay::intersects(const eVector3 &triA, const eVector3 &triB, const eVector3 &triC, eVector3 *ip) const
{
    const eVector3 e0 = triB-triA;
    const eVector3 e1 = triC-triA;
    const eVector3 e2 = m_dir^e1;

    const eF32 a = e0*e2;
    if (eIsFloatZero(a))
        return eFALSE;

    const eF32 f = 1.0f/a;
    const eVector3 s = m_origin-triA;
    const eF32 u = f*(s*e2);
    if (u < 0.0f || u > 1.0f)
        return eFALSE;

    const eVector3 q = s^e0;
    const eF32 v = f*(m_dir*q);
    if (v < 0.0f || u+v > 1.0f)
        return eFALSE;

    const eF32 t = f*(e1*q);
    if (t < 0.0f) // intersection in back?
        return eFALSE;

    if (ip)
        *ip = evaluate(t);

    return eTRUE;
}

void eRay::setOrigin(const eVector3 &origin)
{
    m_origin = origin;
}

void eRay::setDirection(const eVector3 &dir)
{
    m_dir = dir;
}

const eVector3 & eRay::getOrigin() const
{
    return m_origin;
}

const eVector3 & eRay::getDirection() const
{
    return m_dir;
}