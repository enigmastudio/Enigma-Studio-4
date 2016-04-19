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

#include <QtGui/QResizeEvent>
#include <QtGui/QImageWriter>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QApplication>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

#include "renderview.hpp"

const eF32   eRenderView::LENGTH_NORMALS      = 0.1f;
const eF32   eRenderView::CAM_SPEED_ZOOM      = 1.0f;
const eF32   eRenderView::CAM_SPEED_ROTATE    = 1.0f;
const eF32   eRenderView::CAM_SPEED_MOVE_SLOW = 1.0f;
const eF32   eRenderView::CAM_SPEED_MOVE_FAST = 10.0f*eRenderView::CAM_SPEED_MOVE_SLOW;
const eColor eRenderView::COL_PRIMS_SEL       = eCOL_ORANGE;
const eColor eRenderView::COL_PRIMS_NOTSEL    = eCOL_GRAY;
const eColor eRenderView::COL_PRIMS_WIRE      = eCOL_GRAY;
const eColor eRenderView::COL_AABB_LEAF       = eCOL_YELLOW;
const eColor eRenderView::COL_AABB_KDTREE     = eCOL_PURPLE;
const eColor eRenderView::COL_NORMALS_FACE    = eCOL_RED;
const eColor eRenderView::COL_NORMALS_VTX     = eCOL_GREEN;

eRenderView::eRenderView(QWidget *parent) : QWidget(parent),
    m_viewOp(nullptr),
    m_editOp(nullptr),
    m_lastCalcMs(0.0f),
    m_lastFrameMs(0.0f),
    m_opChanged(eFALSE),
    m_time(0.0f),
    m_target(nullptr),
    m_engine(eWF_VSYNC, eSize(300, 200), (ePtr)winId()),
    m_synth(44100),
    m_camLocked(eFALSE),
    m_fxCamMode(FXCAM_FIXED),
    m_actShowGrid(nullptr),
    m_actWireframe(nullptr),
    m_actShowNormals(nullptr),
    m_actShowBBoxes(nullptr),
    m_actShowTiled(nullptr),
    m_actVisualAlpha(nullptr),
    m_actScale(nullptr),
    m_actRotate(nullptr),
    m_actTrans(nullptr),
    m_lblStats((QWidget *)this),
    m_progressBar(this),
    m_progressStartTime(0)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NativeWindow);

    _createActions();
    _resetMeshMode();
    _resetBitmapMode();
    _initMaterialMode();
    _initProgressBar();

    _createGridMesh(eVEC3_XAXIS, eVEC3_ZAXIS, 10, 4, 3.0f);
    m_texAlpha = eGfx->createChessTexture(64, 64, 32, eCOL_BLACK, eCOL_WHITE);
    m_matWirePrims = *eMaterial::getWireframe();
    m_matWirePrims.depthFunc = eDF_NEVER;

    m_timerId = startTimer(0);
}

eRenderView::~eRenderView()
{
    killTimer(m_timerId);

    eGfx->removeTexture2d(m_target);
    eGfx->removeTexture2d(m_texAlpha);
    
    // has to be shutdown before profiler
    // (because of thread)
    m_synth.shutdown();

    eOpStacking::shutdown();
    eProfiler::shutdown();
}

void eRenderView::setViewOp(eIOperator *viewOp)
{
    if (m_viewOp == viewOp)
        return;

    m_viewOp = viewOp;
    m_opChanged = eTRUE;

    // clear menu and actions
    for (eInt i=actions().size()-1; i>=0; i--)
        removeAction(actions().at(i));

    m_menu.clear();

    // use default menu if operator does not exists
    if (!viewOp)
    {
        addActions(m_defMenu.actions());
        m_menu.addActions(m_defMenu.actions());
        return;
    }

    // use menu for operator category
    const eOpClass opc = viewOp->getResultClass();

    switch (opc)
    {
    case eOC_BMP:
        addActions(m_bmpMenu.actions());
        m_menu.addActions(m_bmpMenu.actions());
        break;
    case eOC_MESH:
        addActions(m_meshMenu.actions());
        m_menu.addActions(m_meshMenu.actions());
        break;
    case eOC_MODEL:
        addActions(m_modelMenu.actions());
        m_menu.addActions(m_modelMenu.actions());
        break;
    case eOC_FX:
        addActions(m_fxMenu.actions());
        m_menu.addActions(m_fxMenu.actions());
        break;
    default:
        addActions(m_defMenu.actions());
        m_menu.addActions(m_defMenu.actions());
        break;
    }
}

void eRenderView::setEditOp(eIOperator *editOp)
{
    m_editOp = (editOp ? editOp : m_viewOp);
}

void eRenderView::setTime(eF32 time)
{
    eASSERT(time >= 0.0f);
    m_time = time;
}

eIOperator * eRenderView::getViewOp() const
{
    return m_viewOp;
}

eIOperator * eRenderView::getEditOp() const
{
    return m_editOp;
}

eBool eRenderView::getWireframe() const
{
    return m_wireframe;
}

const eVector3 & eRenderView::getCameraPos() const
{
    return m_camEyePos;
}

eF32 eRenderView::getZoomFactor() const
{
    return m_zoomFac;
}

eF32 eRenderView::getLastCalcMs() const
{
    return m_lastCalcMs;
}

eF32 eRenderView::getLastFrameMs() const
{
    return m_lastFrameMs;
}

eF32 eRenderView::getAvgFrameMs() const
{
    return m_frameTimer.getAvgElapsedMs();
}

eRenderView::FxOpCameraMode eRenderView::getFxOpCameraMode() const
{
    return m_fxCamMode;
}

QSize eRenderView::sizeHint() const
{
    return QSize(120, 500);
}

// needed because we're painting with Direct3D 
// on a widget normally controlled by QT
QPaintEngine * eRenderView::paintEngine() const
{
    return nullptr;
}

