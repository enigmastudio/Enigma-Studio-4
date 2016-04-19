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

#ifndef RAY_HPP
#define RAY_HPP

class eRay
{
public:
    eRay(const eVector3 &origin, const eVector3 &dir);

    eVector3            evaluate(eF32 t) const;
    eBool               intersects(const ePlane &p, eVector3 *ip=nullptr) const;
    eBool               intersects(const eVector3 &triA, const eVector3 &triB, const eVector3 &triC, eVector3 *ip) const;

    void                setOrigin(const eVector3 &origin);
    void                setDirection(const eVector3 &dir);
    const eVector3 &    getOrigin() const;
    const eVector3 &    getDirection() const;

private:
    eVector3            m_origin;
    eVector3            m_dir;
};

#endif // RAY_HPP