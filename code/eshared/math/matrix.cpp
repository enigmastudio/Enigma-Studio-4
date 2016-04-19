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

eMatrix3x3::eMatrix3x3()
{
    identity();
}

eMatrix3x3::eMatrix3x3(eF32 n11, eF32 n12, eF32 n13, eF32 n21, eF32 n22, eF32 n23, eF32 n31, eF32 n32, eF32 n33) :
    m11(n11), m12(n12), m13(n13),
    m21(n21), m22(n22), m23(n23),
    m31(n31), m32(n32), m33(n33)
{
}

eMatrix3x3::eMatrix3x3(const eQuat &q)
{
    fromQuat(q);
}

void eMatrix3x3::null()
{
    eMemSet(m, 0, sizeof(m));
}

eBool eMatrix3x3::invert()
{
    const eF32 d = det();
    if (eIsFloatZero(d))
        return eFALSE;

    const eF32 invDet = 1.0f/d;
    *this = eMatrix3x3((m22*m33-m23*m32)*invDet,
                       (m13*m32-m12*m33)*invDet,
                       (m12*m23-m13*m22)*invDet,
                       (m23*m31-m21*m33)*invDet,
                       (m11*m33-m13*m31)*invDet,
                       (m13*m21-m11*m23)*invDet,
                       (m21*m32-m22*m31)*invDet,
                       (m12*m31-m11*m32)*invDet,
                       (m11*m22-m12*m21)*invDet);
    return eTRUE;
}

void eMatrix3x3::identity()
{
    null();
    m11 = 1.0f;
    m22 = 1.0f;
    m33 = 1.0f;
}

void eMatrix3x3::transpose()
{
    eSwap(m12, m21);
    eSwap(m13, m31);
    eSwap(m23, m32);
}

void eMatrix3x3::rotate(const eVector3 &rot)
{
    eF32 sx, sy, sz, cx, cy, cz;
    eSinCos(rot.x, sx, cx);
    eSinCos(rot.y, sy, cy);
    eSinCos(rot.z, sz, cz);

    *this *= eMatrix3x3( cy*cz, -cx*sz+sx*sy*cz,  sx*sz+cx*sy*cz,
                         cy*cz,  cx*cz+sx*sy*sz, -sx*cz+cz*sy*sz,
                        -sy,     sx*cy,           cz*cy);
}

void eMatrix3x3::scale(const eVector3 &scale)
{
    m11 *= scale.x;
    m21 *= scale.x;
    m31 *= scale.x;
    m12 *= scale.y;
    m22 *= scale.y;
    m32 *= scale.y;
    m13 *= scale.z;
    m23 *= scale.z;
    m33 *= scale.z;
}

// quaternion has to be normalized
void eMatrix3x3::fromQuat(const eQuat &q)
{
    eF32 tmp0, tmp1, sqw, sqx, sqy, sqz;
    sqw = q.w*q.w;
    sqx = q.x*q.x;
    sqy = q.y*q.y;
    sqz = q.z*q.z;
    m11 =  sqx-sqy-sqz+sqw; 
    m22 = -sqx+sqy-sqz+sqw;
    m33 = -sqx-sqy+sqz+sqw;
    tmp0 = q.x*q.y;
    tmp1 = q.z*q.w;
    m21 = 2.0f*(tmp0+tmp1);
    m12 = 2.0f*(tmp0-tmp1);
    tmp0 = q.x*q.z;
    tmp1 = q.y*q.w;
    m31 = 2.0f*(tmp0-tmp1);
    m13 = 2.0f*(tmp0+tmp1);
    tmp0 = q.y*q.z;
    tmp1 = q.x*q.w;
    m32 = 2.0f*(tmp0+tmp1);
    m23 = 2.0f*(tmp0-tmp1);
}

eMatrix3x3 eMatrix3x3::inverse() const
{
    eMatrix3x3 m = *this;
    m.invert();
    return m;
}

eMatrix3x3 eMatrix3x3::transposed() const
{
    eMatrix3x3 m = *this;
    m.transpose();
    return m;
}

eF32 eMatrix3x3::trace() const
{
    return m11+m22+m33;
}

