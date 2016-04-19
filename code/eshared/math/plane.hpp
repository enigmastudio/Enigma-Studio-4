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

#ifndef PLANE_HPP
#define PLANE_HPP

// the front side of a plane is the half
// space the normal is pointing to. the back
// side is the other one. the side on plane
// indicates the plane it-self.
enum ePlaneSide
{
    ePLS_FRONT,
    ePLS_BACK,
    ePLS_ON
};

// plane which is defined in 3D space
// by equation Ax+By+Cz+D=0
class ePlane
{
public:
    ePlane();
    ePlane(const eVector3 &v0, const eVector3 &v1, const eVector3 &v2);
    ePlane(const eVector3 &point, const eVector3 &normal);

    eBool               intersects(const eVector3 &lineSegA, const eVector3 &lineSegB, eVector3 *ip) const;
    ePlaneSide          getSide(const eVector3 &v) const;
    eF32                getDistance(const eVector3 &v) const;

    const eVector3 &    getNormal() const;
    const eVector3 &    getAbsNormal() const;
    eF32                getCoeffD() const;

private:
    eVector3            m_normal;
    eVector3            m_absNormal;
    eF32                m_d;
};

#endif // PLANE_HPP