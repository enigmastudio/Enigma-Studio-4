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

eVector2::eVector2()
{
    null();
}

eVector2::eVector2(const eFXY &fxy)
{
    set(fxy.x, fxy.y);
}

eVector2::eVector2(eF32 value)
{
    set(value, value);
}

eVector2::eVector2(eF32 nx, eF32 ny)
{
    set(nx, ny);
}

void eVector2::set(eF32 nx, eF32 ny)
{
    x = nx;
    y = ny;
}

void eVector2::null()
{
    set(0.0f, 0.0f);
}

// complex operations

eF32 eVector2::creal() const
{
    return x;
}

eF32 eVector2::cimaginary() const
{
    return y;
}

eVector2 eVector2::cconjugated() const
{
    return eVector2(x, -y);
}

void eVector2::cconjugate()
{
    y = -y;
}

eVector2 eVector2::cmul(const eVector2 &c) const
{
    return eVector2(x*c.x-y*c.y, y*c.x+x*c.y);
}

eVector2 eVector2::cdiv(const eVector2 &c) const
{
    const eF32 div = c.x*c.x+c.y*c.y;
    return eVector2(x*c.x+y*c.y/div, y*c.x-x*c.y/div);
}

eF32 eVector2::cabs() const
{
    return eSqrt(x*x+y*y);
}

eF32 eVector2::carg() const
{
    return eATan2(y, x);
}

eVector2 eVector2::cln() const
{
    return eVector2(eLn(cabs()), carg());
}

// vector operations

void eVector2::negate()
{
    set(-x, -y);
}

eF32 eVector2::length() const
{
    return eSqrt(sqrLength());
}

eF32 eVector2::sqrLength() const
{
    return x*x+y*y;
}

eF32 eVector2::distance(const eVector2 &v) const
{
    return ((*this)-v).length();
}

eF32 eVector2::distanceToLine(const eVector2 &v0, const eVector2 &v1) const
{
    const eF32 u = ((x-v0.x)*(v1.x-v0.x)+(y-v0.y)*(v1.y-v0.y))/(v1-v0).sqrLength();
    const eVector2 pl = v0+u*(v1-v0);
    return distance(pl);
}

void eVector2::normalize()
{
    // prevent danger of division by 0
    if (eIsFloatZero(length()))
        return;

    const eF32 invLen = 1.0f/length();
    x *= invLen;
    y *= invLen;
}

eVector2 eVector2::normalized() const
{
    eVector2 n = *this;
    n.normalize();
    return n;
}

eVector2 eVector2::random() const
{
    return eVector2(x*eRandomF(-1.0f, 1.0f), y*eRandomF(-1.0f, 1.0f));
}

void eVector2::abs()
{
    x = eAbs(x);
    y = eAbs(y);
}

void eVector2::minComponents(const eVector2 &v)
{
    x = eMin(x, v.x);
    y = eMin(y, v.y);
}

void eVector2::maxComponents(const eVector2 &v)
{
    x = eMax(x, v.x);
    y = eMax(y, v.y);
}

void eVector2::clamp(eF32 min, eF32 max)
{
    x = eClamp(min, x, max);
    y = eClamp(min, y, max);
}

void eVector2::scale(const eVector2 &v)
{
    x *= v.x;
    y *= v.y;
}

// angle expected in radians
void eVector2::rotate(eF32 angle)
{
    eF32 s, c;
    eSinCos(angle, s, c);
        
    const eF32 nx = x*c-y*s;
    const eF32 ny = x*s+y*c;
    x = nx;
    y = ny;
}

// angle expected in radians
void eVector2::rotate(const eVector2 &origin, eF32 angle)
{
    *this -= origin;
    rotate(angle);
    *this += origin;
}

void eVector2::translate(const eVector2 &v)
{
    *this += v;
}

eBool eVector2::equals(const eVector2 &v) const
{
    return (v.x == this->x) && (v.y == this->y);
}

eVector2 eVector2::midpoint(const eVector2 &v) const
{
    return (*this+v)*0.5f;
}

// linear interpolation (0 <= t <= 1)
eVector2 eVector2::lerp(const eVector2 &to, eF32 t) const
{
    eASSERT(t >= 0.0f && t <= 1.0f);
    return eVector2(*this+(to-*this)*t);
}