void eRenderView::_renderOperator()
{
    // operator invalid?
    if (!m_viewOp || m_viewOp->getError() != eOE_OK)
    {
        _renderNone();
        return;
    }

    // process operator
    eTimer timer;
    if (m_viewOp->process(m_time, _progressCallback, this) == eOPR_CHANGES)
        m_opChanged  = eTRUE;

    m_lastCalcMs = timer.getElapsedMs();

    // render operator
    switch (m_viewOp->getResultClass())
    {
    case eOC_BMP:
        _renderBitmapOp((eIBitmapOp *)m_viewOp);
        break;
    case eOC_MESH:
        _renderMeshOp((eIMeshOp *)m_viewOp);
        break;
    case eOC_MODEL:
        _renderModelOp((eIModelOp *)m_viewOp);
        break;
    case eOC_SEQ:
        _renderSequencerOp((eISequencerOp *)m_viewOp);
        break;
    case eOC_FX:
        _renderEffectOp((eIEffectOp *)m_viewOp);
        break;
    case eOC_DEMO:
        _renderDemoOp((eIDemoOp *)m_viewOp);
        break;
    case eOC_MAT:
        _renderMaterialOp((eIMaterialOp *)m_viewOp);
        break;
    case eOC_R2T:
        _renderRenderToTextureOp((eIRenderToTextureOp *)m_viewOp);
        break;
    default:
        _renderNone();
        break;
    }

    m_opChanged = eFALSE;
}

void eRenderView::_setupTarget()
{
    if (!m_target || eSize(m_target->width, m_target->height) != eGfx->getWndSize())
    {
        eGfx->removeTexture2d(m_target);
        m_target = eGfx->addTexture2d(eGfx->getWndWidth(), eGfx->getWndHeight(), eTEX_TARGET, eTFO_ARGB8);
    }
}

void eRenderView::_copyToScreen()
{
    eRenderState &rs = eGfx->freshRenderState();
    rs.targets[0] = eGraphics::TARGET_SCREEN;
    eRenderer->renderQuad(eRect(0, 0, m_target->width, m_target->height), eGfx->getWndSize(), m_target);
}

void eRenderView::_renderScene(const eScene &scene, const eCamera &cam, eF32 time)
{
    eASSERT(time >= 0.0f);

    _setupTarget();
    eRenderer->renderScene(scene, cam, m_target, nullptr, time);
    _copyToScreen();
}

void eRenderView::_renderNone()
{
    const QColor &bgCol = palette().window().color();
    eGfx->clear(eCM_ALL, eColor(bgCol.red(), bgCol.green(), bgCol.blue()));
}

void eRenderView::_renderBitmapOp(eIBitmapOp *op)
{
    _renderNone(); // used to clear with default window color

    // draw checker board to visualize alpha channel
    eRenderState &rs = eGfx->freshRenderState();
    rs.texFlags[0] = eTMF_NEAREST|eTMF_WRAP;

    if (m_visualAlpha)
    {
        const eF32 tileX = (eF32)eGfx->getWndWidth()/(eF32)m_texAlpha->width;
        const eF32 tileY = (eF32)eGfx->getWndHeight()/(eF32)m_texAlpha->height;
        eRenderer->renderQuad(eRect(ePoint(0, 0), eGfx->getWndSize()), eGfx->getWndSize(), m_texAlpha, eVector2(tileX, tileY));
    }

    // draw bitmap
    const eIBitmapOp::Result &res = op->getResult();
    const ePoint upLeft(m_bmpOffset.x, eGfx->getWndHeight()-res.height-m_bmpOffset.y);
    const ePoint downRight(upLeft.x+res.width*m_zoomFac, upLeft.y+res.height*m_zoomFac);

    rs.blending = eTRUE;
    rs.blendSrc = eBM_SRCALPHA;
    rs.blendDst = eBM_INVSRCALPHA;

    if (m_tiling)
    {
        eRect r(upLeft-ePoint(res.width, res.height), downRight+ePoint(res.width, res.height));
        eRenderer->renderQuad(r, eGfx->getWndSize(), res.uav->tex, eVector2(3.0f, 3.0f));
    }
    else
        eRenderer->renderQuad(eRect(upLeft, downRight), eGfx->getWndSize(), res.uav->tex);
}

void eRenderView::_renderMeshOp(eIMeshOp *op)
{
    const eEditMesh &em = op->getResult().mesh;

    if (m_opChanged)
    {
        // store triangulated mesh for rendering
        // of selected faces
        eEditMesh emWork = em;

        _updateNormalsMesh(em, LENGTH_NORMALS);
        _updateWireframeMesh(emWork); // triangulates the work mesh
        _updateBoundingBoxMesh(emWork.getBoundingBox());

        m_solidMesh.fromEditMesh(emWork, eMT_DYNAMIC);
    }

    // render mesh and additional geometry
    eMeshInst bboxMi(m_bboxMesh);
    eMeshInst normalsMi(m_normalsMesh);
    eMeshInst solidMi(m_solidMesh);
    eMeshInst gridMi(m_gridMesh);
    eMeshInst wireMi(m_wireMesh);
    eMeshInst selFacesMi(m_selFacesMesh);

    eSceneData sd;
    sd.addRenderable(m_wireframe ? &wireMi : &solidMi);

    if (m_wireframe)
        sd.addRenderable(&selFacesMi);
    if (m_showBBoxes)
        sd.addRenderable(&bboxMi);
    if (m_showNormals)
        sd.addRenderable(&normalsMi);
    if (m_showGrid)
        sd.addRenderable(&gridMi);

    const eCamera &cam = _createCamera();
    const eLight &light = _createDefaultLight(cam);

    m_oii.infoTransf.identity();
    m_oii.infoMesh.clear();
    m_oii.cam = cam;
    m_oii.input = _getUserInput();
    m_camLocked = m_editOp->doEditorInteraction(sd, m_oii);

    sd.addRenderable(&m_oii.infoInst, m_oii.infoTransf);
    sd.addLight(&light);
    _renderScene(eScene(sd), cam, m_time);
    _doCameraMovement();
}