eF32 eMatrix3x3::det() const
{
    return m11*m22*m33+m12*m23*m31+m13*m21*m32-m11*m23*m32-m12*m21*m33-m13*m22*m31;
}

eVector3 eMatrix3x3::getColumn(eU32 col) const
{
    eASSERT(col <= 2);
    return eVector3(m[col], m[col+3], m[col+6]);
}

const eVector3 & eMatrix3x3::getRow(eU32 row) const
{
    eASSERT(row <= 2);
    return (eVector3 &)m[row*3];
}

eMatrix3x3 eMatrix3x3::operator + (const eMatrix3x3 &m) const
{
    return eMatrix3x3(m11+m.m11, m12+m.m12, m13+m.m13,
                      m21+m.m21, m22+m.m22, m23+m.m23,
                      m31+m.m31, m32+m.m32, m33+m.m33);
}

eMatrix3x3 eMatrix3x3::operator - (const eMatrix3x3 &m) const
{
    return eMatrix3x3(m11-m.m11, m12-m.m12, m13-m.m13,
                      m21-m.m21, m22-m.m22, m23-m.m23,
                      m31-m.m31, m32-m.m32, m33-m.m33);
}

eMatrix3x3 eMatrix3x3::operator * (const eMatrix3x3 &m) const
{
    return eMatrix3x3(m11*m.m11+m12*m.m21+m13*m.m31,
                      m11*m.m12+m12*m.m22+m13*m.m32,
                      m11*m.m13+m12*m.m23+m13*m.m33,
                      m21*m.m11+m22*m.m21+m23*m.m31,
                      m21*m.m12+m22*m.m22+m23*m.m32,
                      m21*m.m13+m22*m.m23+m23*m.m33,
                      m31*m.m11+m32*m.m21+m33*m.m31,
                      m31*m.m12+m32*m.m22+m33*m.m32,
                      m31*m.m13+m32*m.m23+m33*m.m33);
}

eVector3 eMatrix3x3::operator * (const eVector3 &v) const
{
    return eVector3(m11*v.x+m12*v.y+m13*v.z,
                    m21*v.x+m22*v.y+m23*v.z,
                    m31*v.x+m32*v.y+m33*v.z);
}

eMatrix3x3 eMatrix3x3::operator * (eF32 s) const
{
    return eMatrix3x3(m11*s, m12*s, m13*s,
                      m21*s, m22*s, m23*s,
                      m31*s, m32*s, m33*s);
}

eMatrix3x3 eMatrix3x3::operator / (eF32 s) const
{
    eASSERT(!eIsFloatZero(s));
    return *this*(1.0f/s);
}

eMatrix3x3 & eMatrix3x3::operator += (const eMatrix3x3 &m)
{
    *this = *this+m;
    return *this;
}

eMatrix3x3 & eMatrix3x3::operator -= (const eMatrix3x3 &m)
{
    *this = *this-m;
    return *this;
}

eMatrix3x3 & eMatrix3x3::operator *= (const eMatrix3x3 &m)
{
    *this = *this*m;
    return *this;
}

eMatrix3x3 & eMatrix3x3::operator *= (eF32 s)
{
    *this = *this*s;
    return *this;
}

eMatrix3x3 & eMatrix3x3::operator /= (eF32 s)
{
    *this = *this/s;
    return *this;
}

eMatrix3x3 eMatrix3x3::operator - () const
{
    return eMatrix3x3(-m11, -m12, -m13,
                      -m21, -m22, -m23,
                      -m31, -m32, -m33);
}

eF32 & eMatrix3x3::operator [] (eInt index)
{
    eASSERT(index >= 0 && index < 3*3);
    return m[index];
}

eF32 eMatrix3x3::operator [] (eInt index) const
{
    eASSERT(index >= 0 && index < 3*3);
    return m[index];
}

const eF32 & eMatrix3x3::operator () (eU32 row, eU32 col) const
{
    eASSERT(row < 3 && col < 3);
    return mm[row][col];
}

eF32 & eMatrix3x3::operator () (eU32 row, eU32 col)
{
    eASSERT(row < 3 && col < 3);
    return mm[row][col];
}

eMatrix3x3::operator const eF32 * () const
{
    return (eF32 *)this;
}

