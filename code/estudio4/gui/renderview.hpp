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

#ifndef RENDER_VIEW_HPP
#define RENDER_VIEW_HPP

#include <QtWidgets/QProgressBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>

#include "../../eshared/eshared.hpp"

// viewport for rendering. all kinds of
// operators are displayed in this widget.
class eRenderView : public QWidget
{
    Q_OBJECT

public:
    enum FxOpCameraMode
    {
        FXCAM_FIXED,
        FXCAM_FREE,
        FXCAM_LINK,
    };

private:
    enum CameraType
    {
        CT_FIRST_PERSON,
        CT_ORIGIN_CENTER,
    };

public:
    eRenderView(QWidget *parent);
    virtual ~eRenderView();

    void                    setViewOp(eIOperator *viewOp);
    void                    setEditOp(eIOperator *editOp);
    void                    setTime(eF32 time);

    eIOperator *            getViewOp() const;
    eIOperator *            getEditOp() const;
    eBool                   getWireframe() const;
    const eVector3 &        getCameraPos() const;
    eF32                    getZoomFactor() const;
    eF32                    getLastCalcMs() const;
    eF32                    getLastFrameMs() const;
    eF32                    getAvgFrameMs() const;
    FxOpCameraMode          getFxOpCameraMode() const;

public:
    virtual QSize           sizeHint() const;
    virtual QPaintEngine *  paintEngine() const;

Q_SIGNALS:
    void                    onToggleFullscreen();

private:
    void                    _renderNone();
    void                    _renderOperator();
    void                    _renderBitmapOp(eIBitmapOp *op);
    void                    _renderMeshOp(eIMeshOp *op);
    void                    _renderModelOp(eIModelOp *op);
    void                    _renderEffectOp(eIEffectOp *op);
    void                    _renderSequencerOp(eISequencerOp *op);
    void                    _renderDemoOp(eIDemoOp *op);
    void                    _renderMaterialOp(eIMaterialOp *op);
    void                    _renderRenderToTextureOp(eIRenderToTextureOp *op);

    void                    _createActions();
    void                    _resetMeshMode();
    void                    _resetModelMode();
    void                    _resetBitmapMode();
    void                    _initMaterialMode();
    void                    _initProgressBar();

    void                    _setupTarget();
    void                    _copyToScreen();
    void                    _renderScene(const eScene &scene, const eCamera &cam, eF32 time);

    eUserInput              _getUserInput() const;
    eCamera                 _createCamera() const;
    eLight                  _createDefaultLight(const eCamera &cam);
    void                    _resetCamera();
    void                    _doCameraMovement();
    void                    _doCameraRotation(const QPoint &move, CameraType camMode);
    eBool                   _isKeyDown(Qt::Key key) const;
    void                    _saveImageAs(eU32 width, eU32 height, const eArray<eColor> &data, eInt quality);
    void                    _copy1stPovCameraToLocal();
    eIOpPtrArray            _getOpsOfTypeInStack(eIOperator *op, eU32 opType) const;

    void                    _createGridMesh(const eVector3 &xBase, const eVector3 &zBase, eU32 segmentCount, eU32 majorSegCount, eF32 spacing);
    void                    _updateNormalsMesh(const eEditMesh &em, eF32 length);
    void                    _updateWireframeMesh(eEditMesh &em);
    void                    _updateBoundingBoxMesh(const eAABB &bbox);
    void                    _updateKdMesh(const eSceneData &sd);

