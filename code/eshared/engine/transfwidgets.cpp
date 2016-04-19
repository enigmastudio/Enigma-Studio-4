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

const eF32 eITransformWidget::PROJ_SCALE    = 0.04f;
const eF32 eITransformWidget::AXIS_LEN      = 5.0f;
const eF32 eITransformWidget::PLANE_SIZE    = 1.0f;
const eF32 eITransformWidget::CIRCLE_RADIUS = 5.0f;
const eF32 eITransformWidget::SELECT_TRESH  = 0.1f;

eITransformWidget::eITransformWidget() :
    m_handle(eTWH_NONE),
    m_infoMi(m_infoMesh)
{
    m_mat = *eMaterial::getWireframe();
    m_mat.blendSrc = eBM_ONE;
    m_mat.blendDst = eBM_ONE;
    m_mat.blending = eFALSE;
    m_mat.depthFunc = eDF_ALWAYS;
    m_mat.depthTest = eFALSE;
    m_mat.renderPass = 255;
}

eTwResult eITransformWidget::interact(const eUserInput &input, const eCamera &cam, eSceneData &sd)
{
    // calculate scaling factor
    eMatrix4x4 viewProj = cam.getViewMatrix()*cam.getProjMatrix();
    eVector4 q = eVector4(m_pos, 1.0f)*viewProj;
    m_projFac = PROJ_SCALE*q.w;

    // perform intersection tests
    const eTwResult res = _intersects(input, cam);

    // add scene data
    const eTransform transf(eQuat(), m_pos, eVector3(m_projFac));
    sd.addRenderable(m_activeMi, transf);
    sd.addRenderable(&m_infoMi);
    return res;
}

void eITransformWidget::setPosition(const eVector3 &pos)
{
    m_pos = pos;
}

void eITransformWidget::setTransform(const eVector3 &transf)
{
    m_transf = transf;
}

const eVector3 & eITransformWidget::getPosition() const
{
    return m_pos;
}

const eVector3 & eITransformWidget::getTransform() const
{
    return m_transf;
}

eTwHandle eITransformWidget::getHandle() const
{
    return m_handle;
}

eRay eITransformWidget::_calcRay(const eUserInput &input, const eCamera &cam, eBool transformed) const
{
    eMatrix4x4 modelMtx;

    if (transformed)
        modelMtx.translate(m_pos);

    eVector3 hpos(2.0f*(eF32)input.mousePos.x/(eF32)eGfx->getWndWidth()-1.0f, 1.0f-2.0f*(eF32)input.mousePos.y/(eF32)eGfx->getWndHeight(), 1.0f);
    eVector3 vpos = hpos*cam.getProjMatrix().inverse();
    eVector3 wpos = vpos*cam.getViewMatrix().inverse();

    eVector3 rayOrg = cam.getWorldPos();
    eVector3 rayDir = wpos-rayOrg;
    return eRay(rayOrg*modelMtx.inverse(), (rayDir*modelMtx.transposed()).normalized());
}

void eITransformWidget::_addAxis(eMesh &mesh, const eVector3 &origin, const eVector3 &axis, const eVector3 &vtx, const eVector3 &vty, eF32 size, const eColor &col, const eMaterial *mat) const
{
    const eU32 vtx0 = mesh.addVertex(origin, col);
    const eU32 vtx1 = mesh.addVertex(origin+size*axis, col);
    mesh.addLine(vtx0, vtx1, mat);

    const eU32 cylTopVtx = mesh.addVertex(origin+size*axis, col);
    const eU32 segs = 5;
    eU32 cylBottomVerts[segs];
    eF32 fct = size*0.05f;
    eF32 fct2 = size;
    for (eU32 i=0; i<segs; i++)
    {
        eVector3 pt;
        pt = vtx * eCos(((eTWOPI)/(eF32)segs)*i)*fct;
        pt+= vty * eSin(((eTWOPI)/(eF32)segs)*i)*fct;
        pt+=axis*fct2*0.8f;
        pt+=origin;
        cylBottomVerts[i] = mesh.addVertex(pt, col);
    }
    for (eU32 i=0; i<segs; i++)
        mesh.addTriangle(cylTopVtx, cylBottomVerts[i], cylBottomVerts[(i+1)%segs], mat);
}

