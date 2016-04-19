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

#ifndef AABB_HPP
#define AABB_HPP

enum eCollision
{
    eCOL_INSIDE,
    eCOL_OUTSIDE,
    eCOL_INTERSECTS
};

class eAABB
{
public:
    eAABB();
    eAABB(const eVector3 &min, const eVector3 &max);
    eAABB(const eVector3 &center, eF32 width, eF32 height, eF32 depth);

    void                clear();
    void                merge(const eAABB &aabb);
    void                updateExtent(const eVector3 &v);
    void                translate(const eVector3 &v);
    void                scale(const eVector3 &v);
    void                transform(const eTransform &transf);
    void                transform(const eMatrix4x4 &mtx);
    void                setMinMax(const eVector3 &min, const eVector3 &max);
    void                setCenterSize(const eVector3 &center, const eVector3 &size);

    const eVector3 &    getMin() const;
    const eVector3 &    getMax() const;
    const eVector3 &    getCenter() const;
    const eVector3 &    getSize() const;
    void                getCorners(eVector3 corners[8]) const;

    eBool               isEmpty() const;
    eCollision          intersects(const eAABB &aabb) const;
    eBool               contains(const eVector3 &pos);

private:
    eVector3            m_min;
    eVector3            m_max;
    eVector3            m_center;
    eVector3            m_size;
};

#endif // AABB_HPP