eMatrix3x3 operator * (eF32 s, const eMatrix3x3 &m)
{
    return m*s;
}

eMatrix4x4::eMatrix4x4()
{
    identity();
}

eMatrix4x4::eMatrix4x4(eF32 n11, eF32 n12, eF32 n13, eF32 n14,
                       eF32 n21, eF32 n22, eF32 n23, eF32 n24,
                       eF32 n31, eF32 n32, eF32 n33, eF32 n34,
                       eF32 n41, eF32 n42, eF32 n43, eF32 n44) :
    m11(n11), m12(n12), m13(n13), m14(n14),
    m21(n21), m22(n22), m23(n23), m24(n24),
    m31(n31), m32(n32), m33(n33), m34(n34),
    m41(n41), m42(n42), m43(n43), m44(n44)
{
}

eMatrix4x4::eMatrix4x4(const eQuat &q)
{
    fromQuat(q);
}

void eMatrix4x4::null()
{
    eMemSet(m, 0, sizeof(m));
}

eBool eMatrix4x4::invert()
{
    ePROFILER_FUNC();

    const eF32 d = det();
    if (eIsFloatZero(d))
        return eFALSE;

    const eF32 invDet = 1.0f/d;
    *this = eMatrix4x4(invDet*(m22*(m33*m44-m34*m43)+m23*(m34*m42-m32*m44)+m24*(m32*m43-m33*m42)),
                       invDet*(m32*(m13*m44-m14*m43)+m33*(m14*m42-m12*m44)+m34*(m12*m43-m13*m42)),
                       invDet*(m42*(m13*m24-m14*m23)+m43*(m14*m22-m12*m24)+m44*(m12*m23-m13*m22)),
                       invDet*(m12*(m24*m33-m23*m34)+m13*(m22*m34-m24*m32)+m14*(m23*m32-m22*m33)),
                       invDet*(m23*(m31*m44-m34*m41)+m24*(m33*m41-m31*m43)+m21*(m34*m43-m33*m44)),
                       invDet*(m33*(m11*m44-m14*m41)+m34*(m13*m41-m11*m43)+m31*(m14*m43-m13*m44)),
                       invDet*(m43*(m11*m24-m14*m21)+m44*(m13*m21-m11*m23)+m41*(m14*m23-m13*m24)),
                       invDet*(m13*(m24*m31-m21*m34)+m14*(m21*m33-m23*m31)+m11*(m23*m34-m24*m33)),
                       invDet*(m24*(m31*m42-m32*m41)+m21*(m32*m44-m34*m42)+m22*(m34*m41-m31*m44)),
                       invDet*(m34*(m11*m42-m12*m41)+m31*(m12*m44-m14*m42)+m32*(m14*m41-m11*m44)),
                       invDet*(m44*(m11*m22-m12*m21)+m41*(m12*m24-m14*m22)+m42*(m14*m21-m11*m24)),
                       invDet*(m14*(m22*m31-m21*m32)+m11*(m24*m32-m22*m34)+m12*(m21*m34-m24*m31)),
                       invDet*(m21*(m33*m42-m32*m43)+m22*(m31*m43-m33*m41)+m23*(m32*m41-m31*m42)),
                       invDet*(m31*(m13*m42-m12*m43)+m32*(m11*m43-m13*m41)+m33*(m12*m41-m11*m42)),
                       invDet*(m41*(m13*m22-m12*m23)+m42*(m11*m23-m13*m21)+m43*(m12*m21-m11*m22)),
                       invDet*(m11*(m22*m33-m23*m32)+m12*(m23*m31-m21*m33)+m13*(m21*m32-m22*m31)));
    return eTRUE;
}

void eMatrix4x4::identity()
{
    null();
    m11 = 1.0f;
    m22 = 1.0f;
    m33 = 1.0f;
    m44 = 1.0f;
}

void eMatrix4x4::transpose()
{
    eSwap(m12, m21);
    eSwap(m13, m31);
    eSwap(m14, m41);
    eSwap(m23, m32);
    eSwap(m24, m42);
    eSwap(m34, m43);
}

