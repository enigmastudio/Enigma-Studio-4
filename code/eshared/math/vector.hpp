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

#ifndef VECTOR_HPP
#define VECTOR_HPP

class eMatrix4x4;
class eMatrix3x3;
class ePlane;

class eFXY
{
public:
    union
    {
        struct
        {
            eF32    x;
            eF32    y;
        };

        struct
        {
            eF32    u;
            eF32    v;
        };
    };
};

class eFXYZ
{
public:
    struct
    {
        eF32    x;
        eF32    y;
        eF32    z;
    };
};

class eFXYZW
{
public:
    struct
    {
        eF32    x;
        eF32    y;
        eF32    z;
        eF32    w;
    };
};

// two-dimensional vector class
// also supports complex operations
class eVector2 : public eFXY
{
public:
    eVector2();
    eVector2(const eFXY &fxy);
    eVector2(eF32 value);
    eVector2(eF32 nx, eF32 ny);

    void            set(eF32 nx, eF32 ny);
    void            null();
    eF32            creal() const;
    eF32            cimaginary() const;
    eVector2        cconjugated() const;
    void            cconjugate();
    eVector2        cmul(const eVector2 &c) const;
    eVector2        cdiv(const eVector2 &c) const;
    eF32            cabs() const;
    eF32            carg() const;
    eVector2        cln() const;
    void            negate();
    eF32            length() const;
    eF32            sqrLength() const;
    eF32            distance(const eVector2 &v) const;
    eF32            distanceToLine(const eVector2 &v0, const eVector2 &v1) const;
    void            normalize();
    eVector2        normalized() const;
    eVector2        random() const;
    void            abs();
    void            minComponents(const eVector2 &v);
    void            maxComponents(const eVector2 &v);
    void            clamp(eF32 min, eF32 max);
    void            scale(const eVector2 &v);
    void            rotate(eF32 angle);
    void            rotate(const eVector2 &origin, eF32 angle);
    void            translate(const eVector2 &v);
    eBool           equals(const eVector2 &v) const;
    eVector2        midpoint(const eVector2 &v) const;
    eVector2        lerp(const eVector2 &to, eF32 t) const;
    eBool           isInsideTriangle(const eVector2 &a, const eVector2 &b, const eVector2 &c) const;
    static eF32     area2(const eVector2 &v0, const eVector2 &v1, const eVector2 &v2);

    eVector2        operator + (const eVector2 &v) const;
    eVector2        operator - (const eVector2 &v) const;
    eF32            operator * (const eVector2 &v) const;
    eVector2        operator * (eF32 s) const;
    eVector2        operator / (eF32 s) const;
    eVector2 &      operator += (const eVector2 &v);
    eVector2 &      operator -= (const eVector2 &v);
    eVector2 &      operator *= (eF32 s);
    eVector2 &      operator /= (eF32 s);
    eVector2        operator - () const;
    const eF32 &    operator [] (eInt index) const;
    eF32 &          operator [] (eInt index);
    operator        const eF32 * () const;

    friend eVector2 operator * (eF32 s, const eVector2 &v);
    friend eVector2 operator / (eF32 s, const eVector2 &v);
};

enum eVector3Const
{
    eVEC3_XAXIS,
    eVEC3_YAXIS,
    eVEC3_ZAXIS,
    eVEC3_ORIGIN,
 };

class eVector3 : public eFXYZ
{
public:
    eVector3();
    eVector3(eVector3Const vc);
    eVector3(const eFXYZ &fxyz);
    eVector3(const eIXYZ &ixyz);
    eVector3(eF32 value);
    eVector3(eF32 nx, eF32 ny, eF32 nz);
    eVector3(const eVector2 &xy, eF32 nz);
    eVector3(eF32 nx, const eVector2 &yz);

    void            set(eF32 nx, eF32 ny, eF32 nz);
    void            null();
    void            negate();
    eF32            length() const;
    eF32            sqrLength() const;
    eF32            distance(const eVector3 &v) const;
    eF32            distance(const eVector3 &support, const eVector3 &dir) const;
    eBool           isUniform() const;
    void            normalize();
    eVector3        normalized() const;
    eVector3        random() const;
    eVector3        arbitraryOrtho() const;
    eBool           isLinearDependent(const eVector3 &v) const;
    void            abs();
    void            minComponents(const eVector3 &v);
    void            maxComponents(const eVector3 &v);
    void            clamp(eF32 min, eF32 max);
    void            scale(const eVector3 &v);
    void            rotate(const eVector3 &v);
    void            rotate(const eVector3 &origin, const eVector3 &v);
    void            translate(const eVector3 &v);
    eVector3        midpoint(const eVector3 &v) const;
    eVector3        lerp(eF32 t, const eVector3 &to) const;
    eBool           isInsideCube(const ePlane cubePlanes[6]) const;

