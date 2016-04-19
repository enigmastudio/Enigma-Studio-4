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

#ifndef TRANSF_WIDGETS_HPP
#define TRANSF_WIDGETS_HPP

enum eTwType
{
  eTWT_TRANS,
  eTWT_ROT,
  eTWT_SCALE,
  eTWT_MOVEAXIS
};

enum eTwHandle
{
    eTWH_NONE,
    eTWH_PLANE_XY,
    eTWH_PLANE_XZ,
    eTWH_PLANE_YZ,
    eTWH_AXIS_X,
    eTWH_AXIS_Y,
    eTWH_AXIS_Z,
    eTWH_AXIS_FREE,
    eTWH_COUNT
};

enum eTwResult
{
  eTWR_NOCHANGES,
  eTWR_CHANGED
};

class eITransformWidget
{
public:
    eITransformWidget();

    eTwResult           interact(const eUserInput &input, const eCamera &cam, eSceneData &sd);
    void                setPosition(const eVector3 &pos);
    void                setTransform(const eVector3 &transf);

    const eVector3 &    getPosition() const;
    const eVector3 &    getTransform() const;
    eTwHandle           getHandle() const;

protected:
    virtual eTwResult   _intersects(const eUserInput &input, const eCamera &cam) = 0;
  
    eRay                _calcRay(const eUserInput &input, const eCamera &cam, eBool transformed) const;
    void                _addAxis(eMesh &mesh, const eVector3 &origin, const eVector3 &axis, const eVector3 &vtx, const eVector3 &vty, eF32 size, const eColor &col, const eMaterial *mat) const;
    void                _addQuad(eMesh &mesh, const eVector3 &origin, const eVector3 &axis0, const eVector3 &axis1, eF32 size, const eColor &col, const eColor &colSel, const eMaterial *mat) const;
    void                _addCircle(eMesh &mesh, const eVector3 &axis0, const eVector3 &axis1, const eVector3 &center, eF32 radius, const eColor &col, const eMaterial *mat) const;
    void                _addTriangle(eMesh &mesh, const eVector3 &pos0, const eVector3 &pos1, const eVector3 &pos2, const eColor &col, const eMaterial *mat) const;

protected:
    static const eF32   PROJ_SCALE;
    static const eF32   AXIS_LEN;
    static const eF32   PLANE_SIZE;
    static const eF32   CIRCLE_RADIUS;
    static const eF32   SELECT_TRESH;

protected:
    eMeshInst *         m_activeMi;
    eMaterial           m_mat;
    eVector3            m_lockVtx;
    eVector3            m_transf;
    eTwHandle           m_handle;
    eVector3            m_pos;
    eF32                m_projFac;
    eMeshInst           m_infoMi;
    eMesh               m_infoMesh;
};

class eTranslateWidget : public eITransformWidget
{
public:
    eTranslateWidget();

private:
    virtual eTwResult   _intersects(const eUserInput &input, const eCamera &cam);
    void                _createWidget(eMesh &mesh, eBool xyPlaneSel, eBool xzPlaneSel, eBool yzPlaneSel, eBool xAxisSel, eBool yAxisSel, eBool zAxisSel);

private:
    eMesh               m_widgetNone;
    eMesh               m_widgetXy;
    eMesh               m_widgetYz;
    eMesh               m_widgetXz;
    eMesh               m_widgetX;
    eMesh               m_widgetY;
    eMesh               m_widgetZ;
    eMeshInst           m_widgetNoneMi;
    eMeshInst           m_widgetXyMi;
    eMeshInst           m_widgetYzMi;
    eMeshInst           m_widgetXzMi;
    eMeshInst           m_widgetXMi;
    eMeshInst           m_widgetYMi;
    eMeshInst           m_widgetZMi;
};

// returns rotation not in radiants or degree,
// but in "number of full rotations" (radiants/(2*PI))
class eRotateWidget : public eITransformWidget
{
public:
    eRotateWidget();

private:
    virtual eTwResult   _intersects(const eUserInput &input, const eCamera &cam);
    eBool               _intersectsCircle(const eRay &ray, const ePlane &plane, eVector3 &ip) const;
    void                _createWidget(eMesh &mesh, eTwHandle handle, const eColor &colXy, const eColor &colXz, const eColor &colYz) const;

private:
    eMesh               m_widgetNone;
    eMesh               m_widgetXy;
    eMesh               m_widgetYz;
    eMesh               m_widgetXz;
    eMeshInst           m_widgetNoneMi;
    eMeshInst           m_widgetXyMi;
    eMeshInst           m_widgetYzMi;
    eMeshInst           m_widgetXzMi;
};

class eScaleWidget : public eITransformWidget
{
public:
    eScaleWidget();

private:
    virtual eTwResult   _intersects(const eUserInput &input, const eCamera &cam);
    void                _createWidget(eMesh &mesh, eBool xyPlaneSel, eBool xzPlaneSel, eBool yzPlaneSel, eBool xAxisSel, eBool yAxisSel, eBool zAxisSel);

private:
    eMesh               m_widgetNone;
    eMesh               m_widgetXy;
    eMesh               m_widgetYz;
    eMesh               m_widgetXz;
    eMesh               m_widgetX;
    eMesh               m_widgetY;
    eMesh               m_widgetZ;
    eMeshInst           m_widgetNoneMi;
    eMeshInst           m_widgetXyMi;
    eMeshInst           m_widgetYzMi;
    eMeshInst           m_widgetXzMi;
    eMeshInst           m_widgetXMi;
    eMeshInst           m_widgetYMi;
    eMeshInst           m_widgetZMi;
};

// widget controller for scale, rotate and
// translate transformation widgets
class eSrtWidgetController
{
public:
    eSrtWidgetController();

    eTwResult           interact(const eUserInput &input, const eCamera &cam, eSceneData &sd, eVector3 &scale, eVector3 &rot, eVector3 &transl);
    void                setPosition(const eVector3 &pos);
    void                setActiveWidget(eTwType twType);
    void                setVisible(eBool visible);

private:
    eBool               m_visible;
    eITransformWidget * m_activeTw;
    eScaleWidget        m_scaleWidget;
    eRotateWidget       m_rotWidget;
    eTranslateWidget    m_transWidget;
};

#endif // TRANSF_WIDGETS_HPP