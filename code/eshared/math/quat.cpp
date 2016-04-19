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

eQuat::eQuat() : x(0.0f), y(0.0f), z(0.0f), w(1.0f)
{
}

eQuat::eQuat(eF32 value) : x(value), y(value), z(value), w(value)
{
}

eQuat::eQuat(const eMatrix4x4 &mtx)
{
    fromMatrix(mtx);
}

eQuat::eQuat(const eVector3 &rot)
{
    fromRotation(rot);
}

eQuat::eQuat(eF32 nx, eF32 ny, eF32 nz, eF32 nw) : x(nx), y(ny), z(nz), w(nw)
{
}

eQuat::eQuat(const eVector3 &axis, eF32 angle)
{
    fromAxisAngle(axis, angle);
}

// shortest arc rotation between axis0 and axis1
eQuat::eQuat(const eVector3 &axis0, const eVector3 &axis1)
{
//        eASSERT(eAreFloatsEqual(axis0.sqrLength(), 1.0f));
//        eASSERT(eAreFloatsEqual(axis1.sqrLength(), 1.0f));

    (eVector3 &)x = axis0^axis1;

    eF32 axis0len = axis0.length();
    eF32 axis1len = axis1.length();

    w = eSqrt((axis0len * axis0len) * (axis1len * axis1len)) + (axis0 * axis1);
    normalize();
}

// lerped shortest arc rotation between axis0 and axis1
// returns z-vector of resulting rotation
eVector3 eQuat::lerpAlongShortestArc(const eVector3 &axis0, const eVector3 &axis1, eF32 t)
{
	t = eClamp(0.0f, t, 1.0f);
	eF32 dot = axis0 * axis1;
	if((dot > -0.9999f) && (dot < 0.9999f)) 
		*this = this->slerp(t, (eQuat(axis0, axis1) * *this).normalized());
	return this->getVector(2).normalized();
}


void eQuat::set(eF32 nx, eF32 ny, eF32 nz, eF32 nw)
{
    x = nx;
    y = ny;
    z = nz;
    w = nw;
}

void eQuat::identity()
{
    set(0.0f, 0.0f, 0.0f, 1.0f);
}

void eQuat::negate()
{
    set(-x, -y, -z, -w);
}

void eQuat::conjugate()
{
    set(-x, -y, -z, w);
}

eQuat eQuat::conjugated() const
{
	eQuat r(-x, -y, -z, w);
	return r;
}

eF32 eQuat::length() const
{
    return eSqrt(sqrLength());
}

eF32 eQuat::sqrLength() const
{
    return ((eVector3&)x).sqrLength()+w*w;
}

void eQuat::normalize()
{
    eASSERT(!eIsFloatZero(length()));
    const eF32 invLen = 1.0f/length();
    x *= invLen;
    y *= invLen;
    z *= invLen;
    w *= invLen;
}

eQuat eQuat::normalized() const
{
    eQuat q = *this;
    q.normalize();
    return q;
}

void eQuat::invert()
{
    const eF32 len = sqrLength();
    conjugate();
    *this /= len;
}

eQuat eQuat::inverse() const
{
    eQuat q = *this;
    q.invert();
    return q;
}

// operator * already overloaded with
// concatenation of two quaternions
eF32 eQuat::dot(const eQuat &q) const
{
    eQuat t = q;
    t.conjugate();
    return x*t.x+y*t.y+z*t.z;
}

eQuat eQuat::operator + (const eQuat &q) const
{
    return eQuat(x+q.x, y+q.y, z+q.z, w+q.w);
}

eQuat eQuat::operator - (const eQuat &q) const
{
    return eQuat(x-q.x, y-q.y, z-q.z, w-q.w);
}

// cross product
eQuat eQuat::operator ^ (const eQuat &q) const
{
    return ((*this)*q-q*(*this))*0.5f;
}

eQuat eQuat::operator - () const
{
    return eQuat(-x, -y, -z, -w);
}

// scalar multiplication (scale)
eQuat eQuat::operator * (eF32 s) const
{
    return eQuat(x*s, y*s, z*s, w*s);
}

eQuat eQuat::operator / (eF32 s) const
{
    eASSERT(!eIsFloatZero(s));
    return *this*(1.0f/s);
}

eQuat eQuat::operator / (const eQuat &q) const
{
    return *this*q.inverse();
}

