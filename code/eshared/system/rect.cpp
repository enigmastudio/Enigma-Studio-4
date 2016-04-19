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

eRect::eRect()
{
    set(0, 0, 0, 0);
}

eRect::eRect(const eIXYXY &ixyxy)
{
    set(ixyxy.x0, ixyxy.y0, ixyxy.x1, ixyxy.y1);
}

eRect::eRect(eInt nx0, eInt ny0, eInt nx1, eInt ny1)
{
    set(nx0, ny0, nx1, ny1);
}

eRect::eRect(const ePoint &upperLeft, const ePoint &bottomRight)
{
    set(upperLeft.x, upperLeft.y, bottomRight.x, bottomRight.y);
}

void eRect::set(eInt nx0, eInt ny0, eInt nx1, eInt ny1)
{
    x0 = nx0;
    y0 = ny0;
    x1 = nx1;
    y1 = ny1;
}

void eRect::setWidth(eInt width)
{
    right = left+width;
}

void eRect::setHeight(eInt height)
{
    bottom = top+height;
}

void eRect::translate(eInt transX, eInt transY)
{
    x0 += transX;
    x1 += transX;
    y0 += transY;
    y1 += transY;
}

// might be negative (if rect isn't normalized)
eInt eRect::getWidth() const
{
    return x1-x0;
}

// might be negative (if rect isn't normalized)
eInt eRect::getHeight() const
{
    return y1-y0;
}

ePoint eRect::getCenter() const
{
    return ePoint((x0+x1)/2, (y0+y1)/2);
}

ePoint eRect::getUpperLeft() const
{
    return ePoint(x0, y0);
}

ePoint eRect::getBottomRight() const
{
    return ePoint(x1, y1);
}

// might be negative (rect isn't normalized)
eSize eRect::getSize() const
{
    return eSize(getWidth(), getHeight());
}

// makes sure that left < right and top < bottom
void eRect::normalize()
{
    if (x0 > x1)
        eSwap(x0, x1);
    if (y0 > y1)
        eSwap(y0, y1);
}

eBool eRect::pointInRect(const ePoint &p) const
{
    return (p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1);
}

eBool eRect::rectInRect(const eRect &r) const
{
    return (r.x0 >= x0 && r.x1 <= x1 && r.y0 >= y0 && r.y1 <= y1);
}

eBool eRect::intersect(const eRect &r) const
{
    return !(x0 > r.x1 || x1 < r.x0 || y0 > r.y1 || y1 < r.y0);
}

eInt eRect::operator [] (eInt index) const
{
    eASSERT(index < 4);
    return ((eInt *)this)[index];
}

eInt & eRect::operator [] (eInt index)
{
    eASSERT(index < 4);
    return ((eInt *)this)[index];
}

eBool eRect::operator == (const eRect &r) const
{
    return (r.x0 == x0 && r.y0 == y0 && r.x1 == x1 && r.y1 == y1);
}

eBool eRect::operator != (const eRect &r) const
{
    return !(*this == r);
}