// rotation order is x, y, z.
// angles are expected to be in radians
void eMatrix4x4::rotate(const eVector3 &rot)
{
    eF32 sx, sy, sz, cx, cy, cz;
    eSinCos(rot.x, sx, cx);
    eSinCos(rot.y, sy, cy);
    eSinCos(rot.z, sz, cz);

    *this *= eMatrix4x4( cy*cz, -cx*sz+sx*sy*cz,  sx*sz+cx*sy*cz, 0.0f,
                         cy*cz,  cx*cz+sx*sy*sz, -sx*cz+cz*sy*sz, 0.0f,
                        -sy,     sx*cy,           cz*cy,          0.0f,
                         0.0f,   0.0f,            0.0f,           1.0f);
}

void eMatrix4x4::scale(const eVector3 &scale)
{
    m11 *= scale.x;
    m21 *= scale.x;
    m31 *= scale.x;
    m41 *= scale.x;
    m12 *= scale.y;
    m22 *= scale.y;
    m32 *= scale.y;
    m42 *= scale.y;
    m13 *= scale.z;
    m23 *= scale.z;
    m33 *= scale.z;
    m43 *= scale.z;
}

void eMatrix4x4::translate(const eVector3 &trans)
{
    m11 += m14*trans.x;
    m21 += m24*trans.x;
    m31 += m34*trans.x;
    m41 += m44*trans.x;
    m12 += m14*trans.y;
    m22 += m24*trans.y;
    m32 += m34*trans.y;
    m42 += m44*trans.y;
    m13 += m14*trans.z;
    m23 += m24*trans.z;
    m33 += m34*trans.z;
    m43 += m44*trans.z;
}

// quaternion has to be normalized
void eMatrix4x4::fromQuat(const eQuat &q)
{
    const eMatrix3x3 mtx(q);

    m11 = mtx.m11;
    m12 = mtx.m12;
    m13 = mtx.m13;
    m14 = 0.0f;
    m21 = mtx.m21;
    m22 = mtx.m22;
    m23 = mtx.m23;
    m24 = 0.0f;
    m31 = mtx.m31;
    m32 = mtx.m32;
    m33 = mtx.m33;
    m34 = 0.0f;
    m41 = 0.0f;
    m42 = 0.0f;
    m43 = 0.0f;
    m44 = 1.0f;
}

// field-of-view Y is expected to be in degrees
void eMatrix4x4::perspective(eF32 fovY, eF32 aspect, eF32 zNear, eF32 zFar)
{
    eASSERT(fovY > 0.0f);
    eASSERT(aspect > 0.0f);

    const eF32 yScale = eCot(eDegToRad(fovY)*0.5f);
    const eF32 xScale = yScale/aspect;

    null();
    m11 = xScale;
    m22 = yScale;
    m33 = zFar/(zFar-zNear);
    m43 = -zNear*zFar/(zFar-zNear);
    m34 = 1.0f;
    m44 = 0.0f;
}

void eMatrix4x4::ortho(eF32 left, eF32 right, eF32 top, eF32 bottom, eF32 zNear, eF32 zFar)
{
    identity();
    
    m11 =          2.0f/(right-left);
    m41 =  (left+right)/(left-right);
    m22 =          2.0f/(bottom-top);
    m42 = -(top+bottom)/(bottom-top);
    m33 =          1.0f/(zFar-zNear);
    m43 =         zNear/(zNear-zFar);
}

// face is in [0,..,5], with
// 0: positive x
// 1: negative x
// 2: positive y
// 3: negative y
// 4: positive z
// 5: negative z.
void eMatrix4x4::cubemap(eU32 face)
{
    eASSERT(face < 6);
    null();

    switch(face)
    {
    case 0: // positive x
        m31 = -1.0f;
        m22 =  1.0f;
        m13 =  1.0f;
        m44 =  1.0f;
        break;

    case 1: // negative x
        m31 =  1.0f;
        m22 =  1.0f;
        m13 = -1.0f;
        m44 =  1.0f;
        break;

    case 2: // positive y
        m11 =  1.0f;
        m32 = -1.0f;
        m23 =  1.0f;
        m44 =  1.0f;
        break;

    case 3: // negative y
        m11 =  1.0f;
        m32 =  1.0f;
        m23 = -1.0f;
        m44 =  1.0f;
        break;

    case 4: // positive z
        m11 =  1.0f;
        m22 =  1.0f;
        m33 =  1.0f;
        m44 =  1.0f;
        break;

    case 5: // negative z
        m11 = -1.0f;
        m22 =  1.0f;
        m33 = -1.0f;
        m44 =  1.0f;
        break;
    }
}