void eITransformWidget::_addQuad(eMesh &mesh, const eVector3 &origin, const eVector3 &axis0, const eVector3 &axis1, eF32 size, const eColor &col, const eColor &colSel, const eMaterial *mat) const
{
    const eU32 vtx0 = mesh.addVertex(origin, col);
    const eU32 vtx1 = mesh.addVertex(origin+size*axis0, col);
    const eU32 vtx2 = mesh.addVertex(origin+size*(axis0+axis1), col);
    const eU32 vtx3 = mesh.addVertex(origin+size*axis1, col);

    mesh.addTriangle(vtx0, vtx1, vtx2, mat);
    mesh.addTriangle(vtx0, vtx2, vtx3, mat);

    const eU32 vtx5 = mesh.addVertex(origin+size*axis0, colSel);
    const eU32 vtx6 = mesh.addVertex(origin+size*(axis0+axis1), colSel);
    const eU32 vtx7 = mesh.addVertex(origin+size*axis1, colSel);

    mesh.addLine(vtx5, vtx6, mat);
    mesh.addLine(vtx6, vtx7, mat);
}

void eITransformWidget::_addCircle(eMesh &mesh, const eVector3 &axis0, const eVector3 &axis1, const eVector3 &center, eF32 radius, const eColor &col, const eMaterial *mat) const
{
    const eU32 segs = 50;
    const eU32 oldVtxCount = mesh.getVertexCount();

    for (eU32 i=0; i<segs; i++)
    {
        eF32 s, c;
        eSinCos((eF32)i/(eF32)(segs-1)*eTWOPI, s, c);
        const eVector3 pos = (s*axis0+c*axis1)*radius;
        mesh.addVertex(pos, col);
    }

    for (eU32 i=0; i<segs; i++)
        mesh.addLine(oldVtxCount+i, oldVtxCount+((i+1)%segs), mat);
}

void eITransformWidget::_addTriangle(eMesh &mesh, const eVector3 &pos0, const eVector3 &pos1, const eVector3 &pos2, const eColor &col, const eMaterial *mat) const
{
    const eU32 idx0 = mesh.addVertex(pos0, col);
    const eU32 idx1 = mesh.addVertex(pos1, col);
    const eU32 idx2 = mesh.addVertex(pos2, col);

    mesh.addTriangle(idx0, idx1, idx2, mat);
}

eTranslateWidget::eTranslateWidget() :
    m_widgetNoneMi(m_widgetNone),
    m_widgetXyMi(m_widgetXy),
    m_widgetYzMi(m_widgetYz),
    m_widgetXzMi(m_widgetXz),
    m_widgetXMi(m_widgetX),
    m_widgetYMi(m_widgetY),
    m_widgetZMi(m_widgetZ)
{
    m_activeMi = &m_widgetNoneMi;

    _createWidget(m_widgetNone, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetXy, eTRUE, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetYz, eFALSE, eFALSE, eTRUE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetXz, eFALSE, eTRUE, eFALSE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetX, eFALSE, eFALSE, eFALSE, eTRUE, eFALSE, eFALSE);
    _createWidget(m_widgetY, eFALSE, eFALSE, eFALSE, eFALSE, eTRUE, eFALSE);
    _createWidget(m_widgetZ, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE, eTRUE);
}