eBool eVector2::isInsideTriangle(const eVector2 &a, const eVector2 &b, const eVector2 &c) const
{
    const eVector2 &p = *this;
    const eVector2 &v0 = p-c;
    const eVector2 &v1 = a-c;
    const eVector2 &v2 = b-c;

    const eF32 det = v1.x*v2.y-v1.y*v2.x;
    eASSERT(eIsFloatZero(det) == eFALSE);
    const eF32 invDet = 1.0f/det;

    const eF32 u = (v0.x*v2.y-v0.y*v2.x)*invDet;
    const eF32 v = (v1.x*v0.y-v1.y*v0.x)*invDet;

    return (u >= 0.0f && v >= 0.0f && u+v <= 1.0f);
}

eVector2 eVector2::operator + (const eVector2 &v) const
{
    return eVector2(x+v.x, y+v.y);
}

eVector2 eVector2::operator - (const eVector2 &v) const
{
    return eVector2(x-v.x, y-v.y);
}

// dot product
eF32 eVector2::operator * (const eVector2 &v) const
{
    return x*v.x+y*v.y;
}

// scalar multiplication (scale)
eVector2 eVector2::operator * (eF32 s) const
{
    return eVector2(x*s, y*s);
}

eVector2 eVector2::operator / (eF32 s) const
{
    eASSERT(!eIsFloatZero(s));
    return *this*(1.0f/s);
}

eVector2 & eVector2::operator += (const eVector2 &v)
{
    *this = *this+v;
    return *this;
}

eVector2 & eVector2::operator -= (const eVector2 &v)
{
    *this = *this-v;
    return *this;
}

eVector2 & eVector2::operator *= (eF32 s)
{
    *this = *this*s;
    return *this;
}

eVector2 & eVector2::operator /= (eF32 s)
{
    *this = *this/s;
    return *this;
}

eVector2 eVector2::operator - () const
{
    return eVector2(-x, -y);
}

eVector2::operator const eF32 * () const
{
    return (eF32 *)this;
}

const eF32 & eVector2::operator [] (eInt index) const
{
    eASSERT(index < 2);
    return ((eF32 *)this)[index];
}

eF32 & eVector2::operator [] (eInt index)
{
    eASSERT(index < 2);
    return ((eF32 *)this)[index];
}

// returns the 2*area of then given 2d triangle, 
// signum of returned value tells about the orientation
eF32 eVector2::area2(const eVector2 &v0, const eVector2 &v1, const eVector2 &v2)
{
    return ((v1.x-v0.x)*(v2.y-v0.y)-(v2.x-v0.x)*(v1.y-v0.y));
}

eVector2 operator * (eF32 s, const eVector2 &v)
{
    return v*s;
}

eVector2 operator / (eF32 s, const eVector2 &v)
{
    return eVector2(s/v.x,s/v.y);
}

eVector3::eVector3()
{
    null();
}

eVector3::eVector3(eVector3Const vc)
{
    static const eVector3 vecs[] =
    {
        eVector3(1.0f, 0.0f, 0.0f),
        eVector3(0.0f, 1.0f, 0.0f),
        eVector3(0.0f, 0.0f, 1.0f),
        eVector3(0.0f, 0.0f, 0.0f)
    };

    *this = vecs[vc];
}

eVector3::eVector3(const eFXYZ &fxyz)
{
    set(fxyz.x, fxyz.y, fxyz.z);
    w = 0;
}

eVector3::eVector3(const eIXYZ &ixyz)
{
    set((eF32)ixyz.x, (eF32)ixyz.y, (eF32)ixyz.z);
    w = 0;
}

// sets x, y and z to value
eVector3::eVector3(eF32 value)
{
    set(value, value, value);
    w = 0;
}

eVector3::eVector3(eF32 nx, eF32 ny, eF32 nz)
{
    set(nx, ny, nz);
    w = 0;
}

eVector3::eVector3(const eVector2 &xy, eF32 nz)
{
    set(xy.x, xy.y, nz);
    w = 0;
}

eVector3::eVector3(eF32 nx, const eVector2 &yz)
{
    set(nx, yz.x, yz.y);
    w = 0;
}

void eVector3::set(eF32 nx, eF32 ny, eF32 nz)
{
    x = nx;
    y = ny;
    z = nz;
}

void eVector3::null()
{
    set(0.0f, 0.0f, 0.0f);
    w = 0;
}

void eVector3::negate()
{
    set(-x, -y, -z);
}

eF32 eVector3::length() const
{
    return eSqrt(sqrLength());
}