void eRenderView::_renderModelOp(eIModelOp *op)
{
    const eCamera &cam = _createCamera();
    const eLight &light = _createDefaultLight(cam);

    eMeshInst kdMi(m_kdMesh);
    eMeshInst wireMi(m_wireMesh);
    eMeshInst gridMi(m_gridMesh);
    eSceneData sd;

    eMaterial::forceWireframe(m_wireframe);
    sd.merge(((const eIModelOp *)op)->getResult().sceneData);

    if (m_opChanged)
        _updateKdMesh(sd);    
    if (m_showBBoxes)
        sd.addRenderable(&kdMi);
    if (m_showGrid)
        sd.addRenderable(&gridMi);
    if (!sd.getLightCount())
        sd.addLight(&light);

    m_oii.infoTransf.identity();
    m_oii.infoMesh.clear();
    m_oii.cam = cam;
    m_oii.input = _getUserInput();
    m_camLocked = m_editOp->doEditorInteraction(sd, m_oii);
    sd.addRenderable(&m_oii.infoInst, m_oii.infoTransf);

    const eScene scene(sd);
    _renderScene(scene, cam, m_time);
    eMaterial::forceWireframe(eFALSE);
    _doCameraMovement();
}

void eRenderView::_renderEffectOp(eIEffectOp *op)
{
    eArray<eIXYXY> oldAnimVals;
    eIOpPtrArray povOps;

    if (m_fxCamMode == FXCAM_LINK || m_fxCamMode == FXCAM_FREE)
    {
        povOps = _getOpsOfTypeInStack(op, eOP_TYPE("Misc", "POV"));

        for (eU32 i=0; i<povOps.size(); i++)
        {
            eIOperator *povOp = povOps[i];
            for (eU32 j=0; j<povOp->getParameterCount(); j++)
                oldAnimVals.append(povOp->getParameter(j).getAnimValue().ixyxy);

            povOp->getParameter(6).getAnimValue().flt = 0.0f; // tilt
            povOp->getParameter(8).getAnimValue().fxyz = m_camEyePos;
            povOp->getParameter(9).getAnimValue().fxyz = (m_camLookAt-m_camEyePos).normalized();
            povOp->setChanged();
        }

        m_viewOp->process(m_time, _progressCallback, this);
    }

    // render effect
    _setupTarget();
    op->getResult().fx->run(m_time, m_target, nullptr);
    _copyToScreen();
    _doCameraMovement();

    // revert changes if camera mode is "free"
    if (m_fxCamMode == FXCAM_FREE)
    {
        for (eU32 i=0, index=0; i<povOps.size(); i++)
        {
            eIOperator *povOp = povOps[i];
            for (eU32 j=0; j<povOp->getParameterCount(); j++)
                povOp->getParameter(j).getAnimValue().ixyxy = oldAnimVals[index++];
            
            povOp->setChanged();
        }

        m_viewOp->process(m_time, _progressCallback, this);
    }
}

void eRenderView::_renderSequencerOp(eISequencerOp *op)
{
    _setupTarget();
    op->getResult().seq.run(m_time, m_target, nullptr);
    _copyToScreen();
}

void eRenderView::_renderDemoOp(eIDemoOp *op)
{
    op->getResult().demo.render(m_time);
}

void eRenderView::_renderMaterialOp(eIMaterialOp *op)
{
    if (m_opChanged)
    {
        m_matCube.setMaterial(&op->getResult().mat);
        m_matMesh.fromEditMesh(m_matCube, eMT_DYNAMIC);
    }

    const eCamera &cam = _createCamera();
    const eLight &light = _createDefaultLight(cam);
    eMeshInst mi(m_matMesh);
    eSceneData sd;
    
    sd.addRenderable(&mi);
    sd.addLight(&light);
    _renderScene(eScene(sd), cam, 0.0f);
    _doCameraMovement();
}

void eRenderView::_renderRenderToTextureOp(eIRenderToTextureOp *op)
{
    _renderNone(); // used to clear with default window color

    const eIRenderToTextureOp::Result &res = op->getResult();
    const ePoint upLeft(8, eGfx->getWndHeight()-8-(eInt)(res.renderTarget->height*m_zoomFac));
    const ePoint downRight(upLeft.x+res.renderTarget->width*m_zoomFac, upLeft.y+res.renderTarget->height*m_zoomFac);

    eGfx->freshRenderState();
    eRenderer->renderQuad(eRect(upLeft, downRight), eGfx->getWndSize(), res.renderTarget);
}

void eRenderView::_updateNormalsMesh(const eEditMesh &em, eF32 length)
{
    eASSERT(length > 0.0f);

    const eU32 normalCount = em.getFaceCount()+em.getPositionCount();
    const eMaterial *mat = eMaterial::getWireframe();

    m_normalsMesh.clear();
    m_normalsMesh.reserve(normalCount, normalCount*2);

    // add face normals
    for (eU32 i=0; i<em.getFaceCount(); i++)
    {
        const eVector3 &normal = em.getFace(i).normal;
        const eVector3 &pos = em.getFaceCenter(i);
        m_normalsMesh.addLine(pos, pos+normal*length, COL_NORMALS_FACE, mat);
    }

    // add vertex normals
    for (eU32 i=0; i<em.getWedgeCount(); i++)
    {
        const eEmWedge &wedge = em.getWedge(i);
        const eEmVtxPos &vtxPos = em.getPosition(wedge.posIdx);
        const eVector3 &vtxNrm = em.getNormal(wedge.nrmIdx);
        m_normalsMesh.addLine(vtxPos.pos, vtxPos.pos+vtxNrm*length, COL_NORMALS_VTX, mat);
    }

    m_normalsMesh.finishLoading(eMT_DYNAMIC);
}