eTwResult eTranslateWidget::_intersects(const eUserInput &input, const eCamera &cam)
{
    const eRay ray = _calcRay(input, cam, eTRUE);
    ePlane planeXy(eVEC3_ORIGIN, eVEC3_ZAXIS);
    ePlane planeXz(eVEC3_ORIGIN, eVEC3_YAXIS);
    ePlane planeYz(eVEC3_ORIGIN, eVEC3_XAXIS);
    eVector3 ip;

    if (!(input.mouseBtns&eMB_LEFT))
    {
        m_activeMi = &m_widgetNoneMi;
        m_handle = eTWH_NONE;
    
        if (ray.intersects(planeXy, &ip))
        {
            if (eInRange(ip.x, 0.0f, PLANE_SIZE*m_projFac) && eInRange(ip.y, 0.0f, PLANE_SIZE*m_projFac))
            {
                m_activeMi = &m_widgetXyMi;
                m_handle = eTWH_PLANE_XY;
                m_lockVtx = ip;
            }
            else if (eInRange(ip.x, -SELECT_TRESH*m_projFac, SELECT_TRESH*m_projFac) && eInRange(ip.y, 0.0f, AXIS_LEN*m_projFac))
            {
                m_activeMi = &m_widgetYMi;
                m_handle = eTWH_AXIS_Y;
                m_lockVtx = ip;
            }
            else if (eInRange(ip.y, -SELECT_TRESH*m_projFac, SELECT_TRESH*m_projFac) && eInRange(ip.x, 0.0f, AXIS_LEN*m_projFac))
            {
                m_activeMi = &m_widgetXMi;
                m_handle = eTWH_AXIS_X;
                m_lockVtx = ip;
            }
        }
        
        if (ray.intersects(planeXz, &ip))
        {
            if (eInRange(ip.x, 0.0f, PLANE_SIZE*m_projFac) && eInRange(ip.z, 0.0f, PLANE_SIZE*m_projFac))
            {
                m_activeMi = &m_widgetXzMi;
                m_handle = eTWH_PLANE_XZ;
                m_lockVtx = ip;
            }
            else if (eInRange(ip.z, 0.0f, AXIS_LEN*m_projFac) && eInRange(ip.x, -SELECT_TRESH*m_projFac, SELECT_TRESH*m_projFac))
            {
                m_activeMi = &m_widgetZMi;
                m_handle = eTWH_AXIS_Z;
                m_lockVtx = ip;
            }
        }
        
        if (ray.intersects(planeYz, &ip))
        {
            if (eInRange(ip.y, 0.0f, PLANE_SIZE*m_projFac) && eInRange(ip.z, 0.0f, PLANE_SIZE*m_projFac))
            {
                m_activeMi = &m_widgetYzMi;
                m_handle = eTWH_PLANE_YZ;
                m_lockVtx = ip;
            }
        }
    }
    else if (m_handle != eTWH_NONE)
    {
        ePlane movePlane;

        if (m_handle == eTWH_PLANE_XY || m_handle == eTWH_AXIS_X)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_ZAXIS);
        else if (m_handle == eTWH_PLANE_XZ || m_handle == eTWH_AXIS_Z)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_YAXIS);
        else if (m_handle == eTWH_PLANE_YZ || m_handle == eTWH_AXIS_Y)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_XAXIS);

        ray.intersects(movePlane, &ip);

        eVector3 np = ip-m_lockVtx;

        if (m_handle == eTWH_PLANE_XY)
            np.set(np.x, np.y, 0.0f);
        else if (m_handle == eTWH_PLANE_XZ)
            np.set(np.x, 0.0f, np.z);
        else if (m_handle == eTWH_PLANE_YZ)
            np.set(0.0f, np.y, np.z);
        else if (m_handle == eTWH_AXIS_X)
          np.set(np.x, 0.0f, 0.0f);
        else if (m_handle == eTWH_AXIS_Y)
          np.set(0.0f, np.y, 0.0f);
        else if (m_handle == eTWH_AXIS_Z)
          np.set(0.0f, 0.0f, np.z);

        m_transf += np;
        m_pos += np;
        return eTWR_CHANGED;
    }

    return eTWR_NOCHANGES;
}

