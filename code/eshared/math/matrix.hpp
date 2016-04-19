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

#ifndef MATRIX_HPP
#define MATRIX_HPP

class eQuat;
class eVector3;
class eVector4;

class eMatrix3x3
{
public:
    eMatrix3x3();
    eMatrix3x3(eF32 n11, eF32 n12, eF32 n13, eF32 n21, eF32 n22, eF32 n23, eF32 n31, eF32 n32, eF32 n33);
    eMatrix3x3(const eQuat &q);

    void                null();
    eBool               invert();
    void                identity();
    void                transpose();
    void                rotate(const eVector3 &rot);
    void                scale(const eVector3 &scale);
    void                fromQuat(const eQuat &q);
    eMatrix3x3          inverse() const;
    eMatrix3x3          transposed() const;
    eF32                trace() const;
    eF32                det() const;
    eVector3            getColumn(eU32 col) const;
    const eVector3 &    getRow(eU32 row) const;

    eMatrix3x3          operator + (const eMatrix3x3 &m) const;
    eMatrix3x3          operator - (const eMatrix3x3 &m) const;
    eMatrix3x3          operator * (const eMatrix3x3 &m) const;
    eVector3            operator * (const eVector3 &v) const;
    eMatrix3x3          operator * (eF32 s) const;
    eMatrix3x3          operator / (eF32 s) const;
    eMatrix3x3 &        operator += (const eMatrix3x3 &m);
    eMatrix3x3 &        operator -= (const eMatrix3x3 &m);
    eMatrix3x3 &        operator *= (const eMatrix3x3 &m);
    eMatrix3x3 &        operator *= (eF32 s);
    eMatrix3x3 &        operator /= (eF32 s);
    eMatrix3x3          operator - () const;
    eF32 &              operator [] (eInt index);
    eF32                operator [] (eInt index) const;
    const eF32 &        operator () (eU32 row, eU32 col) const;
    eF32 &              operator () (eU32 row, eU32 col);
    friend eMatrix3x3   operator * (eF32 s, const eMatrix3x3 &m);
    operator            const eF32 * () const;

public:
    union
    {
        struct
        {
            eF32        m11, m12, m13;
            eF32        m21, m22, m23;
            eF32        m31, m32, m33;
        };

        eF32            m[3*3];
        eF32            mm[3][3];
    };
};

class eMatrix4x4
{
public:
    eMatrix4x4();
    eMatrix4x4(eF32 n11, eF32 n12, eF32 n13, eF32 n14,
               eF32 n21, eF32 n22, eF32 n23, eF32 n24,
               eF32 n31, eF32 n32, eF32 n33, eF32 n34,
               eF32 n41, eF32 n42, eF32 n43, eF32 n44);
    explicit eMatrix4x4(const eQuat &q);

    void                null();
    eBool               invert();
    void                identity();
    void                transpose();
    void                rotate(const eVector3 &rot);
    void                scale(const eVector3 &scale);
    void                translate(const eVector3 &trans);
    void                fromQuat(const eQuat &q);
    void                perspective(eF32 fovY, eF32 aspect, eF32 zNear, eF32 zFar);
    void                ortho(eF32 left, eF32 right, eF32 top, eF32 bottom, eF32 zNear, eF32 zFar);
    void                cubemap(eU32 face);
    void                lookAt(const eVector3 &pos, const eVector3 &lookAt, const eVector3 &up);
    eMatrix4x4          inverse() const;
    eMatrix4x4          transposed() const;
    eF32                trace() const;
    eF32                det() const;
    eVector4            getColumn(eU32 col) const;
    const eVector4 &    getRow(eU32 row) const;
    const eVector3 &    getTranslation() const;
    eMatrix3x3          getUpper3x3() const;

    eMatrix4x4          operator + (const eMatrix4x4 &m) const;
    eMatrix4x4          operator - (const eMatrix4x4 &m) const;
    eMatrix4x4          operator * (const eMatrix4x4 &m) const;
    eVector4            operator * (const eVector4 &v) const;
    eVector3            operator * (const eVector3 &v) const;
    eMatrix4x4          operator * (eF32 s) const;
    eMatrix4x4          operator / (eF32 s) const;
    eMatrix4x4 &        operator += (const eMatrix4x4 &m);
    eMatrix4x4 &        operator -= (const eMatrix4x4 &m);
    eMatrix4x4 &        operator *= (const eMatrix4x4 &m);
    eMatrix4x4 &        operator *= (eF32 s);
    eMatrix4x4 &        operator /= (eF32 s);
    eMatrix4x4          operator - () const;
    eF32 &              operator [] (eInt index);
    eF32                operator [] (eInt index) const;
    const eF32 &        operator () (eU32 row, eU32 col) const;
    eF32 &              operator () (eU32 row, eU32 col);

    operator            const eF32 * () const;
    friend eMatrix4x4   operator * (eF32 s, const eMatrix4x4 &m);

public:
    union
    {
        struct
        {
            eF32        m11, m12, m13, m14;
            eF32        m21, m22, m23, m24;
            eF32        m31, m32, m33, m34;
            eF32        m41, m42, m43, m44;
        };

        eF32            m[4*4];
        eF32            mm[4][4];
    };
};

#endif // MATRIX_HPP