void eRenderView::_updateWireframeMesh(eEditMesh &em)
{
    m_wireMesh.clear();
    em.calcAdjacency();

    // add selected vertices
    for (eU32 i=0; i<em.getPositionCount(); i++)
    {
        const eEmVtxPos &vtxPos = em.getPosition(i);
        const eColor &col = (vtxPos.selected ? COL_PRIMS_SEL : COL_PRIMS_NOTSEL);
        m_wireMesh.addPoint(vtxPos.pos, col, &m_matWirePrims);
    }

    // add edges
    for (eU32 i=0; i<em.getEdgeCount(); i++)
    {
        const eEmEdge &edge = em.getEdge(i);
        if (edge.startPos < edge.endPos || edge.twin == -1) // only one half-edge of each edge
        {
            const eVector3 &pos0 = em.getPosition(edge.startPos).pos;
            const eVector3 &pos1 = em.getPosition(edge.endPos).pos;
            m_wireMesh.addLine(pos0, pos1, COL_PRIMS_WIRE, eMaterial::getWireframe());
        }
    }

    m_wireMesh.finishLoading(eMT_DYNAMIC);

    // add selected faces
    m_matWirePrims.pointSize = ePow(em.getBoundingBox().getSize().length(), 1.0f/3.0f)*0.01f;
    m_selFacesMesh.clear();
    em.triangulate();

    for (eU32 i=0; i<em.getFaceCount(); i++)
    {
        const eEmFace &face = em.getFace(i);
        if (face.selected)
        {
            eASSERT(face.count == 3);
            eU32 verts[3];
            
            for (eU32 j=0; j<face.count; j++)
            {
                const eVector3 &pos = em.getPosition(face.posIdx[j]).pos;
                verts[j] = m_selFacesMesh.addVertex(pos, COL_PRIMS_SEL);
            }

            m_selFacesMesh.addTriangle(verts[2], verts[1], verts[0], &m_matWirePrims);
        }
    }

    m_selFacesMesh.finishLoading(eMT_DYNAMIC);
}

// percent is in {0,...,100}
void eRenderView::_updateProgress(eU32 percent)
{
    eASSERT(percent <= 100);

    if (percent == 0)
        m_progressStartTime = eTimer::getTimeMs();

    if (eTimer::getTimeMs()-m_progressStartTime >= PB_WAIT_TIL_SHOW)
    {
        m_progressBar.show();
        m_progressBar.setValue(percent);

        // only set cursor once cause override
        // cursors are pushed to a stack
        static eBool cursorSet = eFALSE;
        if (!cursorSet)
        {
            QApplication::setOverrideCursor(Qt::WaitCursor);
            cursorSet = eTRUE;
        }

        if (percent == 100)
        {
            m_progressBar.hide();
            QApplication::restoreOverrideCursor();
            cursorSet = eFALSE;
        }
    }
}

eBool eRenderView::_progressCallback(eU32 processed, eU32 total, ePtr param)
{
    eASSERT(processed <= total);
    eASSERT(total > 0);
    ((eRenderView *)param)->_updateProgress((eU32)eLerp(0.0f, 100.0f, (eF32)processed/(eF32)total));
    return eTRUE;
}

void eRenderView::_updateKdMesh(const eSceneData &sd)
{
    m_kdMesh.clear();
    eCuller kdTree(sd);

	for (eU32 i=0; i<kdTree.m_calculatedRecords.size(); i++)
    {
        eVector3 corners[8];
		kdTree.m_calculatedRecords[i].m_aabb.getCorners(corners);
        m_kdMesh.addWireCube(corners, COL_AABB_LEAF);
    }

    m_kdMesh.finishLoading(eMT_DYNAMIC);
}

void eRenderView::_updateBoundingBoxMesh(const eAABB &bbox)
{
    eVector3 corners[8];
    bbox.getCorners(corners);

    m_bboxMesh.clear();
    m_bboxMesh.addWireCube(corners, COL_AABB_LEAF);
    m_bboxMesh.finishLoading(eMT_DYNAMIC);
}