eQuat & eQuat::operator += (const eQuat &q)
{
    *this = *this+q;
    return *this;
}

eQuat & eQuat::operator -= (const eQuat &q)
{
    *this = *this-q;
    return *this;
}

eQuat & eQuat::operator *= (const eQuat &q)
{
    *this = *this*q;
    return *this;
}

eQuat & eQuat::operator *= (eF32 s)
{
    *this = *this*s;
    return *this;
}

eQuat & eQuat::operator /= (eF32 s)
{
    *this = *this/s;
    return *this;
}

eQuat::operator const eF32 * () const
{
    return (eF32 *)this;
}

const eF32 & eQuat::operator [] (eInt index) const
{
    eASSERT(index < 4);
    return ((eF32 *)this)[index];
}

eF32 & eQuat::operator [] (eInt index) 
{
    eASSERT(index < 4);
    return ((eF32 *)this)[index];
}

eVector3 eQuat::getVector(eU32 index) const
{
    eASSERT(index < 3);

    const eF32 sqw = w*w;
    const eF32 sqx = x*x;
    const eF32 sqy = y*y;
    const eF32 sqz = z*z;
    const eF32 invs = 2.0f/(sqx+sqy+sqz+sqw);

    switch(index)
    {
        case 0:  return eVector3(0.5f*( sqx-sqy-sqz+sqw), x*y+z*w, x*z-y*w)*invs;
        case 1:  return eVector3(x*y-z*w, 0.5f*(-sqx+sqy-sqz+sqw), y*z+x*w)*invs;
        default: return eVector3(x*z+y*w, y*z-x*w, 0.5f*(-sqx-sqy+sqz+sqw))*invs;
    }
}

eVector3 eQuat::toEuler() const
{
    const eF32 ww = w*w;
    const eF32 xx = x*x;
    const eF32 yy = y*y;
    const eF32 zz = z*z;
    const eF32 rx = eATan2(2.0f*(y*z+x*w), -xx-yy+zz+ww);
    const eF32 ry = eASin(-2.0f*(x*z-y*w));
    const eF32 rz = eATan2(2.0f*(x*y+z*w), xx-yy-zz+ww);

    return eVector3(rx, ry, rz);
}

void eQuat::fromRotation(const eVector3 &v)
{
    const eF32 roll = v.x;
    const eF32 pitch = v.y;
    const eF32 yaw = v.z;
    const eF32 cyaw = eCos(0.5f*yaw);
    const eF32 cpitch = eCos(0.5f*pitch);
    const eF32 croll = eCos(0.5f*roll);
    const eF32 syaw = eSin(0.5f*yaw);
    const eF32 spitch = eSin(0.5f*pitch);
    const eF32 sroll = eSin(0.5f*roll);
    const eF32 cyawcpitch = cyaw*cpitch;
    const eF32 syawspitch = syaw*spitch;
    const eF32 cyawspitch = cyaw*spitch;
    const eF32 syawcpitch = syaw*cpitch;

    x = cyawcpitch*sroll-syawspitch*croll;
    y = cyawspitch*croll+syawcpitch*sroll;
    z = syawcpitch*croll-syawcpitch*sroll;
    w = cyawcpitch*croll+syawspitch*sroll;

    normalize();
}

void eQuat::fromMatrix(const eMatrix4x4 &m)
{
    const eF32 trace = m.m11+m.m22+m.m33+1.0f;

    if (!eIsFloatZero(trace)) 
    {
        const eF32 s = 0.5f/eSqrt(trace);
        x = (m.m32-m.m23)*s;
        y = (m.m13-m.m31)*s;
        z = (m.m21-m.m12)*s;
        w = 0.25f/s;
    } 
    else if (m.m11 > m.m22 && m.m11 > m.m33) 
    {
        const eF32 s = 2.0f*eSqrt(1.0f+m.m11-m.m22-m.m33);
        x = 0.25f*s;
        y = (m.m12+m.m21)/s;
        z = (m.m13+m.m31)/s;
        w = (m.m23-m.m32)/s;
    } 
    else if (m.m22 > m.m33) 
    {
        const eF32 s = 2.0f*eSqrt(1.0f+m.m22-m.m11-m.m33);
        x = (m.m12+m.m21)/s;
        y = 0.25f*s;
        z = (m.m23+m.m32)/s;
        w = (m.m13-m.m31)/s;
    } 
    else 
    {
        const eF32 s = 2.0f*eSqrt(1.0f+m.m33-m.m11-m.m22);
        x = (m.m13+m.m31)/s;
        y = (m.m23+m.m32)/s;
        z = 0.25f*s;
        w = (m.m12-m.m21)/s;
    }
}

