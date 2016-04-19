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

#ifndef QUAT_HPP
#define QUAT_HPP

class eQuat
{
public:
    eQuat();
    eQuat(eF32 value);
    eQuat(const eMatrix4x4 &mtx);
    eQuat(const eVector3 &rot);
    eQuat(eF32 nx, eF32 ny, eF32 nz, eF32 nw);
    eQuat(const eVector3 &axis, eF32 angle);
    eQuat(const eVector3 &axis0, const eVector3 &axis1);

    eVector3        lerpAlongShortestArc(const eVector3 &axis0, const eVector3 &axis1, eF32 t);
    void            set(eF32 nx, eF32 ny, eF32 nz, eF32 nw);
    void            identity();
    void            negate();
    void            conjugate();
	eQuat           conjugated() const;
    eF32            length() const;
    eF32            sqrLength() const;
    void            normalize();
    eQuat           normalized() const;
    void            invert();
    eQuat           inverse() const;
    eF32            dot(const eQuat &q) const;
    eVector3        getVector(eU32 index) const;
    eVector3        toEuler() const;
    void            fromRotation(const eVector3 &v);
    void            fromMatrix(const eMatrix4x4 &m);
    void            fromAxisAngle(const eVector3 &axis, eF32 angle);
    eQuat           log() const;
    eQuat           exp() const;
    eQuat           lerp(eF32 t, const eQuat &to) const;
    eQuat           slerp(eF32 t, const eQuat &to) const;

    eQuat           operator + (const eQuat &q) const;
    eQuat           operator - (const eQuat &q) const;
    eQuat           operator * (const eQuat &q) const;
    eQuat           operator ^ (const eQuat &q) const;
    eQuat           operator - () const;
    eQuat           operator * (eF32 s) const;
    eQuat           operator / (eF32 s) const;
    eQuat           operator / (const eQuat &q) const;
    eQuat &         operator += (const eQuat &q);
    eQuat &         operator -= (const eQuat &q);
    eQuat &         operator *= (const eQuat &q);
    eQuat &         operator *= (eF32 s);
    eQuat &         operator /= (eF32 s);
    const eF32 &    operator [] (eInt index) const;
    eF32 &          operator [] (eInt index);
    operator        const eF32 * () const;
    
public:
    eF32            x;
    eF32            y;
    eF32            z;
    eF32            w;
};

#endif // QUAT_HPP