void eTranslateWidget::_createWidget(eMesh &mesh, eBool xyPlaneSel, eBool xzPlaneSel, eBool yzPlaneSel, eBool xAxisSel, eBool yAxisSel, eBool zAxisSel)
{
    eColor xAxisCol[2] = {eColor(130, 0, 0), eCOL_RED};
    eColor yAxisCol[2] = {eColor(0, 130, 0), eCOL_GREEN};
    eColor zAxisCol[2] = {eColor(0, 0, 130), eCOL_BLUE};

    _addQuad(mesh, eVEC3_ORIGIN, eVEC3_XAXIS, eVEC3_YAXIS, PLANE_SIZE, zAxisCol[xyPlaneSel], eCOL_BLUE, &m_mat);
    _addQuad(mesh, eVEC3_ORIGIN, eVEC3_XAXIS, eVEC3_ZAXIS, PLANE_SIZE, yAxisCol[xzPlaneSel], eCOL_GREEN, &m_mat);
    _addQuad(mesh, eVEC3_ORIGIN, eVEC3_YAXIS, eVEC3_ZAXIS, PLANE_SIZE, xAxisCol[yzPlaneSel], eCOL_RED, &m_mat);

    _addAxis(mesh, eVEC3_ORIGIN, eVEC3_XAXIS, eVEC3_YAXIS, eVEC3_ZAXIS, AXIS_LEN, xAxisCol[xAxisSel], &m_mat);
    _addAxis(mesh, eVEC3_ORIGIN, eVEC3_YAXIS, eVEC3_XAXIS, eVEC3_ZAXIS, AXIS_LEN, yAxisCol[yAxisSel], &m_mat);
    _addAxis(mesh, eVEC3_ORIGIN, eVEC3_ZAXIS, eVEC3_XAXIS, eVEC3_YAXIS, AXIS_LEN, zAxisCol[zAxisSel], &m_mat);

    mesh.finishLoading(eMT_DYNAMIC);
}

eRotateWidget::eRotateWidget() :
    m_widgetNoneMi(m_widgetNone),
    m_widgetXyMi(m_widgetXy),
    m_widgetYzMi(m_widgetYz),
    m_widgetXzMi(m_widgetXz)
{
    m_activeMi = &m_widgetNoneMi;

    _createWidget(m_widgetNone, eTWH_NONE, eColor(0,0,130), eColor(130,0,0), eColor(0,130,0));
    _createWidget(m_widgetXy, eTWH_PLANE_XY, eCOL_BLUE,eColor(130, 0, 0), eColor(0, 130, 0));
    _createWidget(m_widgetXz, eTWH_PLANE_XZ, eColor(0,0,130),eColor(130,0,0),eCOL_GREEN);
    _createWidget(m_widgetYz, eTWH_PLANE_YZ, eColor(0,0,130),eCOL_RED,eColor(0, 130, 0));
}

   static eF32 eATan2S(eF32 y, eF32 x)
   {
       eF32 d = eATan2(y, x);
       return d+(d < 0.0f ? eTWOPI : 0.0f);
   }