    void                    _updateProgress(eU32 percent);
    static eBool            _progressCallback(eU32 processed, eU32 total, ePtr param);

private Q_SLOTS:
    void                    _onBmpTiling();
    void                    _onTextureZoomIn();
    void                    _onTextureZoomOut();
    void                    _onBmpVisualizeAlpha();
    void                    _onResetViewport();
    void                    _onToggleShowGrid();
    void                    _onToggleShowNormals();
    void                    _onToggleBoundingBoxes();
    void                    _onToggleWireframe();
    void                    _onWidgetScale();
    void                    _onWidgetRotate();
    void                    _onWidgetTranslate();
    void                    _onFxCameraFixed();
    void                    _onFxCameraFree();
    void                    _onFxCameraLink();
    void                    _onSaveScreenShot();
    void                    _onSaveBitmapAs();

private:
    virtual void            contextMenuEvent(QContextMenuEvent *ce);
    virtual void            mousePressEvent(QMouseEvent *me);
    virtual void            mouseReleaseEvent(QMouseEvent *me);
    virtual void            mouseMoveEvent(QMouseEvent *me);
    virtual void            keyReleaseEvent(QKeyEvent *ke);
    virtual void            keyPressEvent(QKeyEvent *ke);
    virtual void            wheelEvent(QWheelEvent *we);
    virtual void            resizeEvent(QResizeEvent *re);
    virtual void            timerEvent(QTimerEvent *te);
    virtual void            paintEvent(QPaintEvent *pe);

private:
    static const eF32       LENGTH_NORMALS;
    static const eF32       CAM_SPEED_ZOOM;
    static const eF32       CAM_SPEED_ROTATE;
    static const eF32       CAM_SPEED_MOVE_SLOW;
    static const eF32       CAM_SPEED_MOVE_FAST;
    static const eColor     COL_PRIMS_SEL;
    static const eColor     COL_PRIMS_NOTSEL;
    static const eColor     COL_PRIMS_WIRE;
    static const eColor     COL_AABB_LEAF;
    static const eColor     COL_AABB_KDTREE;
    static const eColor     COL_NORMALS_FACE;
    static const eColor     COL_NORMALS_VTX;

    static const eU32       PB_WAIT_TIL_SHOW = 250;
    static const eInt       SCREENSHOT_QUALITY = 90; // between 0 (low) and 100 (high)

private:
    eEngine                 m_engine;
    eSynth                  m_synth;
    eTexture2d *            m_target;
    QLabel                  m_lblStats;
    eTimer                  m_frameTimer;

    eIOperator *            m_viewOp; // viewed operator (double click / press 's')
    eIOperator *            m_editOp; // edited operator (selected one)
    eInt                    m_timerId;
    eF32                    m_lastCalcMs;
    eF32                    m_lastFrameMs;
    eBool                   m_opChanged;
    QPoint                  m_lastMousePos;
    QPoint                  m_mouseDownPos;
    eF32                    m_time;

    // bitmap and R2T mode variables
    eTexture2d *            m_texAlpha;
    eBool                   m_tiling;
    eBool                   m_visualAlpha;
    eF32                    m_zoomFac;
    ePoint                  m_bmpOffset;

    // mesh and model mode variables
    eVector3                m_camUpVec;
    eVector3                m_camEyePos;
    eVector3                m_camLookAt;
    eBool                   m_showNormals;
    eBool                   m_wireframe;
    eMesh                   m_solidMesh;
    eMesh                   m_bboxMesh;
    eMesh                   m_normalsMesh;
    eMesh                   m_wireMesh;
    eMesh                   m_selFacesMesh;
    eMaterial               m_matWirePrims;
    eBool                   m_camLocked;
    eBool                   m_showGrid;
    eBool                   m_showBBoxes;
    eMesh                   m_gridMesh;
    eMesh                   m_kdMesh;
    eOpInteractionInfos     m_oii;

    // material mode variables
    eEditMesh               m_matCube;
    eMesh                   m_matMesh;

    // effect mode variables
    FxOpCameraMode          m_fxCamMode;

    // stuff used by GUI
    QList<eInt>             m_keysDown;
    QMenu                   m_menu;
    QMenu                   m_defMenu;
    QMenu                   m_bmpMenu;
    QMenu                   m_r2tMenu;
    QMenu                   m_meshMenu;
    QMenu                   m_modelMenu;
    QMenu                   m_fxMenu;
    QAction *               m_actShowGrid;
    QAction *               m_actWireframe;
    QAction *               m_actShowNormals;
    QAction *               m_actShowBBoxes;
    QAction *               m_actShowTiled;
    QAction *               m_actVisualAlpha;
    QAction *               m_actScale;
    QAction *               m_actRotate;
    QAction *               m_actTrans;
    QAction *               m_actFixedCam;
    QAction *               m_actFreeCam;
    QAction *               m_actLinkCam;

    // progress bar variables
    QProgressBar            m_progressBar;
    eU32                    m_progressStartTime;
};

#endif // RENDER_VIEW_HPP