void eMatrix4x4::lookAt(const eVector3 &pos, const eVector3 &lookAt, const eVector3 &up)
{
    const eVector3 zAxis = (lookAt-pos).normalized();
    const eVector3 xAxis = (up^zAxis).normalized();
    const eVector3 yAxis = zAxis^xAxis;

    m11 = xAxis.x;
    m12 = yAxis.x;
    m13 = zAxis.x;
    m14 = 0.0f;
    m21 = xAxis.y;
    m22 = yAxis.y;
    m23 = zAxis.y;
    m24 = 0.0f;
    m31 = xAxis.z;
    m32 = yAxis.z;
    m33 = zAxis.z;
    m34 = 0.0f;
    m41 = -xAxis*pos;
    m42 = -yAxis*pos;
    m43 = -zAxis*pos;
    m44 = 1.0f;
}

eMatrix4x4 eMatrix4x4::inverse() const
{
    eMatrix4x4 m = *this;
    m.invert();
    return m;
}

eMatrix4x4 eMatrix4x4::transposed() const
{
    eMatrix4x4 m = *this;
    m.transpose();
    return m;
}

eF32 eMatrix4x4::trace() const
{
    return m11+m22+m33+m44;
}

eF32 eMatrix4x4::det() const
{
    return +(m11*m22-m12*m21)*(m33*m44-m34*m43)
           -(m11*m23-m13*m21)*(m32*m44-m34*m42)
           +(m11*m24-m14*m21)*(m32*m43-m33*m42)
           +(m12*m23-m13*m22)*(m31*m44-m34*m41)
           -(m12*m24-m14*m22)*(m31*m43-m33*m41)
           +(m13*m24-m14*m23)*(m31*m42-m32*m41);
}

eVector4 eMatrix4x4::getColumn(eU32 col) const
{
    eASSERT(col <= 3);
    return eVector4(m[col], m[col+4], m[col+8], m[col+12]);
}

const eVector4 & eMatrix4x4::getRow(eU32 row) const
{
    eASSERT(row <= 3);
    return (eVector4 &)m[row*4];
}

const eVector3 & eMatrix4x4::getTranslation() const
{
    return (eVector3 &)m41; // last row of matrix
}

eMatrix3x3 eMatrix4x4::getUpper3x3() const
{
    return eMatrix3x3(m11, m12, m13,
                      m21, m22, m23,
                      m31, m32, m33);
}

eMatrix4x4 eMatrix4x4::operator + (const eMatrix4x4 &m) const
{
    return eMatrix4x4(m11+m.m11, m12+m.m12, m13+m.m13, m14+m.m14,
                      m21+m.m21, m22+m.m22, m23+m.m23, m24+m.m24,
                      m31+m.m31, m32+m.m32, m33+m.m33, m34+m.m34,
                      m41+m.m41, m42+m.m42, m43+m.m43, m44+m.m44);
}

eMatrix4x4 eMatrix4x4::operator - (const eMatrix4x4 &m) const
{
    return eMatrix4x4(m11-m.m11, m12-m.m12, m13-m.m13, m14-m.m14,
                      m21-m.m21, m22-m.m22, m23-m.m23, m24-m.m24,
                      m31-m.m31, m32-m.m32, m33-m.m33, m34-m.m34,
                      m41-m.m41, m42-m.m42, m43-m.m43, m44-m.m44);
}

eMatrix4x4 eMatrix4x4::operator * (const eMatrix4x4 &mtx) const
{
    eMatrix4x4 result(*this);
    result *= mtx;
    return result;
}

eVector4 eMatrix4x4::operator * (const eVector4 &v) const
{
    return eVector4(m11*v.x+m12*v.y+m13*v.z+m14*v.w,
                    m21*v.x+m22*v.y+m23*v.z+m24*v.w,
                    m31*v.x+m32*v.y+m33*v.z+m34*v.w,
                    m41*v.x+m42*v.y+m43*v.z+m44*v.w);
}