eTwResult eRotateWidget::_intersects(const eUserInput &input, const eCamera &cam)
{
    ePlane planeXy(eVEC3_ORIGIN, eVEC3_ZAXIS);
    ePlane planeXz(eVEC3_ORIGIN, eVEC3_YAXIS);
    ePlane planeYz(eVEC3_ORIGIN, eVEC3_XAXIS);
    eRay ray = _calcRay(input, cam, eTRUE);

    if (!(input.mouseBtns&eMB_LEFT))
    {
        m_handle = eTWH_NONE;
        m_activeMi = &m_widgetNoneMi;

        eVector3 ip;

        if (_intersectsCircle(ray, planeXy, ip))
        {
            m_handle = eTWH_PLANE_XY;
            m_activeMi = &m_widgetXyMi;
            m_lockVtx = ip;
        }
        else if (_intersectsCircle(ray, planeXz, ip))
        {
            m_handle = eTWH_PLANE_XZ;
            m_activeMi = &m_widgetXzMi;
            m_lockVtx = ip;
        }
        else if (_intersectsCircle(ray, planeYz, ip))
        {
            m_handle = eTWH_PLANE_YZ;
            m_activeMi = &m_widgetYzMi;
            m_lockVtx = ip;
        }
    }
    else if (m_handle != eTWH_NONE)
    {
        ePlane movePlane;

        if (m_handle == eTWH_PLANE_XY || m_handle == eTWH_AXIS_X)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_ZAXIS);
        else if (m_handle == eTWH_PLANE_XZ || m_handle == eTWH_AXIS_Z)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_YAXIS);
        else if (m_handle == eTWH_PLANE_YZ || m_handle == eTWH_AXIS_Y)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_XAXIS);

        eVector3 ip;
        ray.intersects(movePlane, &ip);
        eF32 angle0, angle1;

        if (m_handle == eTWH_PLANE_XY)
        {
            angle0 = eATan2S(ip.y, ip.x);
            angle1 = eATan2S(m_lockVtx.y, m_lockVtx.x);
            m_transf.z += (angle1-angle0)/eTWOPI;
        }
        else if (m_handle == eTWH_PLANE_XZ)
        {
            angle1 = eATan2S(ip.z, ip.x);
            angle0 = eATan2S(m_lockVtx.z, m_lockVtx.x);
            m_transf.y += (angle1-angle0)/eTWOPI;
        }
        else if (m_handle == eTWH_PLANE_YZ)
        {
            angle0 = eATan2S(ip.z, ip.y);
            angle1 = eATan2S(m_lockVtx.z, m_lockVtx.y);
            m_transf.x += (angle1-angle0)/eTWOPI;
        }
        else
        {
            return eTWR_NOCHANGES;
        }

        m_lockVtx = ip;
        return eTWR_CHANGED;
    }

    return eTWR_NOCHANGES;
}

eBool eRotateWidget::_intersectsCircle(const eRay &ray, const ePlane &plane, eVector3 &ip) const
{
    if (!ray.intersects(plane, &ip))
      return eFALSE;

    const eF32 d = ip.distance(eVEC3_ORIGIN);
    return eInRange(d, CIRCLE_RADIUS*m_projFac-SELECT_TRESH, CIRCLE_RADIUS*m_projFac+SELECT_TRESH);
}

void eRotateWidget::_createWidget(eMesh &mesh, eTwHandle handle, const eColor &colXy, const eColor &colXz, const eColor &colYz) const
{
    _addCircle(mesh, eVEC3_XAXIS, eVEC3_YAXIS, eVEC3_ORIGIN, CIRCLE_RADIUS, colXy, &m_mat);
    _addCircle(mesh, eVEC3_YAXIS, eVEC3_ZAXIS, eVEC3_ORIGIN, CIRCLE_RADIUS, colXz, &m_mat);
    _addCircle(mesh, eVEC3_XAXIS, eVEC3_ZAXIS, eVEC3_ORIGIN, CIRCLE_RADIUS, colYz, &m_mat);

    mesh.finishLoading(eMT_DYNAMIC);
}

eScaleWidget::eScaleWidget() :
    m_widgetNoneMi(m_widgetNone),
    m_widgetXyMi(m_widgetXy),
    m_widgetYzMi(m_widgetYz),
    m_widgetXzMi(m_widgetXz),
    m_widgetXMi(m_widgetX),
    m_widgetYMi(m_widgetY),
    m_widgetZMi(m_widgetZ)
{
    m_activeMi = &m_widgetNoneMi;

    _createWidget(m_widgetNone, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetXy, eTRUE, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetYz, eFALSE, eFALSE, eTRUE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetXz, eFALSE, eTRUE, eFALSE, eFALSE, eFALSE, eFALSE);
    _createWidget(m_widgetX, eFALSE, eFALSE, eFALSE, eTRUE, eFALSE, eFALSE);
    _createWidget(m_widgetY, eFALSE, eFALSE, eFALSE, eFALSE, eTRUE, eFALSE);
    _createWidget(m_widgetZ, eFALSE, eFALSE, eFALSE, eFALSE, eFALSE, eTRUE);
}