eF32 eVector3::sqrLength() const
{
    return x*x+y*y+z*z;
}

eF32 eVector3::distance(const eVector3 &v) const
{
    return ((*this)-v).length();
}

eF32 eVector3::distance(const eVector3 &support, const eVector3 &dir) const
{
    return ((*this-support)^dir).length()/dir.length();
}

eBool eVector3::isUniform() const
{
    return (eAreFloatsEqual(x, y) &&
            eAreFloatsEqual(x, z) &&
            eAreFloatsEqual(y, z));
}

void eVector3::normalize()
{
    const eF32 sqrLen = sqrLength();
    if (eIsFloatZero(sqrLen))
        return;

    const eF32 len = eSqrt(sqrLen);
    const eF32 invLen = 1.0f/len;
    *this *= invLen;
}

eVector3 eVector3::normalized() const
{
    eVector3 n = *this;
    n.normalize();
    return n;
}

eVector3 eVector3::random() const
{
    return eVector3(x*eRandomF(-1.0f, 1.0f),
                    y*eRandomF(-1.0f, 1.0f),
                    z*eRandomF(-1.0f, 1.0f));
}

// based on the method:
// Michael M. Stark, Efficient Construction of Perpendicular
// Vectors without Branching, Journal of Graphics, GPU and
// Game Tools, Vol. 14, No. 1: 55-62, 2009
eVector3 eVector3::arbitraryOrtho() const
{
    const eU32 uyx = eSignBit(eAbs(x)-eAbs(y));
    const eU32 uzx = eSignBit(eAbs(x)-eAbs(z));
    const eU32 uzy = eSignBit(eAbs(y)-eAbs(z));
    const eU32 xm = uyx&uzx;
    const eU32 ym = (1^xm)&uzy;
    const eU32 zm = 1^(xm&ym);

    return eVector3(zm*y-ym*z, xm*z-zm*x, ym*x-xm*y);
}

eBool eVector3::isLinearDependent(const eVector3 &v) const
{
    return eIsFloatZero(((*this)^v).sqrLength());
}

void eVector3::abs()
{
    x = eAbs(x);
    y = eAbs(y);
    z = eAbs(z);
}

void eVector3::minComponents(const eVector3 &v)
{
    x = eMin(x, v.x);
    y = eMin(y, v.y);
    z = eMin(z, v.z);
}

void eVector3::maxComponents(const eVector3 &v)
{
    x = eMax(x, v.x);
    y = eMax(y, v.y);
    z = eMax(z, v.z);
}

void eVector3::clamp(eF32 min, eF32 max)
{
    x = eClamp(min, x, max);
    y = eClamp(min, y, max);
    z = eClamp(min, z, max);
}

void eVector3::scale(const eVector3 &v)
{
    x *= v.x;
    y *= v.y;
    z *= v.z;
}

// angles in radians
void eVector3::rotate(const eVector3 &v)
{
    eF32 sx, cx, sy, cy, sz, cz;
    eSinCos(v.x, sx, cx);
    eSinCos(v.y, sy, cy);
    eSinCos(v.z, sz, cz);

    eF32 temp = y*cx-z*sx; // around x-axis
    z = y*sx+z*cx;
    y = temp;

    temp = z*sy+x*cy; // around y-axis
    z = z*cy-x*sy;
    x = temp;

    temp = x*cz-y*sz; // around z-axis
    y = x*sz+y*cz;
    x = temp;
}

// rotation arond given origin (angles in radians)
void eVector3::rotate(const eVector3 &origin, const eVector3 &v)
{
    *this -= origin;
    rotate(v);
    *this += origin;
}

void eVector3::translate(const eVector3 &v)
{
    *this += v;
}

eVector3 eVector3::midpoint(const eVector3 &v) const
{
    return (*this+v)*0.5f;
}

// linear interpolation
eVector3 eVector3::lerp(eF32 t, const eVector3 &to) const
{
    return eVector3(*this+(to-*this)*t);
}

eVector3 eVector3::operator + (const eVector3 &v) const
{
    return eVector3(x+v.x, y+v.y, z+v.z);
}

eVector3 eVector3::operator - (const eVector3 &v) const
{
    return eVector3(x-v.x, y-v.y, z-v.z);
}

// dot product
eF32 eVector3::operator * (const eVector3 &v) const
{
    return x*v.x+y*v.y+z*v.z;
}