void eRenderView::_createActions()
{
    // create default actions
    QAction *actToggleFs = m_defMenu.addAction("Toggle fullscreen", this, SIGNAL(onToggleFullscreen()));
    actToggleFs->setCheckable(true);
    actToggleFs->setShortcut(QKeySequence("tab"));
    actToggleFs->setShortcutContext(Qt::WidgetShortcut);

    // create bitmap actions
    m_bmpMenu.addAction(actToggleFs);
    
    QAction *actResetVp = m_bmpMenu.addAction("Reset viewport", this, SLOT(_onResetViewport()));
    actResetVp->setShortcut(QKeySequence("r"));
    actResetVp->setShortcutContext(Qt::WidgetShortcut);

    m_bmpMenu.addAction("Save bitmap as", this, SLOT(_onSaveBitmapAs()));
    m_bmpMenu.addSeparator();

    m_actShowTiled = m_bmpMenu.addAction("Show tiled", this, SLOT(_onBmpTiling()));
    m_actShowTiled->setShortcut(QKeySequence("t"));
    m_actShowTiled->setShortcutContext(Qt::WidgetShortcut);
    m_actShowTiled->setCheckable(true);

    m_actVisualAlpha = m_bmpMenu.addAction("Visualize alpha", this, SLOT(_onBmpVisualizeAlpha()));
    m_actVisualAlpha->setShortcut(QKeySequence("a"));
    m_actVisualAlpha->setShortcutContext(Qt::WidgetShortcut);
    m_actVisualAlpha->setCheckable(true);
    m_actVisualAlpha->setChecked(true);

    QAction *actZoomIn = m_bmpMenu.addAction("Zoom in", this, SLOT(_onTextureZoomIn()));
    actZoomIn->setShortcut(QKeySequence("+"));
    actZoomIn->setShortcutContext(Qt::WidgetShortcut);

    QAction *actZoomOut = m_bmpMenu.addAction("Zoom out", this, SLOT(_onTextureZoomOut()));
    actZoomOut->setShortcut(QKeySequence("-"));
    actZoomOut->setShortcutContext(Qt::WidgetShortcut);

    // create R2T menu
    m_r2tMenu.addAction(actToggleFs);
    m_r2tMenu.addAction(actResetVp);
    
    QAction *actScreenShot = m_r2tMenu.addAction("Save screenshot", this, SLOT(_onSaveScreenShot()));
    
    m_r2tMenu.addSeparator();
    m_r2tMenu.addAction(actZoomIn);
    m_r2tMenu.addAction(actZoomOut);

    // create mesh actions
    m_meshMenu.addAction(actToggleFs);
    m_meshMenu.addAction(actResetVp);
    m_meshMenu.addAction(actScreenShot);
    m_meshMenu.addSeparator();

    m_actShowGrid = m_meshMenu.addAction("Show grid", this, SLOT(_onToggleShowGrid()));
    m_actShowGrid->setShortcut(QKeySequence("g"));
    m_actShowGrid->setShortcutContext(Qt::WidgetShortcut);
    m_actShowGrid->setCheckable(true);
    m_actShowGrid->setChecked(true);

    m_actWireframe = m_meshMenu.addAction("Render wireframe", this, SLOT(_onToggleWireframe()));
    m_actWireframe->setShortcut(QKeySequence("f"));
    m_actWireframe->setShortcutContext(Qt::WidgetShortcut);
    m_actWireframe->setCheckable(true);

    m_actShowNormals = m_meshMenu.addAction("Show normals", this, SLOT(_onToggleShowNormals()));
    m_actShowNormals->setShortcut(QKeySequence("n"));
    m_actShowNormals->setShortcutContext(Qt::WidgetShortcut);
    m_actShowNormals->setCheckable(true);

    m_actShowBBoxes = m_meshMenu.addAction("Show bounding boxes", this, SLOT(_onToggleBoundingBoxes()));
    m_actShowBBoxes->setShortcut(QKeySequence("b"));
    m_actShowBBoxes->setShortcutContext(Qt::WidgetShortcut);
    m_actShowBBoxes->setCheckable(true);

    m_meshMenu.addSeparator();
    m_actScale = m_meshMenu.addAction("Scale widget", this, SLOT(_onWidgetScale()));
    m_actScale->setShortcut(QKeySequence("y"));
    m_actScale->setShortcutContext(Qt::WidgetShortcut);
    m_actScale->setCheckable(true);

    m_actRotate = m_meshMenu.addAction("Rotate widget", this, SLOT(_onWidgetRotate()));
    m_actRotate->setShortcut(QKeySequence("x"));
    m_actRotate->setShortcutContext(Qt::WidgetShortcut);
    m_actRotate->setCheckable(true);

    m_actTrans = m_meshMenu.addAction("Translate widget", this, SLOT(_onWidgetTranslate()));
    m_actTrans->setShortcut(QKeySequence("c"));
    m_actTrans->setShortcutContext(Qt::WidgetShortcut);
    m_actTrans->setCheckable(true);
    m_actTrans->setChecked(true);

    QActionGroup *ac = new QActionGroup(this);
    ac->addAction(m_actScale);
    ac->addAction(m_actRotate);
    ac->addAction(m_actTrans);

    // create model actions
    m_modelMenu.addAction(actToggleFs);
    m_modelMenu.addAction(actResetVp);
    m_modelMenu.addAction(actScreenShot);
    m_modelMenu.addSeparator();
    m_modelMenu.addAction(m_actShowGrid);
    m_modelMenu.addAction(m_actWireframe);
    m_modelMenu.addAction(m_actShowBBoxes);
    m_modelMenu.addSeparator();
    m_modelMenu.addAction(m_actScale);
    m_modelMenu.addAction(m_actRotate);
    m_modelMenu.addAction(m_actTrans);

    // create effect actions
    m_fxMenu.addAction(actToggleFs);
    m_fxMenu.addAction(actResetVp);
    m_fxMenu.addAction(actScreenShot);
    m_fxMenu.addSeparator();

    m_actFixedCam = m_meshMenu.addAction("Fixed camera", this, SLOT(_onFxCameraFixed()));
    m_actFixedCam->setShortcut(QKeySequence("t"));
    m_actFixedCam->setShortcutContext(Qt::WidgetShortcut);
    m_actFixedCam->setCheckable(true);
    m_actFixedCam->setChecked(true);

    m_actFreeCam = m_meshMenu.addAction("Free camera", this, SLOT(_onFxCameraFree()));
    m_actFreeCam->setShortcut(QKeySequence("z"));
    m_actFreeCam->setShortcutContext(Qt::WidgetShortcut);
    m_actFreeCam->setCheckable(true);

    m_actLinkCam = m_meshMenu.addAction("Link camera", this, SLOT(_onFxCameraLink()));
    m_actLinkCam->setShortcut(QKeySequence("u"));
    m_actLinkCam->setShortcutContext(Qt::WidgetShortcut);
    m_actLinkCam->setCheckable(true);

    m_fxMenu.addAction(m_actFixedCam);
    m_fxMenu.addAction(m_actFreeCam);
    m_fxMenu.addAction(m_actLinkCam);

    ac = new QActionGroup(this);
    ac->addAction(m_actFixedCam);
    ac->addAction(m_actFreeCam);
    ac->addAction(m_actLinkCam);

    // use default menu actions in the beginning
    addActions(m_defMenu.actions());
    m_menu.addActions(m_defMenu.actions());
}