eTwResult eScaleWidget::_intersects(const eUserInput &input, const eCamera &cam)
{
    const eRay ray = _calcRay(input, cam, eTRUE);
    ePlane planeXy(eVEC3_ORIGIN, eVEC3_ZAXIS);
    ePlane planeXz(eVEC3_ORIGIN, eVEC3_YAXIS);
    ePlane planeYz(eVEC3_ORIGIN, eVEC3_XAXIS);
    eVector3 ip;

    if (!(input.mouseBtns&eMB_LEFT))
    {
        m_activeMi = &m_widgetNoneMi;
        m_handle = eTWH_NONE;
    
        if (ray.intersects(planeXy, &ip))
        {
            if (eInRange(ip.x, 0.0f, PLANE_SIZE*m_projFac) && eInRange(ip.y, 0.0f, PLANE_SIZE*m_projFac))
            {
                m_activeMi = &m_widgetXyMi;
                m_handle = eTWH_PLANE_XY;
                m_lockVtx = ip;
            }
            else if (eInRange(ip.x, -SELECT_TRESH*m_projFac, SELECT_TRESH*m_projFac) && eInRange(ip.y, 0.0f, AXIS_LEN*m_projFac))
            {
                m_activeMi = &m_widgetYMi;
                m_handle = eTWH_AXIS_Y;
                m_lockVtx = ip;
            }
            else if (eInRange(ip.y, -SELECT_TRESH*m_projFac, SELECT_TRESH*m_projFac) && eInRange(ip.x, 0.0f, AXIS_LEN*m_projFac))
            {
                m_activeMi = &m_widgetXMi;
                m_handle = eTWH_AXIS_X;
                m_lockVtx = ip;
            }
        }
        
        if (ray.intersects(planeXz, &ip))
        {
            if (eInRange(ip.x, 0.0f, PLANE_SIZE*m_projFac) && eInRange(ip.z, 0.0f, PLANE_SIZE*m_projFac))
            {
                m_activeMi = &m_widgetXzMi;
                m_handle = eTWH_PLANE_XZ;
                m_lockVtx = ip;
            }
            else if (eInRange(ip.z, 0.0f, AXIS_LEN*m_projFac) && eInRange(ip.x, -SELECT_TRESH*m_projFac, SELECT_TRESH*m_projFac))
            {
                m_activeMi = &m_widgetZMi;
                m_handle = eTWH_AXIS_Z;
                m_lockVtx = ip;
            }
        }
        
        if (ray.intersects(planeYz, &ip))
        {
            if (eInRange(ip.y, 0.0f, PLANE_SIZE*m_projFac) && eInRange(ip.z, 0.0f, PLANE_SIZE*m_projFac))
            {
                m_activeMi = &m_widgetYzMi;
                m_handle = eTWH_PLANE_YZ;
                m_lockVtx = ip;
            }
        }
    }
    else if (m_handle != eTWH_NONE)
    {
        ePlane movePlane;

        if (m_handle == eTWH_PLANE_XY || m_handle == eTWH_AXIS_X)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_ZAXIS);
        else if (m_handle == eTWH_PLANE_XZ || m_handle == eTWH_AXIS_Z)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_YAXIS);
        else if (m_handle == eTWH_PLANE_YZ || m_handle == eTWH_AXIS_Y)
            movePlane = ePlane(eVEC3_ORIGIN, eVEC3_XAXIS);

        ray.intersects(movePlane, &ip);

        eVector3 np = ip-m_lockVtx;

        if (m_handle == eTWH_PLANE_XY)
            np.set(np.x, np.y, 0.0f);
        else if (m_handle == eTWH_PLANE_XZ)
            np.set(np.x, 0.0f, np.z);
        else if (m_handle == eTWH_PLANE_YZ)
            np.set(0.0f, np.y, np.z);
        else if (m_handle == eTWH_AXIS_X)
          np.set(np.x, 0.0f, 0.0f);
        else if (m_handle == eTWH_AXIS_Y)
          np.set(0.0f, np.y, 0.0f);
        else if (m_handle == eTWH_AXIS_Z)
          np.set(0.0f, 0.0f, np.z);
        
        m_transf += 2.0f*np;
        m_lockVtx = ip;
        return eTWR_CHANGED;
    }

    return eTWR_NOCHANGES;
}