// scalar multiplication
eVector3 eVector3::operator * (eF32 s) const
{
    return eVector3(x*s, y*s, z*s);
}

eVector3 eVector3::operator / (eF32 s) const
{
    eASSERT(!eIsFloatZero(s));
    return *this*(1.0f/s);
}

// cross product
eVector3 eVector3::operator ^ (const eVector3 &v) const
{
    return eVector3(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x);
}

eVector3 & eVector3::operator += (const eVector3 &v)
{
    *this = *this+v;
    return *this;
}

eVector3 & eVector3::operator -= (const eVector3 &v)
{
    *this = *this-v;
    return *this;
}

eVector3 & eVector3::operator *= (eF32 s)
{
    *this = *this*s;
    return *this;
}

eVector3 & eVector3::operator *= (const eMatrix4x4 &m)
{
    *this = *this*m;
    return *this;
}

eVector3 & eVector3::operator *= (const eMatrix3x3 &m)
{
    *this = *this*m;
    return *this;
}

eVector3 & eVector3::operator /= (eF32 s)
{
    *this = *this/s;
    return *this;
}

eVector3 eVector3::operator - () const
{
    return eVector3(-x, -y, -z);
}

eVector3 & eVector3::operator = (const eColor &col)
{
    x = (eF32)col.r/255.0f;
    y = (eF32)col.g/255.0f;
    z = (eF32)col.b/255.0f;
    return *this;
}

eBool eVector3::operator != (const eVector3 &v) const
{
    return (!eAreFloatsEqual(x, v.x) ||
            !eAreFloatsEqual(y, v.y) ||
            !eAreFloatsEqual(z, v.z));
}

eBool eVector3::operator == (const eVector3 &v) const
{
    return !(*this != v);
}

eVector3::operator const eF32 * () const
{
    return (eF32 *)this;
}

const eF32 & eVector3::operator [] (eInt index) const
{
    eASSERT(index < 3);
    return ((eF32 *)this)[index];
}

eF32 & eVector3::operator [] (eInt index)
{
    eASSERT(index < 3);
    return ((eF32 *)this)[index];
}

eVector3 eVector3::catmullRom(eF32 t, const eVector3 &v0, const eVector3 &v1, const eVector3 &v2, const eVector3 &v3)
{
    const eF32 tt = t*t;
    return 0.5f*((2.0f*v1)+(-v0+v2)*t+(2.0f*v0-5.0f*v1+4.0f*v2-v3)*tt+(-v0+3.0f*(v1-v2)+v3)*tt*t);
}

eVector3 eVector3::operator * (const eMatrix4x4 &m) const
{
    return eVector3(x*m.m11+y*m.m21+z*m.m31+m.m41,
                    x*m.m12+y*m.m22+z*m.m32+m.m42,
                    x*m.m13+y*m.m23+z*m.m33+m.m43);
}

eVector3 eVector3::operator * (const eMatrix3x3 &m) const
{
    return eVector3(x*m.m11+y*m.m21+z*m.m31,
                    x*m.m12+y*m.m22+z*m.m32,
                    x*m.m13+y*m.m23+z*m.m33);
}

eBool eVector3::isInsideCube(const ePlane cubePlanes[6]) const
{
    for (eU32 i=0; i<6; i++)
        if (cubePlanes[i].getSide(*this) == ePLS_BACK)
            return eFALSE;

    return eTRUE;
}