eVector3 eMatrix4x4::operator * (const eVector3 &v) const
{
    return eVector3(m11*v.x+m12*v.y+m13*v.z+m14,
                    m21*v.x+m22*v.y+m23*v.z+m24,
                    m31*v.x+m32*v.y+m33*v.z+m34);
}

eMatrix4x4 eMatrix4x4::operator * (eF32 s) const
{
    return eMatrix4x4(m11*s, m12*s, m13*s, m14*s,
                      m21*s, m22*s, m23*s, m24*s,
                      m31*s, m32*s, m33*s, m34*s,
                      m41*s, m42*s, m43*s, m44*s);
}

eMatrix4x4 eMatrix4x4::operator / (eF32 s) const
{
    eASSERT(!eIsFloatZero(s));
    return *this*(1.0f/s);
}

eMatrix4x4 & eMatrix4x4::operator += (const eMatrix4x4 &m)
{
    *this = *this+m;
    return *this;
}

eMatrix4x4 & eMatrix4x4::operator -= (const eMatrix4x4 &m)
{
    *this = *this-m;
    return *this;
}

eMatrix4x4 & eMatrix4x4::operator *= (const eMatrix4x4 &m)
{
    ePROFILER_FUNC();

    *this = eMatrix4x4(m11*m.m11+m12*m.m21+m13*m.m31+m14*m.m41,
                       m11*m.m12+m12*m.m22+m13*m.m32+m14*m.m42,
                       m11*m.m13+m12*m.m23+m13*m.m33+m14*m.m43,
                       m11*m.m14+m12*m.m24+m13*m.m34+m14*m.m44,
                       m21*m.m11+m22*m.m21+m23*m.m31+m24*m.m41,
                       m21*m.m12+m22*m.m22+m23*m.m32+m24*m.m42,
                       m21*m.m13+m22*m.m23+m23*m.m33+m24*m.m43,
                       m21*m.m14+m22*m.m24+m23*m.m34+m24*m.m44,
                       m31*m.m11+m32*m.m21+m33*m.m31+m34*m.m41,
                       m31*m.m12+m32*m.m22+m33*m.m32+m34*m.m42,
                       m31*m.m13+m32*m.m23+m33*m.m33+m34*m.m43,
                       m31*m.m14+m32*m.m24+m33*m.m34+m34*m.m44,
                       m41*m.m11+m42*m.m21+m43*m.m31+m44*m.m41,
                       m41*m.m12+m42*m.m22+m43*m.m32+m44*m.m42,
                       m41*m.m13+m42*m.m23+m43*m.m33+m44*m.m43,
                       m41*m.m14+m42*m.m24+m43*m.m34+m44*m.m44);
    return *this;
}

eMatrix4x4 & eMatrix4x4::operator *= (eF32 s)
{
    *this = *this*s;
    return *this;
}

eMatrix4x4 & eMatrix4x4::operator /= (eF32 s)
{
    *this = *this/s;
    return *this;
}

eMatrix4x4 eMatrix4x4::operator - () const
{
    return eMatrix4x4(-m11, -m12, -m13, -m14,
                        -m21, -m22, -m23, -m24,
                        -m31, -m32, -m33, -m34,
                        -m41, -m42, -m43, -m44);
}

eF32 & eMatrix4x4::operator [] (eInt index)
{
    eASSERT(index >= 0 && index < 4*4);
    return m[index];
}

eF32 eMatrix4x4::operator [] (eInt index) const
{
    eASSERT(index >= 0 && index < 4*4);
    return m[index];
}

const eF32 & eMatrix4x4::operator () (eU32 row, eU32 col) const
{
    eASSERT(row < 4 && col < 4);
    return mm[row][col];
}

eF32 & eMatrix4x4::operator () (eU32 row, eU32 col)
{
    eASSERT(row < 4 && col < 4);
    return mm[row][col];
}

eMatrix4x4::operator const eF32 * () const
{
    return (eF32 *)this;
}

eMatrix4x4 operator * (eF32 s, const eMatrix4x4 &m)
{
    return m*s;
}