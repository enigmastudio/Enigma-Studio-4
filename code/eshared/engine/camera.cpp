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
#include "../math/math.hpp"
#include "engine.hpp"

eCamera::eCamera(eF32 fovY, eF32 aspect, eF32 zNear, eF32 zFar) :
    m_type(eCAM_PERSPECTIVE),
    m_fovY(fovY),
    m_aspect(aspect),
    m_zNear(zNear),
    m_zFar(zFar),
    m_left(0.0f),
    m_right(0.0f),
    m_top(0.0f),
    m_bottom(0.0f)
{
    m_projMtx.perspective(m_fovY, m_aspect, m_zNear, m_zFar);
    _extractFrustumPlanes();
}

eCamera::eCamera(eF32 left, eF32 right, eF32 top, eF32 bottom, eF32 zNear, eF32 zFar) :
    m_type(eCAM_ORTHO),
    m_left(left),
    m_right(right),
    m_top(top),
    m_bottom(bottom),
    m_zNear(zNear),
    m_zFar(zFar),
    m_fovY(0.0f),
    m_aspect(0.0f)
{
    m_projMtx.ortho(m_left, m_right, m_top, m_bottom, m_zNear, m_zFar);
}

void eCamera::activate(const eMatrix4x4 &modelMtx) const
{
    static eConstBufferDx11<ShaderConsts, eST_VS> cb;
    cb.data.viewMtx = m_viewMtx;
    cb.data.projMtx = m_projMtx;
    cb.data.mvpMtx = modelMtx*m_viewMtx*m_projMtx;
    cb.data.itViewMtx = m_itViewMtx;
    cb.data.camWorldPos = getWorldPos();
    cb.data.clearCol = m_clearCol;

    eRenderState &rs = eGfx->getRenderState();
    rs.constBufs[eCBI_CAMERA] = &cb;

    eGfx->setMatrices(modelMtx, m_viewMtx, m_projMtx);
}

eBool eCamera::intersectsSphere(const eVector3 &sphereCenter, eF32 sphereRadius) const
{
    ePROFILER_FUNC();
    eASSERT(m_type == eCAM_PERSPECTIVE);

    const eVector3 v = sphereCenter*m_viewMtx;
    
    for (eInt i=0; i<6; i++)
        if (m_frustumPlanes[i].getDistance(v) < -sphereRadius)
            return eFALSE;

    return eTRUE;
}

// checks the AABB against the frustum in world-space
eBool eCamera::intersectsAabb(const eAABB &aabb) const
{
    ePROFILER_FUNC();
    eASSERT(m_type == eCAM_PERSPECTIVE);

    for (eU32 i=0; i<6; i++)
    {
        const ePlane &p = m_frustumPlanes[i];
        const eF32 a = aabb.getCenter()*p.getNormal();
        const eF32 b = aabb.getSize()*p.getAbsNormal();

        if (a+b+p.getCoeffD() < 0.0f)
            return eFALSE;
    }

    return eTRUE;
}

void eCamera::setClearColor(const eColor &clearCol)
{
    m_clearCol = clearCol;
}

void eCamera::setViewMatrix(const eMatrix4x4 &viewMtx)
{
    m_viewMtx = viewMtx;
    m_invViewMtx = viewMtx.inverse();
    m_itViewMtx = m_invViewMtx;
    m_itViewMtx.transpose();

    _extractFrustumPlanes();
}

const eColor & eCamera::getClearColor() const
{
    return m_clearCol;
}

eVector3 eCamera::getWorldPos() const
{
    return m_invViewMtx.getTranslation();
}

const eMatrix4x4 & eCamera::getViewMatrix() const
{
    return m_viewMtx;
}

const eMatrix4x4 & eCamera::getInvViewMatrix() const
{
    return m_invViewMtx;
}

const eMatrix4x4 & eCamera::getProjMatrix() const
{
    return m_projMtx;
}

eF32 eCamera::getAspectRatio() const
{
    return m_aspect;
}

eF32 eCamera::getFieldOfViewY() const
{
    return m_fovY;
}

void eCamera::_extractFrustumPlanes()
{
    eASSERT(m_type == eCAM_PERSPECTIVE);

    // frustum corners after perspective projection
    eVector3 corners[8] =
    {
        eVector3(-1.0f, -1.0f,  0.0f), 
        eVector3( 1.0f, -1.0f,  0.0f), 
        eVector3(-1.0f,  1.0f,  0.0f), 
        eVector3( 1.0f,  1.0f,  0.0f), 
        eVector3(-1.0f, -1.0f,  1.0f), 
        eVector3( 1.0f, -1.0f,  1.0f), 
        eVector3(-1.0f,  1.0f,  1.0f), 
        eVector3( 1.0f,  1.0f,  1.0f), 
    };

    // get corners before perspective projection by
    // multiplying with inverse projection matrix
    const eMatrix4x4 invViewProjMtx = (m_viewMtx*m_projMtx).inverse();

    for (eInt i=0; i<8; i++)
    {
        eVector4 v(corners[i], 1.0f);
        v *= invViewProjMtx;
        v /= v.w;
        corners[i].set(v.x, v.y, v.z);
    }

    // setup frustum planes
    m_frustumPlanes[0] = ePlane(corners[0], corners[1], corners[2]);
    m_frustumPlanes[1] = ePlane(corners[6], corners[7], corners[5]);
    m_frustumPlanes[2] = ePlane(corners[2], corners[6], corners[4]);
    m_frustumPlanes[3] = ePlane(corners[7], corners[3], corners[5]);
    m_frustumPlanes[4] = ePlane(corners[2], corners[3], corners[6]);
    m_frustumPlanes[5] = ePlane(corners[1], corners[0], corners[4]);
}