    static eVector3 catmullRom(eF32 t, const eVector3 &v0, const eVector3 &v1, const eVector3 &v2, const eVector3 &v3);
    static void     cubicBezier(eF32 t, const eVector3 &cp0, const eVector3 &cp1, const eVector3 &cp2,
                                const eVector3 &cp3, eVector3 &resPos, eVector3 &resTangent);

    eVector3        operator + (const eVector3 &v) const;
    eVector3        operator - (const eVector3 &v) const;
    eVector3        operator * (const eMatrix4x4 &m) const;
    eVector3        operator * (const eMatrix3x3 &m) const;
    eF32            operator * (const eVector3 &v) const;
    eVector3        operator * (eF32 s) const;
    eVector3        operator / (eF32 s) const;
    eVector3        operator ^ (const eVector3 &v) const;
    eVector3 &      operator += (const eVector3 &v);
    eVector3 &      operator -= (const eVector3 &v);
    eVector3 &      operator *= (eF32 s);
    eVector3 &      operator *= (const eMatrix4x4 &m);
    eVector3 &      operator *= (const eMatrix3x3 &m);
    eVector3 &      operator /= (eF32 s);
    eVector3        operator - () const;
    eVector3 &      operator = (const eColor &col);
    eBool           operator != (const eVector3 &v) const;
    eBool           operator == (const eVector3 &v) const;
    const eF32 &    operator [] (eInt index) const;
    eF32 &          operator [] (eInt index);
    operator        const eF32 * () const;

    friend eVector3 operator * (eF32 s, const eVector3 &v);
    friend eVector3 operator / (eF32 s, const eVector3 &v);

public:
    eF32 w; // allows SSE usage of vector
};

class eVector4 : public eFXYZW
{
public:
    eVector4();
    eVector4(const eFXYZW &fxyzw);
    eVector4(eF32 value);
    eVector4(eF32 nx, eF32 ny, eF32 nz, eF32 nw=1.0f);
    eVector4(const eVector2 &v0, const eVector2 &v1);
    eVector4(const eVector3 &v, eF32 w);

    void            set(eF32 nx, eF32 ny, eF32 nz, eF32 nw=1.0f);
    void            null();
    void            negate();
    void            normalize();
    void            abs();
    void            minComponents(const eVector4 &v);
    void            maxComponents(const eVector4 &v);
    void            clamp(eF32 min, eF32 max);
    void            scale(const eVector4 &v);
    void            translate(const eVector4 &v);

    eVector3        toVec3() const;
    eF32            length() const;
    eF32            sqrLength() const;
    eF32            distance(const eVector4 &v) const;
    eVector4        normalized() const;
    eVector4        random() const;
    eVector4        midpoint(const eVector4 &v) const;
    eVector4        lerp(const eVector4 &to, eF32 t) const;

    eVector4        operator + (const eVector4 &v) const;
    eVector4        operator - (const eVector4 &v) const;
    eF32            operator * (const eVector4 &v) const;
    eVector4        operator * (const eMatrix4x4 &m) const;
    eVector4        operator ^ (const eVector4 &v) const;
    eVector4        operator * (eF32 s) const;
    eVector4        operator / (eF32 s) const;
    eVector4 &      operator += (const eVector4 &v);
    eVector4 &      operator -= (const eVector4 &v);
    eVector4 &      operator *= (eF32 s);
    eVector4 &      operator *= (const eMatrix4x4 &m);
    eVector4 &      operator /= (eF32 s);
    eVector4        operator - () const;
    eVector4 &      operator = (const eColor &col);
    eBool           operator != (const eVector4 &v) const;
    eBool           operator == (const eVector4 &v) const;
    const eF32 &    operator [] (eInt index) const;
    eF32 &          operator [] (eInt index);

    friend eVector4 operator * (eF32 s, const eVector4 &v);
    friend eVector4 operator / (eF32 s, const eVector4 &v);
};

#endif // VECTOR_HPP