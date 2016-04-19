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

#ifndef RECT_HPP
#define RECT_HPP

class eIXYXY
{
public:
    union
    {
        struct
        {
            eInt    left;
            eInt    top;
            eInt    right;
            eInt    bottom;
        };

        struct
        {
            eInt    x0;
            eInt    y0;
            eInt    x1;
            eInt    y1;
        };
    };
};

// defines the coordinates of the upper-left
// and lower-right corners of a rectangle
class eRect : public eIXYXY
{
public:
    eRect();
    eRect(const eIXYXY &ixyxy);
    eRect(eInt nx0, eInt ny0, eInt nx1, eInt ny1);
    eRect(const ePoint &upperLeft, const ePoint &bottomRight);

    void    set(eInt nx0, eInt ny0, eInt nx1, eInt ny1);
    void    setWidth(eInt width);
    void    setHeight(eInt height);
    void    translate(eInt transX, eInt transY);
    eInt    getWidth() const;
    eInt    getHeight() const;
    ePoint  getCenter() const;
    ePoint  getUpperLeft() const;
    ePoint  getBottomRight() const;
    eSize   getSize() const;
    void    normalize();
    eBool   pointInRect(const ePoint &p) const;
    eBool   rectInRect(const eRect &r) const;
    eBool   intersect(const eRect &r) const;

    eInt    operator [] (eInt index) const;
    eInt &  operator [] (eInt index);
    eBool   operator == (const eRect &r) const;
    eBool   operator != (const eRect &r) const;
};

#endif // RECT_HPP