void eRenderView::_resetMeshMode()
{
    m_showGrid = eTRUE;
    m_wireframe = eFALSE;
    m_showBBoxes = eFALSE;
    m_showNormals = eFALSE;

    m_actShowGrid->setChecked(true);
    m_actWireframe->setChecked(false);
    m_actShowBBoxes->setChecked(false);
    m_actShowNormals->setChecked(false);

    eIMeshOp *meshOp = (eIMeshOp *)m_viewOp;
    if (meshOp)
    {
        const eAABB &bbox = meshOp->getResult().mesh.getBoundingBox();
        m_camLookAt = bbox.getCenter();
        m_camEyePos = bbox.getMax()+(bbox.getMax()-m_camLookAt).normalized()*5.0f;
        m_camUpVec = eVEC3_YAXIS;
    }
    else
        _resetCamera();
}

void eRenderView::_resetModelMode()
{
    m_showGrid = eTRUE;
    m_actShowGrid->setChecked(true); // reset "show grid"

    eIModelOp *modelOp = (eIModelOp *)m_viewOp;
    if (modelOp)
    {
        const eAABB &bbox = modelOp->getResult().sceneData.getBoundingBox();
        m_camLookAt = bbox.getCenter();
        m_camEyePos = bbox.getMax()+(bbox.getMax()-m_camLookAt).normalized()*5.0f;
        m_camUpVec = eVEC3_YAXIS;
    }
    else
        _resetCamera();
}

void eRenderView::_resetBitmapMode()
{
    m_tiling = eFALSE;
    m_zoomFac = 1.0f;
    m_bmpOffset = ePoint(10, 10);
    m_visualAlpha = eTRUE;

    m_actShowTiled->setChecked(false);
    m_actVisualAlpha->setChecked(true);
}

void eRenderView::_initMaterialMode()
{
    eIMeshOp *op = (eIMeshOp *)eIOperator::newInstance(eOP_TYPE("Mesh", "Cube"));
    op->process(0.0f);
    m_matCube = op->getResult().mesh;
    m_matCube.triangulate();
    eDelete(op);
}

void eRenderView::_initProgressBar()
{
    m_progressBar.hide();
    m_progressBar.setTextVisible(false); // important as text rendering does not work on render-view
    QVBoxLayout *vbl = new QVBoxLayout(this);
    vbl->addWidget(&m_progressBar, 1);
}

eUserInput eRenderView::_getUserInput() const
{
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    const QPoint mouseDelta = m_mouseDownPos-mousePos;
    const Qt::MouseButtons mb = QApplication::mouseButtons();

    eUserInput input;
    input.mouseDelta.set(mouseDelta.x(), mouseDelta.y());
    input.mousePos.set(mousePos.x(), mousePos.y());
    input.mouseBtns = (underMouse() ? (mb&Qt::LeftButton ? eMB_LEFT : 0)|(mb&Qt::RightButton ? eMB_RIGHT : 0)|(mb&Qt::MidButton ? eMB_MIDDLE : 0) : eMB_NONE);
    return input;
}

eCamera eRenderView::_createCamera() const
{
    eCamera cam(45.0f, (eF32)size().width()/(eF32)size().height(), 0.1f, 1000.0f);
    eMatrix4x4 mtx;
    mtx.lookAt(m_camEyePos, m_camLookAt, m_camUpVec);
    cam.setViewMatrix(mtx);
    return cam;
}

eLight eRenderView::_createDefaultLight(const eCamera &cam)
{
    eLight light(eCOL_WHITE, eColor(32, 32, 32), eCOL_BLACK, 500.0f, eFALSE);
    light.setPosition(eVector3(0.0f, 0.0f, 0.0f)*cam.getViewMatrix().inverse());
    return light;
}

void eRenderView::_resetCamera()
{
    m_camUpVec  = eVEC3_YAXIS;
    m_camLookAt = eVEC3_ORIGIN;
    m_camEyePos = eVector3(10.0f, 10.0f, 10.0f);
}

void eRenderView::_doCameraMovement()
{
    // calculate adjusting values for frame
    // rate independent movement
    static eTimer timer;
    static eF32 oldTime = timer.getElapsedMs();
    const eF32 curTime = timer.getElapsedMs();
    const eF32 moveRate = (curTime-oldTime)/100.0f+eALMOST_ZERO;
    oldTime = curTime;

    // exit if no keys are pressed. above values
    // have to be updated each time because
    // old time needs to be altered.
    if (m_keysDown.isEmpty())
        return;

    const eF32 speed = (_isKeyDown(Qt::Key_Shift) ? CAM_SPEED_MOVE_FAST : CAM_SPEED_MOVE_SLOW);
    const eVector3 viewDir = (m_camLookAt-m_camEyePos).normalized()*moveRate*speed;
    const eVector3 right = (viewDir^m_camUpVec).normalized()*moveRate*speed;

    if (_isKeyDown(Qt::Key_W))
    {
        m_camEyePos += viewDir;
        m_camLookAt += viewDir;
    }
    else if (_isKeyDown(Qt::Key_S))
    {
        m_camEyePos -= viewDir;
        m_camLookAt -= viewDir;
    }

    if (_isKeyDown(Qt::Key_A))
    {
        m_camEyePos += right;
        m_camLookAt += right;
    }
    else if (_isKeyDown(Qt::Key_D))
    {
        m_camEyePos -= right;
        m_camLookAt -= right;
    }
}

