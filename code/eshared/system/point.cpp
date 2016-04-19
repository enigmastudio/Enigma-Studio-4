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

#include "system.hpp"

ePoint::ePoint()
{
    x = 0;
    y = 0;
}

ePoint::ePoint(const eIXY &ixy)
{
    set(ixy.x, ixy.y);
}

ePoint::ePoint(eInt nx, eInt ny)
{
    set(nx, ny);
}

void ePoint::set(eInt nx, eInt ny)
{
    x = nx;
    y = ny;
}

eF32 ePoint::distance(const ePoint &p) const
{
    const eInt dx = x-p.x;
    const eInt dy = y-p.y;
    return eSqrt((eF32)(dx*dx+dy*dy));
}

void ePoint::minComponents(const ePoint &p)
{
    x = eMin(x, p.x);
    y = eMin(y, p.y);
}

void ePoint::maxComponents(const ePoint &p)
{
    x = eMax(x, p.x);
    y = eMax(y, p.y);
}

ePoint ePoint::operator + (const ePoint &p) const
{
    return ePoint(x+p.x, y+p.y);
}

ePoint ePoint::operator - (const ePoint &p) const
{
    return ePoint(x-p.x, y-p.y);
}

// scalar multiplication (scale)
ePoint ePoint::operator * (eF32 s) const
{
    return ePoint(eFtoL(x*s), eFtoL(y*s));
}

ePoint & ePoint::operator += (const ePoint &p)
{
    *this = *this+p;
    return *this;
}

ePoint & ePoint::operator -= (const ePoint &p)
{
    *this = *this-p;
    return *this;
}

ePoint & ePoint::operator *= (eF32 s)
{
    *this = *this*s;
    return *this;
}

eInt ePoint::operator [] (eInt index) const
{
    eASSERT(index < 2);
    return ((eInt *)this)[index];
}

eInt & ePoint::operator [] (eInt index)
{
    eASSERT(index < 2);
    return ((eInt *)this)[index];
}

eBool ePoint::operator == (const ePoint &p) const
{
    return (p.x == x && p.y == y);
}

eBool ePoint::operator != (const ePoint &p) const
{
    return !(*this == p);
}