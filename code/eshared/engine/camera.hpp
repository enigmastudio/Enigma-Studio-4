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

#ifndef CAMERA_HPP
#define CAMERA_HPP

enum eCameraType
{
    eCAM_ORTHO,
    eCAM_PERSPECTIVE
};

class eCamera
{
public:
    eCamera(eF32 fovY=45.0f, eF32 aspect=4.0f/3.0f, eF32 zNear=0.1f, eF32 zFar=1000.0f);
    eCamera(eF32 left, eF32 right, eF32 top, eF32 bottom, eF32 zNear, eF32 zFar);

    void                activate(const eMatrix4x4 &modelMtx=eMatrix4x4()) const;
    eBool               intersectsSphere(const eVector3 &sphereCenter, eF32 sphereRadius) const;
    eBool               intersectsAabb(const eAABB &aabb) const;

    void                setClearColor(const eColor &backgCol);
    void                setViewMatrix(const eMatrix4x4 &viewMtx);

    const eColor &      getClearColor() const;
    eVector3            getWorldPos() const;
    const eMatrix4x4 &  getViewMatrix() const;
    const eMatrix4x4 &  getInvViewMatrix() const;
    const eMatrix4x4 &  getProjMatrix() const;
    eF32                getAspectRatio() const;
	eF32                getFieldOfViewY() const;

private:
    void                _extractFrustumPlanes();

private:
    struct ShaderConsts
    {
        eMatrix4x4      viewMtx;
        eMatrix4x4      projMtx;
        eMatrix4x4      mvpMtx;
        eMatrix4x4      itViewMtx;
        eVector3        camWorldPos;
        eVector4        clearCol;
    };

private:
    eCameraType         m_type;
    eColor              m_clearCol;
    eF32                m_zNear;
    eF32                m_zFar;
    eMatrix4x4          m_viewMtx;
    eMatrix4x4          m_invViewMtx;
    eMatrix4x4          m_itViewMtx;

    // ortho mode parameters
    eF32                m_left;
    eF32                m_right;
    eF32                m_top;
    eF32                m_bottom;

    // perspective mode parameters
    eF32                m_fovY;
    eF32                m_aspect;
    eMatrix4x4          m_projMtx;
    ePlane              m_frustumPlanes[6];
};

#endif // CAMERA_HPP