void eRenderView::_doCameraRotation(const QPoint &move, CameraType camMode)
{
    const eVector2 delta(-(eF32)move.x()/180.0f*CAM_SPEED_ROTATE, (eF32)move.y()/180.0f*CAM_SPEED_ROTATE);

    const eVector3 viewDir = (m_camLookAt-m_camEyePos).normalized();
    const eVector3 right = (viewDir^m_camUpVec).normalized();

    if (camMode == CT_FIRST_PERSON)
    {
        const eMatrix4x4 rotRight(eQuat(right, -delta.y));
        const eMatrix4x4 rotY(eQuat(eVEC3_YAXIS, -delta.x));
        const eMatrix4x4 rot = rotRight*rotY;

        m_camUpVec *= rot;

        // rotate look-at vector around eye position
        m_camLookAt -= m_camEyePos;
        m_camLookAt *= rot;
        m_camLookAt += m_camEyePos;
    }
    else
    {
        eVector3 newEyePos = m_camEyePos;
        newEyePos.rotate(delta.y*right+delta.x*m_camUpVec);
        m_camLookAt.null();

        const eVector3 newViewDir = (m_camLookAt-newEyePos).normalized();

        // make sure that view direction and camera's
        // up vector stay linearly independant
        if (eAbs(newViewDir*m_camUpVec) < 0.99f)
        {
            m_camUpVec = eVEC3_YAXIS;
            m_camEyePos = newEyePos;
        }
    }
}

eBool eRenderView::_isKeyDown(Qt::Key key) const
{
    return (m_keysDown.contains(key) ? eTRUE : eFALSE);
}

void eRenderView::_saveImageAs(eU32 width, eU32 height, const eArray<eColor> &data, eInt quality)
{
    // swap R and B channel
    eArray<eColor> swapped = data;
    for (eU32 i=0; i<data.size(); i++)
        eSwap(swapped[i].r, swapped[i].b);

    // create filter of supported formats for dialog
    const QList<QByteArray> &formats = QImageWriter::supportedImageFormats();
    QString filter;

    for (eInt i=0; i<formats.size(); i++)
    {
        if (i > 0)
            filter += ";;";
        
        filter += formats[i].toUpper();
        filter += " (";
        filter += "*.";
        filter += formats[i];
        filter += ")";
    }

    eASSERT(!formats.empty());

    // ask for file name
    QString selFilter = "PNG (*.png)";
    const QString fileName = QFileDialog::getSaveFileName(this, "", "", filter, &selFilter);

    if (fileName != "")
    {
        // find correct format
        QString format;
        for (eInt i=0; i<formats.size(); i++)
            if (fileName.endsWith(formats[i]))
                format = formats[i];

        if (format == "")
        {
            const QStringList list = selFilter.split(" ");
            eASSERT(!list.empty());
            format = list[0];
        }

        // save image to file
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QImage img(width, height, QImage::Format_ARGB32);
        eMemCopy(img.bits(), &swapped[0], swapped.size()*sizeof(eColor));
        img.save(fileName, format.toLatin1(), quality);
        QApplication::restoreOverrideCursor();
    }
}

void eRenderView::_copy1stPovCameraToLocal()
{
    const eIOpPtrArray povOps = _getOpsOfTypeInStack(m_editOp, eOP_TYPE("Misc", "POV"));
    if (!povOps.isEmpty())
    {
        m_camEyePos = povOps[0]->getParameter(8).getAnimValue().fxyz;
        m_camLookAt = povOps[0]->getParameter(9).getAnimValue().fxyz;
    }
}

eIOpPtrArray eRenderView::_getOpsOfTypeInStack(eIOperator *op, eU32 opType) const
{
    eIOpPtrArray ops;
    m_editOp->getOpsInStack(ops);

    for (eInt i=(eInt)ops.size()-1; i>=0; i--)
        if (ops[i]->getMetaInfos().type != opType)
            ops.removeSwap(i);

    return ops;
}

void eRenderView::_createGridMesh(const eVector3 &xBase, const eVector3 &zBase, eU32 segmentCount, eU32 majorSegCount, eF32 spacing)
{
    const eInt segLines = segmentCount/2*majorSegCount;
    const eF32 size = (eF32)segLines*spacing;
    const eU32 lineCount = segLines*4+6;
    const eMaterial *mat = eMaterial::getWireframe();
    const eVector3 xAxis = xBase*size;
    const eVector3 zAxis = zBase*size;
    const eVector3 yAxis = (zBase^xBase)*size;

    m_gridMesh.clear();
    m_gridMesh.reserve(lineCount, lineCount*2);

    for (eInt i=-segLines; i<=segLines; i++)
    {
        const eColor &color = (i == 0 ? eCOL_WHITE : ((i%majorSegCount)%2 == 0 ? eCOL_LIGHTGRAY : eCOL_GRAY));
        m_gridMesh.addLine(-xAxis+zBase*i*spacing, xAxis+zBase*i*spacing, color, mat);
        m_gridMesh.addLine(-zAxis+xBase*i*spacing, zAxis+xBase*i*spacing, color, mat);
    }

    m_gridMesh.finishLoading(eMT_STATIC);
}

void eRenderView::_onBmpTiling()
{
    m_tiling = !m_tiling;
}

void eRenderView::_onTextureZoomIn()
{
    m_zoomFac *= 2.0f;
}

void eRenderView::_onTextureZoomOut()
{
    m_zoomFac /= 2.0f;
}

void eRenderView::_onBmpVisualizeAlpha()
{
    m_visualAlpha = !m_visualAlpha;
}

void eRenderView::_onResetViewport()
{
    if (!m_viewOp)
        return;

    switch (m_viewOp->getResultClass())
    {
    case eOC_MESH:
        _resetMeshMode();
        break;
    case eOC_MODEL:
        _resetModelMode();
        break;
    case eOC_BMP:
        _resetBitmapMode();
        break;
    }
}

void eRenderView::_onToggleShowGrid()
{
    m_showGrid = !m_showGrid;
}

void eRenderView::_onToggleShowNormals()
{
    m_showNormals = !m_showNormals;
}

void eRenderView::_onToggleBoundingBoxes()
{
    m_showBBoxes = !m_showBBoxes;
}