// axis has to be normalized and angle in radians
void eQuat::fromAxisAngle(const eVector3 &axis, eF32 angle)
{
    eASSERT(eAreFloatsEqual(axis.sqrLength(), 1.0f));

    eF32 sa, ca;
    eSinCos(angle*0.5f, sa, ca);
    (eFXYZ &)x = axis*sa;
    w = ca;
}

eQuat eQuat::lerp(eF32 t, const eQuat &to) const
{
    return eQuat(eLerp(x, to.x, t),
                 eLerp(y, to.y, t),
                 eLerp(z, to.z, t),
                 eLerp(w, to.w, t));
}

eQuat eQuat::slerp(eF32 t, const eQuat &to) const
{
    eASSERT(t >= 0.0f && t <= 1.0f);

    eQuat qb = to;
    // quaternion to return
    eQuat qm;
    // Calculate angle between them.
    eF32 cosHalfTheta = this->w * qb.w + this->x * qb.x + this->y * qb.y + this->z * qb.z;
    if (cosHalfTheta < 0.0f) {
      qb.w = -qb.w; qb.x = -qb.x; qb.y = -qb.y; qb.z = qb.z;
      cosHalfTheta = -cosHalfTheta;
    }
    // if qa=qb or qa=-qb then theta = 0 and we can return qa
    if (eAbs(cosHalfTheta) >= 1.0f){
        qm.w = this->w;
        qm.x = this->x;
        qm.y = this->y;
        qm.z = this->z;
        return qm;
    }
    // Calculate temporary values.
    eF32 halfTheta = eACos(cosHalfTheta);
    eF32 sinHalfTheta = eSqrt(1.0f - cosHalfTheta*cosHalfTheta);
    // if theta = 180 degrees then result is not fully defined
    // we could rotate around any axis normal to qa or qb
    if (eAbs(sinHalfTheta) < 0.001f){ // fabs is floating point absolute
        qm.w = (this->w * 0.5f + qb.w * 0.5f);
        (eFXYZ&)qm.x = ((eVector3&)this->x * 0.5f + (eVector3&)qb.x * 0.5f);
        return qm;
    }
    eF32 ratioA = eClamp(0.0f, eSin((1.0f - t) * halfTheta) / sinHalfTheta, 1.0f);
    eF32 ratioB = eClamp(0.0f, eSin(t * halfTheta) / sinHalfTheta, 1.0f); 
    //calculate Quaternion.
    qm.w = (this->w * ratioA + qb.w * ratioB);
    (eFXYZ&)qm.x = ((eVector3&)this->x * ratioA + (eVector3&)qb.x * ratioB);
    return qm;

}

eQuat eQuat::log() const
{
    const eF32 theta = eACos(w);
    const eF32 sinTheta = eSin(theta);

    if (!eIsFloatZero(sinTheta))
    {
        const eVector3 a = (eVector3 &)x/sinTheta*theta;
        return eQuat(a.x, a.y, a.z, 0.0f);
    }

    return eQuat(x, y, z, 0.0f);
}

eQuat eQuat::exp() const
{
    const eF32 theta = eSqrt((eVector3 &)x*(eVector3 &)x);

    eF32 sinTheta, cosTheta;
    eSinCos(theta, sinTheta, cosTheta);

    if (!eIsFloatZero(sinTheta))
    {
        const eVector3 a = (eVector3 &)x*sinTheta/theta;
        return eQuat(a.x, a.y, a.z, cosTheta);
    }
 
    return eQuat(x, y, z, cosTheta);
}

// standard quaternion multiplication
// (not commutative):
// (s,v)*(t,w) = (s*t-<v,w>,sw+tv+(v x w))
eQuat eQuat::operator * (const eQuat &q) const
{
    const eF32 w2 = w*q.w-(eVector3 &)x*(eVector3 &)q.x;
    
    const eVector3 vcp = (eVector3 &)x^(eVector3 &)q.x;
    const eVector3 vq0 = (eVector3 &)x*q.w;
    const eVector3 vq1 = (eVector3 &)q.x*w;
    const eVector3 res = vq0+vq1+vcp;

    return eQuat(res.x, res.y, res.z, w2);
}