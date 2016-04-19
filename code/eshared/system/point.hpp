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

#ifndef POINT_HPP
#define POINT_HPP

struct eIXYZ
{
public:
    eInt            x;
    eInt            y;
    eInt            z;
};

class eIXY
{
public:
    union
    {
        struct
        {
            eInt    x;
            eInt    y;
        };

        struct
        {
            eInt    width;
            eInt    height;
        };
    };
};

// class encapsulates an integral point
class ePoint : public eIXY
{
public:
    ePoint();
    ePoint(const eIXY &ixy);
    ePoint(eInt nx, eInt ny);

    void        set(eInt nx, eInt ny);
    eF32        distance(const ePoint &p) const;
    void        minComponents(const ePoint &p);
    void        maxComponents(const ePoint &p);

    ePoint      operator + (const ePoint &p) const;
    ePoint      operator - (const ePoint &p) const;
    ePoint      operator * (eF32 s) const;
    ePoint &    operator += (const ePoint &p);
    ePoint &    operator -= (const ePoint &p);
    ePoint &    operator *= (eF32 s);
    eInt        operator [] (eInt index) const;
    eInt &      operator [] (eInt index);
    eBool       operator == (const ePoint &p) const;
    eBool       operator != (const ePoint &p) const;

    friend ePoint operator * (eF32 s, const ePoint &p)
    {
        return p*s;
    }
};

typedef ePoint eSize;

#endif // POINT_HPP