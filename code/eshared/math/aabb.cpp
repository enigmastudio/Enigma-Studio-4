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

eAABB::eAABB()
{
    clear();
}

eAABB::eAABB(const eVector3 &min, const eVector3 &max)
{
    setMinMax(min, max);
}

eAABB::eAABB(const eVector3 &center, eF32 width, eF32 height, eF32 depth)
{
    setCenterSize(center, eVector3(width, height, depth));
}

void eAABB::clear()
{
    const eInt ii = 0x7F800000;
    const eF32 inf = *(eF32 *)&ii;

    m_min.set( inf,  inf,  inf);
    m_max.set(-inf, -inf, -inf);
    m_size.set(inf,  inf,  inf);
    m_center.null();
}

void eAABB::merge(const eAABB &aabb)
{
    m_min.minComponents(aabb.m_min);
    m_max.maxComponents(aabb.m_max);
    m_center = (m_min+m_max)*0.5f;
    m_size = m_max-m_min;
}

void eAABB::updateExtent(const eVector3 &v)
{
    m_min.minComponents(v);
    m_max.maxComponents(v);
    m_center = (m_min+m_max)*0.5f;
    m_size = m_max-m_min;
}

void eAABB::translate(const eVector3 &v)
{
    m_min += v;
    m_max += v;
    m_center += v;
}

void eAABB::scale(const eVector3 &v)
{
    m_min.scale(v);
    m_max.scale(v);
    m_size.scale(v);
}

void eAABB::transform(const eTransform &transf)
{
    transform(transf.getMatrix());
}

// transformation of AABBs is a bit tricky in order
// to compensate for rotations from model- to world-
// space (kills axis alignment)
void eAABB::transform(const eMatrix4x4 &mtx)
{
    eVector3 nheX(mtx.m11*m_size.x, mtx.m12*m_size.x, mtx.m13*m_size.x);
    eVector3 nheY(mtx.m21*m_size.y, mtx.m22*m_size.y, mtx.m23*m_size.y);
    eVector3 nheZ(mtx.m31*m_size.z, mtx.m32*m_size.z, mtx.m33*m_size.z);

    nheX.abs();
    nheY.abs();
    nheZ.abs();

    m_center *= mtx;
    m_size = nheX+nheY+nheZ;
    m_min = m_center-0.5f*m_size;
    m_max = m_center+0.5f*m_size;
}

void eAABB::setMinMax(const eVector3 &min, const eVector3 &max)
{
    m_min = min;
    m_max = max;
    m_center = (min+max)*0.5f;
    m_size = max-min;
}

void eAABB::setCenterSize(const eVector3 &center, const eVector3 &size)
{
    const eVector3 halfeSize = size*0.5f;

    m_size = size;
    m_center = center;
    m_min = center-halfeSize;
    m_max = center+halfeSize;
}

const eVector3 & eAABB::getMin() const
{
    return m_min;
}

const eVector3 & eAABB::getMax() const
{
    return m_max;
}

const eVector3 & eAABB::getCenter() const
{
    return m_center;
}

const eVector3 & eAABB::getSize() const
{
    return m_size;
}

void eAABB::getCorners(eVector3 corners[8]) const
{
    corners[0] = m_min;
    corners[1] = eVector3(m_max.x, m_min.y, m_min.z);
    corners[2] = eVector3(m_max.x, m_max.y, m_min.z);
    corners[3] = eVector3(m_min.x, m_max.y, m_min.z);
    corners[4] = eVector3(m_min.x, m_min.y, m_max.z);
    corners[5] = eVector3(m_max.x, m_min.y, m_max.z);
    corners[6] = m_max;
    corners[7] = eVector3(m_min.x, m_max.y, m_max.z);
}

eBool eAABB::isEmpty() const
{
    const eInt ii = 0x7F800000;
    const eF32 inf = *(float *)&ii;
    const eF32 sqrLen = m_size.sqrLength();
    return (sqrLen == inf || eIsFloatZero(sqrLen));
}

eCollision eAABB::intersects(const eAABB &aabb) const
{
    const eVector3 min = aabb.m_min;
    const eVector3 max = aabb.m_max;

    if (min.x >= m_min.x && max.x <= m_max.x && min.y >= m_min.y && max.y <= m_max.y && min.z >= m_min.z && max.z <= m_max.z)
        return eCOL_INSIDE;
    else  if (m_max.x < min.x || m_min.x > max.x || m_max.y < min.y || m_min.y > max.y || m_max.z < min.z || m_min.z > max.z)
        return eCOL_OUTSIDE;
    else
        return eCOL_INTERSECTS;
}

eBool eAABB::contains(const eVector3 &pos)
{
    for (eU32 i=0; i<3; i++)
        if (pos[i] < m_min[i] || pos[i] > m_max[i])
            return eFALSE;

    return eTRUE;
}