void eScaleWidget::_createWidget(eMesh &mesh, eBool xyPlaneSel, eBool xzPlaneSel, eBool yzPlaneSel, eBool xAxisSel, eBool yAxisSel, eBool zAxisSel)
{
    eColor xAxisCol[2] = {eColor(130, 0, 0), eCOL_RED};
    eColor yAxisCol[2] = {eColor(0, 130, 0), eCOL_GREEN};
    eColor zAxisCol[2] = {eColor(0, 0, 130), eCOL_BLUE};

    _addTriangle(mesh, eVector3(PLANE_SIZE, 0.0f, 0.0f), eVector3(0.0f, PLANE_SIZE, 0.0f), eVEC3_ORIGIN, zAxisCol[xyPlaneSel], &m_mat);
    _addTriangle(mesh, eVector3(PLANE_SIZE, 0.0f, 0.0f), eVector3(0.0f, 0.0f, PLANE_SIZE), eVEC3_ORIGIN, yAxisCol[xzPlaneSel], &m_mat);
    _addTriangle(mesh, eVector3(0.0f, PLANE_SIZE, 0.0f), eVector3(0.0f, 0.0f, PLANE_SIZE), eVEC3_ORIGIN, xAxisCol[yzPlaneSel], &m_mat);

    _addAxis(mesh, eVEC3_ORIGIN, eVEC3_XAXIS, eVEC3_YAXIS, eVEC3_ZAXIS, AXIS_LEN, xAxisCol[xAxisSel], &m_mat);
    _addAxis(mesh, eVEC3_ORIGIN, eVEC3_YAXIS, eVEC3_XAXIS, eVEC3_ZAXIS, AXIS_LEN, yAxisCol[yAxisSel], &m_mat);
    _addAxis(mesh, eVEC3_ORIGIN, eVEC3_ZAXIS, eVEC3_XAXIS, eVEC3_YAXIS, AXIS_LEN, zAxisCol[zAxisSel], &m_mat);

    mesh.finishLoading(eMT_DYNAMIC);
}

eSrtWidgetController::eSrtWidgetController() :
    m_visible(eFALSE),
    m_activeTw(&m_transWidget)
{
}

eTwResult eSrtWidgetController::interact(const eUserInput &input, const eCamera &cam, eSceneData &sd,
                                         eVector3 &scale, eVector3 &rot, eVector3 &transl)
{
    m_scaleWidget.setTransform(scale);
    m_rotWidget.setTransform(rot);
    m_transWidget.setTransform(transl);

    const eTwResult res = m_activeTw->interact(input, cam, sd);

    scale = m_scaleWidget.getTransform();
    rot = m_rotWidget.getTransform();
    transl = m_transWidget.getTransform();

    return res;
}

void eSrtWidgetController::setPosition(const eVector3 &pos)
{
    m_scaleWidget.setPosition(pos);
    m_rotWidget.setPosition(pos);
    m_transWidget.setPosition(pos);
}

void eSrtWidgetController::setActiveWidget(eTwType twType)
{
    switch (twType)
    {
    case eTWT_TRANS:
        m_activeTw = &m_transWidget;
        break;
    case eTWT_ROT:
        m_activeTw = &m_rotWidget;
        break;
    case eTWT_SCALE:
        m_activeTw = &m_scaleWidget;
        break;
    default:
        eASSERT(eFALSE);
    }
}

void eSrtWidgetController::setVisible(eBool visible)
{
    m_visible = visible;
}