void eVector3::cubicBezier(eF32 t, const eVector3 &cp0, const eVector3 &cp1, const eVector3 &cp2, const eVector3 &cp3, eVector3 &resPos, eVector3 &resTangent)
{
    __m128 m3 = _mm_set1_ps(3);
    __m128 mt = _mm_set1_ps(t);
    __m128 mt3 = _mm_mul_ps(mt, m3);
    __m128 mtt = _mm_mul_ps(mt, mt);
    __m128 mtt3 = _mm_mul_ps(mtt, m3);
    __m128 mtinv = _mm_set1_ps(1.0f - t);
    __m128 mtinv3 = _mm_mul_ps(mtinv, m3);
    __m128 mttinv = _mm_mul_ps(mtinv, mtinv);
    __m128 mttinv3 = _mm_mul_ps(mttinv, m3);
    
	__m128 mcp0 = _mm_loadu_ps(&cp0.x);
    __m128 mcp1 = _mm_loadu_ps(&cp1.x);
	__m128 respos0 = _mm_add_ps(_mm_mul_ps(mcp0, _mm_mul_ps(mttinv, mtinv)), 
		                        _mm_mul_ps(mcp1, _mm_mul_ps(mt3, mttinv)));
	__m128 mt_tinv_6 = _mm_mul_ps(mt3, mtinv3);
	__m128 restan0 = _mm_sub_ps(_mm_mul_ps(mcp1, _mm_sub_ps(mttinv3, mt_tinv_6)), 
		                        _mm_mul_ps(mcp0, mttinv3));

	__m128 mcp2 = _mm_loadu_ps(&cp2.x);
    __m128 mcp3 = _mm_loadu_ps(&cp3.x);
	__m128 respos1 = _mm_add_ps(_mm_mul_ps(mcp2, _mm_mul_ps(mtinv3, mtt)), 
		                        _mm_mul_ps(mcp3, _mm_mul_ps(mtt, mt)));
	__m128 restan1 = _mm_add_ps(_mm_mul_ps(mcp2, _mm_sub_ps(mt_tinv_6, mtt3)), 
		                        _mm_mul_ps(mcp3, mtt3));
	_mm_storeu_ps(&resPos.x, _mm_add_ps(respos0, respos1));
	_mm_storeu_ps(&resTangent.x, _mm_add_ps(restan0, restan1));
/*
	const eF32 tt = t * t;
    const eF32 tinv = 1.0f-t;
    const eF32 ttinv = tinv * tinv;
	resPos = (tinv * ttinv) * cp0 + 
			    (3.0f * t * ttinv) * cp1 + 
				(3.0f * tt * tinv) * cp2 + 
				(t * tt) * cp3;
		
	resTangent = -(3.0f * ttinv) * cp0 + 
			        ((3.0f * ttinv) - (6.0f * t * tinv)) * cp1 +
					((6.0f * t * tinv) - (3.0f * tt)) * cp2 +
					(3.0f * tt) * cp3;
*/
}

eVector3 operator * (eF32 s, const eVector3 &v)
{
    return v*s;
}

eVector3 operator / (eF32 s, const eVector3 &v)
{
    return eVector3(s/v.x,s/v.y,s/v.z);
}

eVector4::eVector4()
{
    set(0.0f, 0.0f, 0.0f, 1.0f);
}

eVector4::eVector4(const eFXYZW &fxyzw)
{
    set(fxyzw.x, fxyzw.y, fxyzw.z, fxyzw.w);
}

eVector4::eVector4(eF32 value)
{
    set(value, value, value, value);
}

eVector4::eVector4(eF32 nx, eF32 ny, eF32 nz, eF32 nw)
{
    set(nx, ny, nz, nw);
}

eVector4::eVector4(const eVector2 &v0, const eVector2 &v1)
{
    set(v0.x, v0.y, v1.x, v1.y);
}

eVector4::eVector4(const eVector3 &v, eF32 w)
{
    set(v.x, v.y, v.z, w);
}

void eVector4::set(eF32 nx, eF32 ny, eF32 nz, eF32 nw)
{
    x = nx;
    y = ny;
    z = nz;
    w = nw;
}

void eVector4::null()
{
    set(0.0f, 0.0f, 0.0f, 0.0f);
}

eVector3 eVector4::toVec3() const
{
    return eVector3(x, y, z);
}

void eVector4::negate()
{
    set(-x, -y, -z, -w);
}

eF32 eVector4::length() const
{
    return eSqrt(sqrLength());
}

eF32 eVector4::sqrLength() const
{
    return x*x+y*y+z*z+w*w;
}

eF32 eVector4::distance(const eVector4 &v) const
{
    return ((*this)-v).length();
}

void eVector4::normalize()
{
    if (eIsFloatZero(length()))
        return;

    const eF32 invLen = 1.0f/length();
    x *= invLen;
    y *= invLen;
    z *= invLen;
    w *= invLen;
}

eVector4 eVector4::normalized() const
{
    eVector4 n = *this;

    n.normalize();
    return n;
}

eVector4 eVector4::random() const
{
    return eVector4(x*eRandomF(-1.0f, 1.0f),
                    y*eRandomF(-1.0f, 1.0f),
                    z*eRandomF(-1.0f, 1.0f),
                    w*eRandomF(-1.0f, 1.0f));
}

void eVector4::abs()
{
    x = eAbs(x);
    y = eAbs(y);
    z = eAbs(z);
    w = eAbs(w);
}