void eRenderView::_onToggleWireframe()
{
    m_wireframe = !m_wireframe;
}

void eRenderView::_onWidgetScale()
{
    m_oii.srtCtrl.setActiveWidget(eTWT_SCALE);
    m_actScale->setChecked(true);
}

void eRenderView::_onWidgetRotate()
{
    m_oii.srtCtrl.setActiveWidget(eTWT_ROT);
    m_actRotate->setChecked(true);
}

void eRenderView::_onWidgetTranslate()
{
    m_oii.srtCtrl.setActiveWidget(eTWT_TRANS);
    m_actTrans->setChecked(true);
}

void eRenderView::_onFxCameraFixed()
{
    m_fxCamMode = FXCAM_FIXED;
    m_actFixedCam->setChecked(true);
}

void eRenderView::_onFxCameraFree()
{
    _copy1stPovCameraToLocal();
    m_fxCamMode = FXCAM_FREE;
    m_actFreeCam->setChecked(true);
}

void eRenderView::_onFxCameraLink()
{
    _copy1stPovCameraToLocal();
    m_fxCamMode = FXCAM_LINK;
    m_actLinkCam->setChecked(true);
}

void eRenderView::_onSaveScreenShot()
{
    eASSERT(m_viewOp->getResultClass() != eOC_BMP);
    eArray<eColor> data;
    eGfx->readTexture2d(m_target, data);
    _saveImageAs(m_target->width, m_target->height, data, SCREENSHOT_QUALITY);
}

void eRenderView::_onSaveBitmapAs()
{
    eASSERT(m_viewOp->getResultClass() == eOC_BMP);
    
    const eIBitmapOp::Result &res = ((eIBitmapOp *)m_viewOp)->getResult(); 
    eArray<eColor> bmpData;
    eGfx->readTexture2d(res.uav->tex, bmpData);
    _saveImageAs(res.width, res.height, bmpData, SCREENSHOT_QUALITY);
}

void eRenderView::contextMenuEvent(QContextMenuEvent *ce)
{
    QWidget::contextMenuEvent(ce);

    // show context menu if mouse was not
    // moved too much
    if ((m_mouseDownPos-ce->pos()).manhattanLength() < 4)
        m_menu.exec(QCursor::pos());
}

void eRenderView::mousePressEvent(QMouseEvent *me)
{
    QWidget::mousePressEvent(me);

    m_lastMousePos = me->pos();
    m_mouseDownPos = me->pos();

    if (me->buttons() & Qt::RightButton)
        setContextMenuPolicy(Qt::PreventContextMenu);
}

void eRenderView::mouseReleaseEvent(QMouseEvent *me)
{
    QWidget::mouseReleaseEvent(me);

    if (me->button() == Qt::RightButton)
    {
        QContextMenuEvent ce(QContextMenuEvent::Mouse, me->pos());
        setContextMenuPolicy(Qt::DefaultContextMenu);
        contextMenuEvent(&ce);
    }
}

void eRenderView::mouseMoveEvent(QMouseEvent *me)
{
    QWidget::mouseMoveEvent(me);
    
    const eBool leftBtn = (me->buttons() & Qt::LeftButton);
    const QPoint move = m_lastMousePos-me->pos();
    m_lastMousePos = me->pos();

    if (!m_viewOp || m_camLocked)
        return;

    switch (m_viewOp->getResultClass())
    {
    case eOC_FX:
    case eOC_MODEL:
    case eOC_MESH:
    case eOC_MAT:
        _doCameraRotation(move, leftBtn ? CT_FIRST_PERSON : CT_ORIGIN_CENTER);
        break;

    case eOC_BMP:
        m_bmpOffset.x -= move.x();
        m_bmpOffset.y -= move.y();
        break;
    }
}

void eRenderView::keyReleaseEvent(QKeyEvent *ke)
{
    QWidget::keyReleaseEvent(ke);

    // remove key from held keys list
    const eInt index = m_keysDown.indexOf(ke->key());
    if (index != -1 && !ke->isAutoRepeat())
        m_keysDown.removeAt(index);
}

void eRenderView::keyPressEvent(QKeyEvent *ke)
{
    QWidget::keyPressEvent(ke);

    // add key to held keys list
    if (!m_keysDown.contains(ke->key()) && !ke->isAutoRepeat())
        m_keysDown.append(ke->key());
}

// zoom in/out using mouse-wheel
void eRenderView::wheelEvent(QWheelEvent *we)
{
    QWidget::wheelEvent(we);

    const eInt wheelDeg = we->delta()/8; // degrees wheel was turned
    const eInt wheelSteps = wheelDeg/15; // number of steps wheel was turned

    // in bitmap view:
    if (m_viewOp && m_viewOp->getResultClass() == eOC_BMP)
    {
        if (wheelDeg > 0)
            _onTextureZoomIn();
        else
            _onTextureZoomOut();
    }
    else // in other views:
    {
        const eVector3 viewDir = (m_camLookAt-m_camEyePos).normalized()*(eF32)wheelSteps*CAM_SPEED_ZOOM;
        m_camEyePos += viewDir;
        m_camLookAt += viewDir;
    }
}

void eRenderView::resizeEvent(QResizeEvent *re)
{
    QWidget::resizeEvent(re);
    eGfx->resizeBackbuffer(re->size().width(), re->size().height());
}

void eRenderView::timerEvent(QTimerEvent *te)
{
    QWidget::timerEvent(te);
    QApplication::processEvents();
    update();
}

void eRenderView::paintEvent(QPaintEvent *pe)
{
    ePROFILER_BEGIN_THREAD_FRAME();
    {
        ePROFILER_FUNC();
        m_lastFrameMs = m_frameTimer.restart();
        eGfx->beginFrame();
        _renderOperator();
        eGfx->endFrame();
    }
    ePROFILER_END_THREAD_FRAME();

    QWidget::paintEvent(pe);
}