void eVector4::minComponents(const eVector4 &v)
{
    x = eMin(x, v.x);
    y = eMin(y, v.y);
    z = eMin(z, v.z);
    w = eMin(w, v.w);
}

void eVector4::maxComponents(const eVector4 &v)
{
    x = eMax(x, v.x);
    y = eMax(y, v.y);
    z = eMax(y, v.z);
    w = eMax(y, v.w);
}

void eVector4::clamp(eF32 min, eF32 max)
{
    x = eClamp(min, x, max);
    y = eClamp(min, y, max);
    z = eClamp(min, z, max);
    w = eClamp(min, w, max);
}

void eVector4::scale(const eVector4 &v)
{
    x *= v.x;
    y *= v.y;
    z *= v.z;
    w *= v.w;
}

void eVector4::translate(const eVector4 &v)
{
    *this += v;
}

eVector4 eVector4::midpoint(const eVector4 &v) const
{
    return (*this+v)*0.5f;
}

// linear interpolation (0 <= t <= 1)
eVector4 eVector4::lerp(const eVector4 &to, eF32 t) const
{
    eASSERT(t >= 0.0f && t <= 1.0f);
    return eVector4(*this+(to-*this)*t);
}

eVector4 eVector4::operator + (const eVector4 &v) const
{
    return eVector4(x+v.x, y+v.y, z+v.z, w+v.w);
}

eVector4 eVector4::operator - (const eVector4 &v) const
{
    return eVector4(x-v.x, y-v.y, z-v.z, w-v.w);
}

// dot product
eF32 eVector4::operator * (const eVector4 &v) const
{
    return x*v.x+y*v.y+z*v.z+w*v.w;
}

eVector4 eVector4::operator ^ (const eVector4 &v) const
{
    return eVector4(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x, 1.0f);
}

// scalar multiplication (scale)
eVector4 eVector4::operator * (eF32 s) const
{
    return eVector4(x*s, y*s, z*s, w*s);
}

eVector4 eVector4::operator / (eF32 s) const
{
    return *this*(1.0f/s);
}

eVector4 & eVector4::operator += (const eVector4 &v)
{
    *this = *this+v;
    return *this;
}

eVector4 & eVector4::operator -= (const eVector4 &v)
{
    *this = *this-v;
    return *this;
}

eVector4 & eVector4::operator *= (eF32 s)
{
    *this = *this*s;
    return *this;
}

eVector4 & eVector4::operator *= (const eMatrix4x4 &m)
{
    *this = *this*m;
    return *this;
}

eVector4 & eVector4::operator /= (eF32 s)
{
    *this = *this/s;
    return *this;
}

eVector4 eVector4::operator - () const
{
    return eVector4(-x, -y, -z, -w);
}

eVector4 & eVector4::operator = (const eColor &col)
{
    x = (eF32)col.r/255.0f;
    y = (eF32)col.g/255.0f;
    z = (eF32)col.b/255.0f;
    w = (eF32)col.a/255.0f;
    return *this;
}

eBool eVector4::operator != (const eVector4 &v) const
{
    return (!eAreFloatsEqual(x, v.x) ||
            !eAreFloatsEqual(y, v.y) ||
            !eAreFloatsEqual(z, v.z) || 
            !eAreFloatsEqual(w, v.w));
}

eBool eVector4::operator == (const eVector4 &v) const
{
    return !(*this != v);
}

const eF32 & eVector4::operator [] (eInt index) const
{
    eASSERT(index < 4);
    return ((eF32 *)this)[index];
}

eF32 & eVector4::operator [] (eInt index)
{
    eASSERT(index < 4);
    return ((eF32 *)this)[index];
}

// vector * matrix
eVector4 eVector4::operator * (const eMatrix4x4 &m) const
{
    return eVector4((x*m.m11)+(y*m.m21)+(z*m.m31)+(w*m.m41),
                    (x*m.m12)+(y*m.m22)+(z*m.m32)+(w*m.m42),
                    (x*m.m13)+(y*m.m23)+(z*m.m33)+(w*m.m43),
                    (x*m.m14)+(y*m.m24)+(z*m.m34)+(w*m.m44));
}

eVector4 operator * (eF32 s, const eVector4 &v)
{
    return v*s;
}

eVector4 operator / (eF32 s, const eVector4 &v)
{
    return eVector4(s/v.x,s/v.y,s/v.z, s/v.w);
}