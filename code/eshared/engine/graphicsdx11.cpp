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

#define NOMINMAX
#include <d3d11.h>
#include <d3dcompiler.h>

#include "../eshared.hpp"

static void eCallDx(const HRESULT res)
{
    eASSERT(!FAILED(res));

    if (FAILED(res))
        eShowError(eIntToStr(res));
}

eTexture2d * eGraphicsDx11::TARGET_SCREEN = (eTexture2d *)0xdeadbeef;

static const struct TextureFormat
{
    DXGI_FORMAT d3dFormat;
    eU32        pixelSize;
}
TEXTURE_FORMAT_INFOS[] =
{
    {DXGI_FORMAT_R8G8B8A8_UNORM,      4},
    {DXGI_FORMAT_R16G16B16A16_UNORM,  8},
    {DXGI_FORMAT_R16G16B16A16_FLOAT,  8},
    {DXGI_FORMAT_R32_TYPELESS,        4},
    {DXGI_FORMAT_R16_FLOAT,           2},
    {DXGI_FORMAT_R32_FLOAT,           4},
    {DXGI_FORMAT_R16G16_FLOAT,        4},
    {DXGI_FORMAT_R32G32_FLOAT,        8},
    {DXGI_FORMAT_R8_UNORM,            1}
};

static const struct InputLayoutInfo
{
    D3D11_INPUT_ELEMENT_DESC    desc[16];
    eU32                        elements;
    eChar *                     dummyShader;
}
INPUT_LAYOUT_INFOS[] =
{
    {   // default vertex
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // transformation matrix
            { "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,    1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // normal matrix
            { "TEXCOORD", 6, DXGI_FORMAT_R32G32B32_FLOAT,    1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 7, DXGI_FORMAT_R32G32B32_FLOAT,    1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        },
        11,
        "void main(in float4 _01: POSITION0,"\
        "          in float3 _02: NORMAL0,"  \
        "          in float2 _03: TEXCOORD0,"\
        "          in float4 _04: COLOR0,"   \
        "          in float4 _05: TEXCOORD1,"\
        "          in float4 _06: TEXCOORD2,"\
        "          in float4 _07: TEXCOORD3,"\
        "          in float4 _08: TEXCOORD4,"\
        "          in float3 _09: TEXCOORD5,"\
        "          in float3 _10: TEXCOORD6,"\
        "          in float3 _11: TEXCOORD7,"\
        "         out float4 pos: SV_POSITION) { pos = 0; }",
    },
    {   // simple vertex
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },

            { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // transformation matrix
            { "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,    1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // normal matrix
            { "TEXCOORD", 6, DXGI_FORMAT_R32G32B32_FLOAT,    1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 7, DXGI_FORMAT_R32G32B32_FLOAT,    1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        },
        9,
        "void main(in float4 _01: POSITION0,"\
        "          in float2 _02: TEXCOORD0,"\
        "          in float4 _03: TEXCOORD1,"\
        "          in float4 _04: TEXCOORD2,"\
        "          in float4 _05: TEXCOORD3,"\
        "          in float4 _06: TEXCOORD4,"\
        "          in float3 _07: TEXCOORD5,"\
        "          in float3 _08: TEXCOORD6,"\
        "          in float3 _09: TEXCOORD7,"\
        "         out float4 pos: SV_POSITION) { pos = 0; }",
    },
    {   // particle vertex
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        },
        3,
        "void main(in float4 _1: POSITION0, in float2 _2: TEXCOORD0, in float4 _3: COLOR0, out float4 pos: SV_POSITION) { pos = 0; }",
    }
};

eREGISTER_ENGINE_SHADER_WITHSUFFIX(globals, i);
//eREGISTER_ENGINE_SHADER_WITHSUFFIX(utils, i);

#ifndef eDEBUG
class eShaderIncludeHandler : public ID3DInclude
{
public:
    STDMETHOD(Open)(D3D_INCLUDE_TYPE includeType, const eChar *fileName, eConstPtr parentData, eConstPtr *data, eU32 *bytes)
    {
#ifdef ePLAYER

        if (eStrCompare(fileName, "globals.hlsli") == 0) {
            *data = eEngine::demodata_shader_indices[eSHADER_IDX_globals];
            *bytes = ((eU32&)eEngine::demodata_shader_indices[eSHADER_IDX_globals + 1] - (eU32&)eEngine::demodata_shader_indices[eSHADER_IDX_globals]) - 1;
            return S_OK;
        }
#else
        if (eStrCompare(fileName, "globals.hlsli") == 0) {
            *data = globals_hlsli;
            *bytes = eStrLength(globals_hlsli);
            return S_OK;
		}
#endif
        return S_FALSE;
    }

    STDMETHOD(Close)(LPCVOID pData) 
    {
        return S_OK;
    }
};

static eShaderIncludeHandler m_ShaderIncludeHandler;
#endif

eGraphicsDx11::eGraphicsDx11() :
    m_dxgiFactory(nullptr),
    m_adapter(nullptr),
    m_adapterOutput(nullptr),
    m_swapChain(nullptr),
    m_dev(nullptr),
    m_devCtx(nullptr),
    m_rtvScreen(nullptr),
    m_dsScreen(nullptr),
    m_dsvScreen(nullptr),
    m_dsvActive(nullptr),
    m_hwnd(nullptr),
    m_ownWindow(eFALSE),
    m_fullScreen(eFALSE),
    m_vsync(eTRUE),
    m_wndWidth(800),
    m_wndHeight(600),
    m_startPull(0),
    m_frameNum(0)
{
	/*
g_numAddsTex2d=0;
g_numAddsTex3d=0;
g_numAddsTexCube=0;
g_numRemsTex2d=0;
g_numRemsTex3d=0;
g_numRemsTexCube=0;
g_numAddsStaticGeo=0;
g_numRemsStaticGeo=0;
*/

    freshRenderState();
    m_rsActive = m_rsEdit;
    eMemSet(m_rtvsActive, 0, sizeof(m_rtvsActive));

#ifdef eEDITOR
    eMemSet(&m_renderStats, 0, sizeof(m_renderStats));
    eMemSet(&m_engineStats, 0, sizeof(m_engineStats));
#endif
}

eGraphicsDx11::~eGraphicsDx11()
{
    shutdown();
}

void eGraphicsDx11::initialize()
{
    eCallDx(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (ePtr *)&m_dxgiFactory));
    
    if (!FAILED(m_dxgiFactory->EnumAdapters1(0, &m_adapter)))
    {
        eCallDx(m_adapter->EnumOutputs(0, &m_adapterOutput));

        eU32 numModes = 0;
        if (!FAILED(m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr)))
        {
            eArray<DXGI_MODE_DESC> displayModes(numModes);
            eCallDx(m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, &displayModes[0]));

            for (eU32 i=0; i<numModes; i++)
            {
                DXGI_MODE_DESC &mode = displayModes[i];
                const eSize res(mode.Width, mode.Height);

                // only list resolutions > 800x600 and there are
                // many resolutions with same width/height but
                // different HZ values => add just one auf them
//                if (res.width >= 800 && res.height >= 600)
                    m_resolutions.appendUnique(res);
            }

            return;
        }
    }

    const eSize res(640, 480);
    m_resolutions.append(res);
}

void eGraphicsDx11::shutdown()
{
    // the following resources should be released
    // by application, not graphics interface
    eASSERT(m_texs2d.isEmpty());
    eASSERT(m_texs3d.isEmpty());
    eASSERT(m_texsCube.isEmpty());
    eASSERT(m_uavBufs.isEmpty());
    eASSERT(m_geos.isEmpty());

    for (eInt i=(eInt)m_shaders.size()-1; i>=0; i--)
        removeShader(m_shaders[i]);
    for (eU32 i=0; i<m_inputLayouts.size(); i++)
        eReleaseCom(m_inputLayouts[i]);
    for (eU32 i=0; i<m_depthStates.size(); i++)
        eReleaseCom(m_depthStates[i].d3dDss);
    for (eU32 i=0; i<m_rasterStates.size(); i++)
        eReleaseCom(m_rasterStates[i].d3dRs);
    for (eU32 i=0; i<m_samplerStates.size(); i++)
        eReleaseCom(m_samplerStates[i].d3dSs);
    for (eU32 i=0; i<m_blendStates.size(); i++)
        eReleaseCom(m_blendStates[i].d3dBs);

    for (eU32 i=0; i<m_geoBufs.size(); i++)
    {
        eReleaseCom(m_geoBufs[i]->d3dBuf);
        eDelete(m_geoBufs[i]);
    }

    eASSERT(m_shaders.isEmpty());
    m_geoBufs.clear();
    m_inputLayouts.clear();
    m_depthStates.clear();
    m_rasterStates.clear();
    m_samplerStates.clear();
    m_blendStates.clear();

    if (m_fullScreen)
        m_swapChain->SetFullscreenState(FALSE, nullptr);

    eReleaseCom(m_dsvScreen);
    eReleaseCom(m_dsScreen);
    eReleaseCom(m_rtvScreen);
    eReleaseCom(m_swapChain);
    eReleaseCom(m_adapterOutput);
    eReleaseCom(m_adapter);
    eReleaseCom(m_dxgiFactory);
    eReleaseCom(m_devCtx);
    eReleaseCom(m_dev);

    if (m_fullScreen)
        ShowCursor(TRUE);

    if (m_ownWindow)
    {
        DestroyWindow((HWND)m_hwnd);
        m_hwnd = nullptr;
    }
}

void eGraphicsDx11::openWindow(eU32 width, eU32 height, eInt windowFlags, ePtr hwnd)
{
    m_wndWidth = width;
    m_wndHeight = height;
    m_fullScreen = (windowFlags&eWF_FULLSCREEN);
    m_vsync = (windowFlags&eWF_VSYNC);

    if (hwnd)
    {
        m_hwnd = (HWND)hwnd;
        m_ownWindow = eFALSE;
    }
    else
    {
        m_hwnd = _createWindow(width, height, m_fullScreen);
        m_ownWindow = eTRUE;
    }

    _createDeviceAndSwapChain();
    _createRenderTargetView();
    _createDepthStencilView();
    _createDynamicBuffers();

    if (m_fullScreen)
        ShowCursor(FALSE);
}

void eGraphicsDx11::setWindowTitle(const eString &title)
{
    SetWindowText((HWND)m_hwnd, title);
}

void eGraphicsDx11::handleMessages(eMessage &msg)
{
    msg = eMSG_IDLE;

    MSG winMsg;
    if (PeekMessage(&winMsg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&winMsg);
        DispatchMessage(&winMsg);
        
        msg = (winMsg.message == WM_QUIT ? eMSG_QUIT : eMSG_BUSY);
    }
}

void eGraphicsDx11::resizeBackbuffer(eU32 width, eU32 height)
{
    if (m_wndWidth != width || m_wndHeight != height)
    {
        m_wndWidth = width;
        m_wndHeight = height;

        if (m_swapChain)
        {
            eReleaseCom(m_dsvScreen);
            eReleaseCom(m_dsScreen);
            eReleaseCom(m_rtvScreen);
            eCallDx(m_swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
            _createRenderTargetView();
            _createDepthStencilView();
        }
    }
}

void eGraphicsDx11::clear(eInt clearMode, const eColor &col)
{
    _activateRenderState();

    // color target has to be cleared?
    if (clearMode&eCM_COLOR)
    {
        eVector4 fc;
        fc = col;

        for (eU32 i=0; i<eGFX_MAXMRT; i++)
            if (m_rtvsActive[i])
                m_devCtx->ClearRenderTargetView(m_rtvsActive[i], &fc.x);
    }

    // depth/stencil target has to be cleared?
    eU32 clear = 0;
        
    if (clearMode&eCM_DEPTH)
        clear |= D3D11_CLEAR_DEPTH;
    if (clearMode&eCM_STENCIL)
        clear |= D3D11_CLEAR_STENCIL;

    if (clear)
        m_devCtx->ClearDepthStencilView(m_dsvActive, clear, 1.0f, 0);
}

void eGraphicsDx11::beginFrame()
{
#ifdef eEDITOR
    eMemSet(&m_renderStats, 0, sizeof(m_renderStats));
#endif

    m_devCtx->Begin(m_occlQuery[m_frameNum]);
}

void eGraphicsDx11::endFrame()
{
#ifdef eEDITOR
    m_engineStats.numTex2d = m_texs2d.size();
    m_engineStats.numTex3d = m_texs3d.size();
    m_engineStats.numTexCube = m_texsCube.size();
    m_engineStats.numGeos = m_geos.size();
    m_engineStats.numGeoBuffs = m_geoBufs.size();
    m_engineStats.numShaders = m_shaders.size();
    m_engineStats.numStates = m_depthStates.size()+m_rasterStates.size()+m_samplerStates.size()+m_blendStates.size();
    m_engineStats.usedGpuMem = m_engineStats.usedGpuMemGeo+m_engineStats.usedGpuMemTex;
#endif

    ePROFILER_FUNC();
    eCallDx(m_swapChain->Present(m_vsync ? 1 : 0, 0));

    // prevents stuttering
    m_devCtx->End(m_occlQuery[m_frameNum]);
    ++m_frameNum %= FRAMES;

    if (!m_frameNum)
        m_startPull = eTRUE;

    if (m_startPull)
        while (m_devCtx->GetData(m_occlQuery[m_frameNum], nullptr, 0, 0) == S_FALSE)
            Sleep(1);
}

#ifdef eDEBUG
#include <sys/stat.h>

static eS64 getFileChangedTime(const eChar *fileName)
{
    struct _stat stat;
    if (_stat(fileName, &stat) == 0)
        return stat.st_mtime;

    return -1;
}

void eGraphicsDx11::reloadEditedShaders()
{
    ePROFILER_FUNC();

    for (eU32 i=0; i<m_shaders.size(); i++)
    {
        eIShader *shader = m_shaders[i];
        const eS64 newTime = getFileChangedTime(shader->filePath);
    
        if (newTime != -1 && newTime > shader->lastTime)
        {
            shader->lastTime = newTime;
            _loadShader(shader->filePath, shader->define, shader);
        }
    }
}
#endif

eU32 eGraphicsDx11::getResolutionCount() const
{
    return m_resolutions.size();
}

const eSize & eGraphicsDx11::getResolution(eU32 index) const
{
    return m_resolutions[index];
}

#ifdef eEDITOR
const eRenderStats & eGraphicsDx11::getRenderStats() const
{
    return m_renderStats;
}

const eEngineStats & eGraphicsDx11::getEngineStats() const
{
    return m_engineStats;
}
#endif

eBool eGraphicsDx11::getFullScreen() const
{
    return m_fullScreen;
}

eU32 eGraphicsDx11::getWndWidth() const
{
    return m_wndWidth;
}

eU32 eGraphicsDx11::getWndHeight() const
{
    return m_wndHeight;
}

eSize eGraphicsDx11::getWndSize() const
{
    return eSize(m_wndWidth, m_wndHeight);
}

void eGraphics::setMatrices(const eMatrix4x4 &modelMtx, const eMatrix4x4 &viewMtx, const eMatrix4x4 &projMtx)
{
    m_modelMtx = modelMtx;
    m_viewMtx = viewMtx;
    m_projMtx = projMtx;
}

const eMatrix4x4 & eGraphics::getViewMatrix() const
{
    return m_viewMtx;
}

const eMatrix4x4 & eGraphics::getModelMatrix() const
{
    return m_modelMtx;
}

const eMatrix4x4 & eGraphics::getProjMatrix() const
{
    return m_projMtx;
}

void eGraphics::getBillboardVectors(eVector3 &right, eVector3 &up, eVector3 *view) 
{
    right.set(m_viewMtx.m11, m_viewMtx.m21, m_viewMtx.m31);
    right.normalize();
    up.set(m_viewMtx.m12, m_viewMtx.m22, m_viewMtx.m32);
    up.normalize();

    if (view)
    {
        view->set(m_viewMtx.m13, m_viewMtx.m23, m_viewMtx.m33);
        view->normalize();
    }
}

eRenderStateDx11 & eGraphicsDx11::pushRenderState()
{
    m_rsStack.push(m_rsEdit);
    return m_rsEdit;
}

eRenderStateDx11 & eGraphicsDx11::popRenderState()
{
    m_rsEdit = m_rsStack.pop();
    return m_rsEdit;
}

// sets active render state to default values
eRenderStateDx11 & eGraphicsDx11::freshRenderState()
{
    eMemSet(&m_rsEdit, 0, sizeof(m_rsEdit));

    // depth stencil states
    m_rsEdit.depthTest = eTRUE;
    m_rsEdit.depthWrite = eTRUE;
    m_rsEdit.depthFunc = eDF_LESS;

    // rasterizer states
    m_rsEdit.cullMode = eCULL_BACK;

    // blend states
    m_rsEdit.blendSrc = eBM_ONE;
    m_rsEdit.blendDst = eBM_ZERO;
    m_rsEdit.blendOp = eBO_ADD;
    m_rsEdit.colorWrite = eTRUE;

    // sampler states
    for (eInt i=0; i<eGFX_MAXTEX; i++)
        m_rsEdit.texFlags[i] = eTMF_CLAMP|eTMF_BILINEAR;

    // rest
    m_rsEdit.targets[0] = TARGET_SCREEN;
    m_rsEdit.viewport.setWidth(m_wndWidth);
    m_rsEdit.viewport.setHeight(m_wndHeight);

    return m_rsEdit;
}

eRenderStateDx11 & eGraphicsDx11::getRenderState()
{
    return m_rsEdit;
}

eGeometry * eGraphicsDx11::addGeometry(eInt flags, eVertexType vtxType, eGeoPrimitiveType primType, eGeoFillCallback fillCb, ePtr fcParam)
{
    eGeometry *geo = m_geos.append(new eGeometryDx11);
    geo->dynamic = (flags&eGEO_DYNAMIC);
    geo->vtxType = vtxType;
    geo->primType = primType;
    geo->fillCb = fillCb;
    geo->fcParam = fcParam;
    geo->indexed = ((flags&eGEO_IB16) || (flags&eGEO_IB32));
    geo->vtxSize = eVERTEX_SIZES[vtxType];
    geo->idxSize = (flags&eGEO_IB32 ? sizeof(eU32) : sizeof(eU16));
    geo->loading = eFALSE;
    geo->usedIndices = 0;
    geo->usedVerts = 0;
    geo->maxIndices = 0;
    geo->maxVerts = 0;
    geo->ib = nullptr;
    geo->vb = nullptr;

//	if (!geo->dynamic)
//		g_numAddsStaticGeo++;

    // quad lists can't be indexed
    eASSERT(primType != eGPT_QUADLIST || (primType == eGPT_QUADLIST && !geo->indexed));
    // sprite lists can be neither indexed nor dynamic
    eASSERT(primType != eGPT_SPRITELIST || (primType == eGPT_SPRITELIST && geo->indexed && geo->dynamic));

    return geo;
}

void eGraphicsDx11::removeGeometry(eGeometry *&geo)
{
    const eInt index = m_geos.find(geo);
    if (index >= 0)
    {
//	if (!geo->dynamic)
//		g_numRemsStaticGeo++;

        if (geo->ib)
            geo->ib->allocs--;
        if (geo->vb)
            geo->vb->allocs--;

        eDelete(geo);
        m_geos.removeAt(index);
    }
}

void eGraphicsDx11::beginLoadGeometry(eGeometryDx11 *geo, eU32 vertexCount, ePtr *vertices, eU32 indexCount, ePtr *indices)
{
    eASSERT(vertices);
    eASSERT(!geo->loading);
    geo->loading = eTRUE;
    geo->maxVerts = vertexCount;
    geo->maxIndices = indexCount;
    geo->usedVerts = 0;
    geo->usedIndices = 0;

    const eU32 reqVbSize = vertexCount*geo->vtxSize;
    const eU32 reqIbSize = indexCount*geo->idxSize;
    D3D11_MAPPED_SUBRESOURCE msr;

    if (geo->dynamic) // is dynamic geometry?
    {
        // lock vertex buffer
        geo->vb = m_geoBufs[GBID_VB_DYN];
        eASSERT(reqVbSize <= geo->vb->size);

        if (geo->vb->pos+reqVbSize >= geo->vb->size)
        {
            geo->vb->pos = 0;
            m_devCtx->Map(geo->vb->d3dBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        }
        else
            m_devCtx->Map(geo->vb->d3dBuf, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &msr);

        *vertices = (eU8 *)msr.pData+geo->vb->pos;

        // lock index buffer
        if (geo->indexed)
        {
            geo->ib = m_geoBufs[geo->idxSize == 2 ? GBID_IB_DYN16 : GBID_IB_DYN32];
            eASSERT(indices);
            eASSERT(reqIbSize <= geo->ib->size);

            if (geo->ib->pos+reqIbSize >= geo->ib->size)
            {
                geo->ib->pos = 0;
                m_devCtx->Map(geo->ib->d3dBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
            }
            else
                m_devCtx->Map(geo->ib->d3dBuf, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &msr);

            *indices = (eU8 *)msr.pData+geo->ib->pos;
        }
    }
    else // is static geometry?
    {
        // vb
        for (eU32 i=GBID_BUFS_USER; i<m_geoBufs.size(); i++)
        {
            if (m_geoBufs[i]->isVb)
            {
                const eU32 newPos = m_geoBufs[i]->pos+reqVbSize;
                if (newPos < m_geoBufs[i]->size)
                {
                    geo->vb = m_geoBufs[i];
                    break;
                }
            }
        }

        if (!geo->vb)
            geo->vb = _createGeoBuffer(eTRUE, eMax(reqVbSize, (eU32)GBS_VB_DYN), eFALSE);

        m_geoMapData[0].resize(reqVbSize);
        *vertices = &m_geoMapData[0][0];

        // ib
        for (eU32 i=GBID_BUFS_USER; i<m_geoBufs.size(); i++)
        {
            if (!m_geoBufs[i]->isVb)
            {
                const eU32 newPos = m_geoBufs[i]->pos+reqIbSize;
                if (newPos < m_geoBufs[i]->size)
                {
                    geo->ib = m_geoBufs[i];
                    break;
                }
            }
        }

        if (!geo->ib)
            geo->ib = _createGeoBuffer(eFALSE, eMax(reqIbSize, (eU32)GBS_IB_DYN), eFALSE);

        m_geoMapData[1].resize(reqIbSize);
        *indices = &m_geoMapData[1][0];
    }

    geo->vb->allocs++;
    geo->vbPos = geo->vb->pos;

    if (geo->ib)
    {
        geo->ib->allocs++;
        geo->ibPos = geo->ib->pos;
    }
}

void eGraphicsDx11::endLoadGeometry(eGeometry *geo, eInt vertexCount, eInt indexCount)
{
    eASSERT(geo->loading);
    geo->loading = eFALSE;

    geo->usedVerts += (vertexCount != -1 ? vertexCount : geo->maxVerts);
    eASSERT(geo->usedVerts <= geo->maxVerts);
    geo->usedIndices += (indexCount != -1 ? indexCount : geo->maxIndices);
    eASSERT(geo->usedIndices <= geo->maxIndices);

    // unlock index buffer
    if (geo->indexed)
    {
        eASSERT(geo->ib);

        if (geo->dynamic)
            m_devCtx->Unmap(geo->ib->d3dBuf, 0);
        else
        {
            D3D11_BOX box;
            eMemSet(&box, 0, sizeof(D3D11_BOX));
            box.left = geo->ib->pos;
            box.right = geo->ib->pos+geo->usedIndices*geo->idxSize;
            box.back = 1;
            box.bottom = 1;
            m_devCtx->UpdateSubresource(geo->ib->d3dBuf, 0, &box, &m_geoMapData[1][0], 0, 0);
        }

        geo->ib->pos += geo->usedIndices*geo->idxSize;
    }

    // unlock vertex buffer
    eASSERT(geo->vb);

    if (geo->dynamic)
        m_devCtx->Unmap(geo->vb->d3dBuf, 0);
    else
    {
        D3D11_BOX box;
        eMemSet(&box, 0, sizeof(D3D11_BOX));
        box.left = geo->vb->pos;
        box.right = geo->vb->pos+geo->usedVerts*geo->vtxSize;
        box.back = 1;
        box.bottom = 1;
        m_devCtx->UpdateSubresource(geo->vb->d3dBuf, 0, &box, &m_geoMapData[0][0], 0, 0);
    }

    geo->vb->pos += geo->usedVerts*geo->vtxSize;
}

void eGraphicsDx11::renderGeometry(eGeometry *geo, const eArray<eInstVtx> &insts)
{
    // dynamic buffer has to be filled from callback?
    if (geo->dynamic && geo->fillCb)
        geo->fillCb(geo, geo->fcParam);

    eASSERT(!geo->loading);

    // is there anything to render?
    if (!geo->usedVerts)
        return;

    D3D11_PRIMITIVE_TOPOLOGY topology;
    const eU32 numObjs = (insts.isEmpty() ? 1 : insts.size());
    eU32 triCount = 0;
    eU32 lineCount = 0;

    switch (geo->primType)
    {
    case eGPT_TRILIST:
        topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        triCount = geo->usedVerts/3;
        break;

    case eGPT_QUADLIST:
        topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        triCount = geo->usedVerts/4*2;
        eASSERT(!geo->indexed);
        break;

    case eGPT_LINELIST:
        topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        lineCount = geo->usedVerts/2;
        break;

    case eGPT_TRISTRIPS:
        topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        triCount = geo->usedVerts-2;
        break;

    case eGPT_LINESTRIPS:
        topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        lineCount = geo->usedVerts-1;
        break;
    }

#ifdef eEDITOR
    m_renderStats.batches++;
    m_renderStats.instances += insts.size();
    m_renderStats.triangles += numObjs*triCount;
    m_renderStats.vertices += numObjs*geo->usedVerts;
    m_renderStats.lines += numObjs*lineCount;
#endif

    // activate render state and set input data
    _activateRenderState();

    m_devCtx->IASetPrimitiveTopology(topology);
    m_devCtx->IASetInputLayout(m_inputLayouts[geo->vtxType]);
    m_devCtx->IASetVertexBuffers(0, 1, &geo->vb->d3dBuf, &eVERTEX_SIZES[geo->vtxType], &geo->vbPos);

    // fill instance buffer
    if (insts.size() > 0)
    {
        const eU32 bufSize = insts.size()*sizeof(eInstVtx);
        eASSERT(bufSize <= m_geoBufs[GBID_VB_INST]->size);
        const eU32 nextInstVbPos = m_geoBufs[GBID_VB_INST]->pos+bufSize;

        D3D11_MAPPED_SUBRESOURCE msr;

        if (nextInstVbPos >= m_geoBufs[GBID_VB_INST]->size)
        {
            m_geoBufs[GBID_VB_INST]->pos = 0;
            m_devCtx->Map(m_geoBufs[GBID_VB_INST]->d3dBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        }
        else
            m_devCtx->Map(m_geoBufs[GBID_VB_INST]->d3dBuf, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &msr);

        eMemCopy((eU8 *)msr.pData+m_geoBufs[GBID_VB_INST]->pos, &insts[0], bufSize);
        m_devCtx->Unmap(m_geoBufs[GBID_VB_INST]->d3dBuf, 0);
        m_devCtx->IASetVertexBuffers(1, 1, &m_geoBufs[GBID_VB_INST]->d3dBuf, &eVERTEX_SIZES[eVTX_INSTANCE], &m_geoBufs[GBID_VB_INST]->pos);
        m_geoBufs[GBID_VB_INST]->pos += bufSize;
    }

    // render geometry
    if (geo->primType == eGPT_QUADLIST)
    {
        m_devCtx->IASetIndexBuffer(m_geoBufs[GBID_IB_QUAD]->d3dBuf, DXGI_FORMAT_R16_UINT, 0);
        const eU32 numIndices = triCount*3;

        if (insts.size())
            m_devCtx->DrawIndexedInstanced(numIndices, insts.size(), 0, 0, 0);
        else
            m_devCtx->DrawIndexed(numIndices, 0, 0);
    }
    else if (geo->indexed)
    {
        m_devCtx->IASetIndexBuffer(geo->ib->d3dBuf, (geo->idxSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT), geo->ibPos);

        if (insts.size())
            m_devCtx->DrawIndexedInstanced(geo->usedIndices, insts.size(), 0, 0, 0);
        else
            m_devCtx->DrawIndexed(geo->usedIndices, 0, 0);
    }
    else
    {
        m_devCtx->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

        if (insts.size())
            m_devCtx->DrawInstanced(geo->usedVerts, insts.size(), 0, 0);
        else
            m_devCtx->Draw(geo->usedVerts, 0);
    }
}

eTexture2dDx11 * eGraphicsDx11::addTexture2d(eU32 width, eU32 height, eInt flags, eTextureFormat format)
{
	//g_numAddsTex2d++;

    eASSERT(width*height > 0);

#ifdef eDEBUG
    if (flags&eTEX_DYNAMIC) // mipmaps not allowed for dynamic textures
        eASSERT(flags&eTEX_NOMIPMAPS);
    if (flags&eTEX_TARGET) // dynamic not allowed for targets
        eASSERT(!(flags&eTEX_DYNAMIC));
#endif

    eTexture2dDx11 *tex = m_texs2d.append(new eTexture2dDx11);
    eMemSet(tex, 0, sizeof(eTexture2dDx11));
    tex->width = width;
    tex->height = height;
    tex->pixelSize = TEXTURE_FORMAT_INFOS[format].pixelSize;
    tex->format = format;
    tex->target = (flags&eTEX_TARGET);
    tex->dynamic = (flags&eTEX_DYNAMIC);
    tex->mipmaps = (!(flags&eTEX_TARGET) && !(flags&eTEX_NOMIPMAPS));
    tex->mipLevels = (tex->mipmaps ? eFtoL(eLog2((eF32)eMax(width, height))) : 1);
    tex->d3dReadTex = nullptr;

    D3D11_TEXTURE2D_DESC desc;
    eMemSet(&desc, 0, sizeof(desc));
    desc.Format = TEXTURE_FORMAT_INFOS[format].d3dFormat;
    desc.ArraySize = 1;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = (tex->mipmaps ? 0 : 1);
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.MiscFlags = (tex->mipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);
    desc.BindFlags |= (flags&eTEX_UAV ? D3D11_BIND_UNORDERED_ACCESS : 0);

    if (tex->format == eTFO_DEPTH32F)
    {
        desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
        desc.Usage = D3D11_USAGE_DEFAULT;
        eCallDx(m_dev->CreateTexture2D(&desc, nullptr, &tex->d3dTex));

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
        eMemSet(&dsvd, 0, sizeof(dsvd));
        dsvd.Format = DXGI_FORMAT_D32_FLOAT;
        dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvd.Texture2D.MipSlice = 0;
        eCallDx(m_dev->CreateDepthStencilView(tex->d3dTex, &dsvd, &tex->d3dDsv));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
        eMemSet(&srvd, 0, sizeof(srvd));
        srvd.Format = DXGI_FORMAT_R32_FLOAT;
        srvd.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = desc.MipLevels;
        srvd.Texture2D.MostDetailedMip = 0;
        eCallDx(m_dev->CreateShaderResourceView(tex->d3dTex, &srvd, &tex->d3dSrv));
    }
    else if (tex->target)
    {
        desc.BindFlags |= D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        desc.Usage = D3D11_USAGE_DEFAULT;

        eCallDx(m_dev->CreateTexture2D(&desc, nullptr, &tex->d3dTex));
        eCallDx(m_dev->CreateRenderTargetView(tex->d3dTex, nullptr, &tex->d3dRtv));
        eCallDx(m_dev->CreateShaderResourceView(tex->d3dTex, nullptr, &tex->d3dSrv));
    }
    else
    {
        desc.Usage = (tex->dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
        desc.CPUAccessFlags = (tex->dynamic ? D3D11_CPU_ACCESS_WRITE : 0);
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE|(tex->mipmaps ? D3D11_BIND_RENDER_TARGET : 0);

        eCallDx(m_dev->CreateTexture2D(&desc, nullptr, &tex->d3dTex));
        eCallDx(m_dev->CreateShaderResourceView(tex->d3dTex, nullptr, &tex->d3dSrv));
    }

#ifdef eEDITOR
    // account for mipmaps (1.33x more memory required)
    for (eU32 i=0, s=width*height*tex->pixelSize; i<tex->mipLevels; i++, s/=4)
        m_engineStats.usedGpuMemTex += s;
#endif

    return tex;
}

eTexture3dDx11 * eGraphicsDx11::addTexture3d(eU32 width, eU32 height, eU32 depth, eInt flags, eTextureFormat format)
{
	//g_numAddsTex3d++;

    eASSERT(width*height*depth > 0);
    eASSERT(!(flags&eTEX_TARGET));

    eTexture3dDx11 *tex = m_texs3d.append(new eTexture3dDx11);
    tex->width = width;
    tex->height = height;
    tex->pixelSize = TEXTURE_FORMAT_INFOS[format].pixelSize;
    tex->depth = depth;
    tex->format = format;
    tex->dynamic = (flags&eTEX_DYNAMIC);
    tex->mipmaps = !(flags&eTEX_NOMIPMAPS);
    tex->mipLevels = (tex->mipmaps ? eFtoL(eLog2((eF32)eMax(eMax(width, height), depth))) : 1);

    D3D11_TEXTURE3D_DESC desc;
    eMemSet(&desc, 0, sizeof(desc));
    desc.Format = TEXTURE_FORMAT_INFOS[format].d3dFormat;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = depth;
    desc.MipLevels = (tex->mipmaps ? 0 : 1);
    desc.Usage = (tex->dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
    desc.CPUAccessFlags = (tex->dynamic ? D3D11_CPU_ACCESS_WRITE : 0);
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = (tex->mipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);

    eCallDx(m_dev->CreateTexture3D(&desc, nullptr, &tex->d3dTex));
    eCallDx(m_dev->CreateShaderResourceView(tex->d3dTex, nullptr, &tex->d3dSrv));

#ifdef eEDITOR
    // account for mipmaps
    for (eU32 i=0, s=width*height*depth*tex->pixelSize; i<tex->mipLevels; i++, s/=8)
        m_engineStats.usedGpuMemTex -= s;
#endif

    return tex;
}

eTextureCubeDx11 * eGraphicsDx11::addTextureCube(eU32 size, eInt flags, eTextureFormat format)
{
	//g_numAddsTexCube++;

    eASSERT(size > 0);

    eTextureCubeDx11 *tex = m_texsCube.append(new eTextureCubeDx11);
    eMemSet(tex, 0, sizeof(eTextureCubeDx11));
    tex->size = size;
    tex->pixelSize = TEXTURE_FORMAT_INFOS[format].pixelSize;
    tex->format = format;
    tex->target = flags&eTEX_TARGET;
    tex->dynamic = flags&eTEX_DYNAMIC;

    D3D11_TEXTURE2D_DESC desc;
    eMemSet(&desc, 0, sizeof(desc));
    desc.Format = TEXTURE_FORMAT_INFOS[format].d3dFormat;
    desc.Width = size;
    desc.Height = size;
    desc.MipLevels = 1;
    desc.Usage = (tex->dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
    desc.CPUAccessFlags = (tex->dynamic ? D3D11_CPU_ACCESS_WRITE : 0);
    desc.SampleDesc.Quality = 0;
    desc.SampleDesc.Count = 1;
    desc.ArraySize = eCMF_COUNT;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE|(tex->target ? D3D11_BIND_RENDER_TARGET : 0);

    eCallDx(m_dev->CreateTexture2D(&desc, nullptr, &tex->d3dTex));
    eCallDx(m_dev->CreateShaderResourceView(tex->d3dTex, nullptr, &tex->d3dSrv));

    if (tex->target)
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc;
        eMemSet(&desc, 0, sizeof(desc));
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;

        for (eInt i=0; i<eCMF_COUNT; i++)
        {
            eTexture2d &faceTex = tex->faces[i];
            eMemSet(&faceTex, 0, sizeof(faceTex));
            faceTex.target = tex->target;
            faceTex.dynamic = tex->dynamic;
            faceTex.width = size;
            faceTex.height = size;
            faceTex.format = format;
            faceTex.d3dTex = tex->d3dTex;
            faceTex.d3dSrv = tex->d3dSrv;

            desc.Texture2DArray.FirstArraySlice = i;
            eCallDx(m_dev->CreateRenderTargetView(tex->d3dTex, &desc, &faceTex.d3dRtv));
        }
    }

#ifdef eEDITOR
    m_engineStats.usedGpuMemTex += size*size*6*tex->pixelSize;
#endif
    return tex;
}

void eGraphicsDx11::removeTexture2d(eTexture2dDx11 *&tex)
{
	//g_numRemsTex2d++;

    const eInt index = m_texs2d.find(tex);
    if (index >= 0)
    {
        // bound as texture?
        for (eU32 j=0; j<eGFX_MAXTEX; j++)
            if (m_rsEdit.textures[j] == tex)
                m_rsEdit.textures[j] = nullptr;

        // bound as target?
        for (eU32 j=0; j<eGFX_MAXMRT; j++)
            if (m_rsEdit.targets[j] == tex)
                m_rsEdit.targets[j] = nullptr;

        // remove texture
#ifdef eEDITOR
        for (eU32 i=0, s=tex->width*tex->height*tex->pixelSize; i<tex->mipLevels; i++, s/=4)
            m_engineStats.usedGpuMemTex -= s;
#endif

        eReleaseCom(tex->d3dReadTex);
        eReleaseCom(tex->d3dTex);
        eReleaseCom(tex->d3dDsv);
        eReleaseCom(tex->d3dRtv);
        eReleaseCom(tex->d3dSrv);
        eDelete(tex);
        m_texs2d.removeAt(index);
    }
}

void eGraphicsDx11::removeTexture3d(eTexture3dDx11 *&tex)
{
	//g_numRemsTex3d++;

    const eInt index = m_texs3d.find(tex);
    if (index >= 0)
    {
        // bound as texture?
        for (eU32 j=0; j<eGFX_MAXTEX; j++)
            if (m_rsEdit.textures[j] == tex)
                m_rsEdit.textures[j] = nullptr;

#ifdef eEDITOR
        // remove texture
        for (eU32 i=0, s=tex->width*tex->height*tex->depth*tex->pixelSize; i<tex->mipLevels; i++, s/=8)
            m_engineStats.usedGpuMemTex -= s;
#endif

        eReleaseCom(tex->d3dTex);
        eReleaseCom(tex->d3dSrv);
        eDelete(tex);
        m_texs3d.removeAt(index);
    }
}

void eGraphicsDx11::removeTextureCube(eTextureCubeDx11 *&tex)
{
	//g_numRemsTexCube++;

    const eInt index = m_texsCube.find(tex);
    if (index >= 0)
    {
        // bound as texture?
        for (eU32 j=0; j<eGFX_MAXTEX; j++)
            if (m_rsEdit.textures[j] == tex)
                m_rsEdit.textures[j] = nullptr;

        // bound as target?
        for (eU32 j=0; j<eGFX_MAXMRT; j++)
            for (eU32 k=0; k<eCMF_COUNT; k++)
                if (m_rsEdit.targets[j] == &tex->faces[k])
                    m_rsEdit.targets[j] = nullptr;

        // remove texture
        if (tex->target)
            for (eU32 j=0; j<eCMF_COUNT; j++)
                eReleaseCom(tex->faces[j].d3dRtv);

#ifdef eEDITOR
        m_engineStats.usedGpuMemTex -= tex->size*tex->size*6*tex->pixelSize;
#endif

        eReleaseCom(tex->d3dTex);
        eReleaseCom(tex->d3dSrv);
        eDelete(tex);
        m_texsCube.removeAt(index);
    }
}

void eGraphicsDx11::updateTexture2d(eTexture2dDx11 *tex, eConstPtr data)
{
    D3D11_BOX box;
    eMemSet(&box, 0, sizeof(box));
    box.right = tex->width;
    box.bottom = tex->height;
    box.back = 1;

    m_devCtx->UpdateSubresource(tex->d3dTex, 0, &box, data, tex->width*tex->pixelSize, 0);

    if (tex->mipmaps)
        m_devCtx->GenerateMips(tex->d3dSrv);
}

void eGraphicsDx11::updateTexture2d(eTexture2dDx11 *dst, eTexture2dDx11 *src, const ePoint &dstPos, const eRect &srcRegion)
{
    D3D11_BOX box;
    box.left = srcRegion.left;
    box.top = srcRegion.top;
    box.right = srcRegion.right;
    box.bottom = srcRegion.bottom;
    box.front = 0;
    box.back = 1;

    m_devCtx->CopySubresourceRegion(dst->d3dTex, 0, dstPos.x, dstPos.y, 0, src->d3dTex, 0, &box);

    if (dst->mipmaps)
        m_devCtx->GenerateMips(dst->d3dSrv);
}

void eGraphicsDx11::updateTexture3d(eTexture3dDx11 *tex, eConstPtr data)
{
    const eU32 rowPitch = tex->width*tex->pixelSize;
    const eU32 depthPitch = rowPitch*tex->height;

    D3D11_BOX box;
    eMemSet(&box, 0, sizeof(box));
    box.right = tex->width;
    box.bottom = tex->height;
    box.back = tex->depth;

    m_devCtx->UpdateSubresource(tex->d3dTex, 0, &box, data, rowPitch, depthPitch);

    if (tex->mipmaps)
        m_devCtx->GenerateMips(tex->d3dSrv);
}

void eGraphicsDx11::updateTextureCube(eTextureCubeDx11 *tex, eConstPtr data, eCubeMapFace face)
{   
    D3D11_BOX box;
    eMemSet(&box, 0, sizeof(box));
    box.right = tex->size;
    box.bottom = tex->size;
    box.back = 1;

    m_devCtx->UpdateSubresource(tex->d3dTex, face, &box, data, tex->size*tex->pixelSize, 0);
}

void eGraphicsDx11::readTexture2d(eTexture2dDx11 *tex, eArray<eColor> &texData) const
{
    // create staging texture for read back
    if (!tex->d3dReadTex)
    {
        D3D11_TEXTURE2D_DESC desc;
        tex->d3dTex->GetDesc(&desc);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        eCallDx(m_dev->CreateTexture2D(&desc, nullptr, &tex->d3dReadTex));
    }

    // copy data from GPU to CPU
    D3D11_MAPPED_SUBRESOURCE msr;
    m_devCtx->CopyResource(tex->d3dReadTex, tex->d3dTex);
    m_devCtx->Map(tex->d3dReadTex, 0, D3D11_MAP_READ, 0, &msr);
    texData.resize(tex->height*tex->width);

    if (tex->format == eTFO_ARGB8)
    {
        for (eU32 i=0; i<tex->height; i++)
            eMemCopy(&texData[i*tex->width], (eU8 *)msr.pData+i*msr.RowPitch, tex->width*4);
    }
    else
        eASSERT(eFALSE);

    m_devCtx->Unmap(tex->d3dReadTex, 0);
}

eUavBufferDx11 * eGraphicsDx11::addUavBuffer(eU32 width, eU32 height, eTextureFormat format)
{
    eUavBufferDx11 *uav = m_uavBufs.append(new eUavBufferDx11);
    uav->tex = addTexture2d(width, height, eTEX_UAV|eTEX_NOMIPMAPS, format);

    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
    eMemSet(&viewDesc, 0, sizeof(viewDesc));
    viewDesc.Buffer.FirstElement = 0;
    viewDesc.Buffer.Flags = 0;
    viewDesc.Buffer.NumElements = uav->tex->width*uav->tex->height;
    viewDesc.Format = DXGI_FORMAT_UNKNOWN;
    viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    eCallDx(m_dev->CreateUnorderedAccessView(uav->tex->d3dTex, &viewDesc, &uav->d3dUav));
    return uav;
}

void eGraphicsDx11::removeUavBuffer(eUavBufferDx11 *&uav)
{
    const eInt index = m_uavBufs.find(uav);
    if (index >= 0)
    {
        removeTexture2d(uav->tex);
        eReleaseCom(uav->d3dUav);
        eDelete(uav);
        m_uavBufs.removeAt(index);
    }
}

void eGraphicsDx11::execComputeShader(eU32 numThreadsX, eU32 numThreadsY, eU32 numThreadsZ)
{
    eASSERT(numThreadsX > 0 && numThreadsY > 0 && numThreadsZ > 0);
    _activateRenderState();
    m_devCtx->Dispatch(numThreadsX, numThreadsY, numThreadsZ);
}

ePixelShaderDx11 * eGraphicsDx11::loadPixelShader(const eChar *src, const eChar *define)
{
    ePixelShaderDx11 *shader = (ePixelShaderDx11 *)_findShader(src, define);
    if (!shader)
    {
        shader = (ePixelShaderDx11 *)m_shaders.append(new ePixelShaderDx11);
        shader->type = eST_PS;
        _loadShader(src, define, shader);
    }

    return shader;
}

eVertexShaderDx11 * eGraphicsDx11::loadVertexShader(const eChar *src, const eChar *define)
{
    eVertexShaderDx11 *shader = (eVertexShaderDx11 *)_findShader(src, define);
    if (!shader)
    {
        shader = (eVertexShaderDx11 *)m_shaders.append(new eVertexShaderDx11);
        shader->type = eST_VS;
        _loadShader(src, define, shader);
    }

    return shader;
}

eGeometryShaderDx11 * eGraphicsDx11::loadGeometryShader(const eChar *src, const eChar *define)
{
    eGeometryShaderDx11 *shader = (eGeometryShaderDx11 *)_findShader(src, define);
    if (!shader)
    {
        shader = (eGeometryShaderDx11 *)m_shaders.append(new eGeometryShaderDx11);
        shader->type = eST_GS;
        _loadShader(src, define, shader);
    }

    return shader;
}

eComputeShaderDx11 * eGraphicsDx11::loadComputeShader(const eChar *src, const eChar *define)
{
    eComputeShaderDx11 *shader = (eComputeShaderDx11 *)_findShader(src, define);
    if (!shader)
    {
        shader = (eComputeShaderDx11 *)m_shaders.append(new eComputeShaderDx11);
        shader->type = eST_CS;
        _loadShader(src, define, shader);
    }

    return shader;
}

void eGraphicsDx11::removeShader(eIShaderDx11 *&shader)
{
    const eInt index = m_shaders.find(shader);
    if (index >= 0)
    {
        ePixelShaderDx11 *ps = (ePixelShaderDx11 *)shader; // we just have to access somehow the D3D resource
        eReleaseCom(ps->d3dPs);
        eDelete(shader);
        m_shaders.removeAt(index);
    }
}

eTexture2d * eGraphicsDx11::createChessTexture(eU32 width, eU32 height, eU32 step, const eColor &col0, const eColor &col1)
{
    eASSERT(width > 2);
    eASSERT(height > 2);
    eASSERT(step < width);
    eASSERT(step < height);
    eASSERT(width%step == 0);
    eASSERT(height%step == 0);

    const eColor colors[2] = {col0, col1};
    eArray<eColor> data(width*height);

    for (eU32 y=0, index=0; y<height; y++)
        for (eU32 x=0; x<width; x++)
            data[index++] = colors[(x/step+y/step)%2];

    eTexture2d *tex = addTexture2d(width, height, 0, eTFO_ARGB8);
    updateTexture2d(tex, &data[0]);
    return tex;
}

// callback function for Direct3D window
static LRESULT CALLBACK wndProc(HWND hwnd, eU32 msg, WPARAM wparam, LPARAM lparam)
{
    static eGraphicsDx11 *gfx = nullptr;

    switch (msg)
    {
    case WM_CREATE:
        // pointer to graphics class passed
        // in creation parameter
        gfx = (eGraphicsDx11 *)(((CREATESTRUCT *)lparam)->lpCreateParams);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        // window width in low, height in high word
        gfx->resizeBackbuffer(eLoword(lparam), eHiword(lparam));
        return 0;

    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

ePtr eGraphicsDx11::_createWindow(eU32 width, eU32 height, eBool fullScreen)
{
    // register window class
    WNDCLASS wc;
    eMemSet(&wc, 0, sizeof(wc));
    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpfnWndProc = wndProc;
    wc.lpszClassName = "Enigma";
    const ATOM res = RegisterClass(&wc);
    eASSERT(res);

    // create window
    if (m_fullScreen)
    {
        return CreateWindow("Enigma", "Enigma", WS_VISIBLE|WS_POPUP, 0, 0, m_wndWidth,
                            m_wndHeight, nullptr, nullptr, nullptr, this);
    }
    else
    {
        // adjust window rect that client area has
        // size of desired resolution
        RECT r;
        r.left = r.top = 0;
        r.right = m_wndWidth;
        r.bottom = m_wndHeight;
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW|WS_VISIBLE, FALSE);    

        return CreateWindow("Enigma", "Enigma", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                            r.right-r.left, r.bottom-r.top, nullptr, nullptr, nullptr, this);
    }
}

void eGraphicsDx11::_createDeviceAndSwapChain()
{
    DXGI_SWAP_CHAIN_DESC desc;
    eMemSet(&desc, 0, sizeof(desc));
    desc.BufferCount = 1;
    desc.BufferDesc.Width = m_wndWidth;
    desc.BufferDesc.Height = m_wndHeight;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 0;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = (HWND)m_hwnd;
    desc.SampleDesc.Count = 1; // turn multisampling off
    desc.Windowed = !m_fullScreen;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    eU32 flags = D3D11_CREATE_DEVICE_SINGLETHREADED;  // dxsdk 2010 cannot do that: |D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT
#ifdef eDEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_DRIVER_TYPE driverType[] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
    const D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_10_1};

    for (eInt i=0; i<eELEMENT_COUNT(driverType); i++)
    {
        if (!FAILED(D3D11CreateDeviceAndSwapChain(nullptr, driverType[i], nullptr, flags, &featureLevel[i], 1, 
                                                  D3D11_SDK_VERSION, &desc, &m_swapChain, &m_dev, NULL, &m_devCtx)))
        {
#ifdef eEDITOR
            m_engineStats.availGpuMem =_getAvailableGpuMemory();
#endif
            _createInputLayouts();

            // create stuttering prevention queries
            for (eInt i=0; i<FRAMES; i++)
            {
                D3D11_QUERY_DESC qd;
                qd.Query = D3D11_QUERY_OCCLUSION;
                qd.MiscFlags = 0;
                eCallDx(m_dev->CreateQuery(&qd, &m_occlQuery[i]));
            }

            return;
        }
    }
    
    eShowError("Couldn't create Direct3D 11 device!");
    eFatal(-1);
}

eU32 eGraphicsDx11::_getAvailableGpuMemory()
{
    IDXGIDevice * dxGiDev = nullptr;
    IDXGIAdapter * dxGiAdapter = nullptr;
    DXGI_ADAPTER_DESC ad;

    eCallDx(m_dev->QueryInterface(__uuidof(IDXGIDevice), (ePtr *)&dxGiDev));
    eCallDx(dxGiDev->GetAdapter(&dxGiAdapter));
    eCallDx(dxGiAdapter->GetDesc(&ad));
    eReleaseCom(dxGiAdapter);
    eReleaseCom(dxGiDev);

    return ad.DedicatedVideoMemory+ad.SharedSystemMemory;
}

void eGraphicsDx11::_createInputLayouts()
{
    for (eInt i=0; i<eELEMENT_COUNT(INPUT_LAYOUT_INFOS); i++)
    {
        const InputLayoutInfo &ili = INPUT_LAYOUT_INFOS[i];
        ID3D11VertexShader *vs = nullptr;
        ID3D11InputLayout *il = nullptr;
        ID3DBlob *binary = nullptr;

        eCallDx(D3DCompile(ili.dummyShader, eStrLength(ili.dummyShader), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &binary, nullptr));
        eCallDx(m_dev->CreateVertexShader(binary->GetBufferPointer(), binary->GetBufferSize(), nullptr, &vs));
        eCallDx(m_dev->CreateInputLayout(ili.desc, ili.elements, binary->GetBufferPointer(), binary->GetBufferSize(), &il));
        eReleaseCom(vs);

        m_inputLayouts.append(il);
    }
}

void eGraphicsDx11::_createRenderTargetView()
{
    ID3D11Texture2D *backBufferPtr = nullptr;
    eCallDx(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (ePtr *)&backBufferPtr));
    eCallDx(m_dev->CreateRenderTargetView(backBufferPtr, NULL, &m_rtvScreen));
    eReleaseCom(backBufferPtr);
}

void eGraphicsDx11::_createDepthStencilView()
{
    D3D11_TEXTURE2D_DESC td;
    eMemSet(&td, 0, sizeof(td));
    td.Width = m_wndWidth;
    td.Height = m_wndHeight;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;
    eCallDx(m_dev->CreateTexture2D(&td, NULL, &m_dsScreen));

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
    eMemSet(&dsvd, 0, sizeof(dsvd));
    dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvd.Texture2D.MipSlice = 0;
    eCallDx(m_dev->CreateDepthStencilView(m_dsScreen, &dsvd, &m_dsvScreen));
}

void eGraphicsDx11::_createDynamicBuffers()
{
    _createGeoBuffer(eTRUE,  GBS_VB_DYN,   eTRUE);
    _createGeoBuffer(eFALSE, GBS_IB_DYN,   eTRUE);
    _createGeoBuffer(eFALSE, GBS_IB_DYN*2, eTRUE);
    _createGeoBuffer(eFALSE, GBS_IB_QUAD,      eFALSE);
    _createGeoBuffer(eTRUE,  GBS_VB_INST,  eTRUE);

    // initialize index buffer for quads
    eArray<eU16> quadIb(GBE_IB_QUAD);
    eU16 *ip = &quadIb[0];

    for (eU32 i=0, j=0; i<GBE_IB_QUAD/6; i++, j+=4)
    {
        *ip++ = j+0;
        *ip++ = j+1;
        *ip++ = j+2;
        *ip++ = j+0;
        *ip++ = j+2;
        *ip++ = j+3;
    }

    D3D11_BOX box;
    eMemSet(&box, 0, sizeof(box));
    box.right = GBS_IB_QUAD;
    box.bottom = 1;
    box.back = 1;
    m_devCtx->UpdateSubresource(m_geoBufs[GBID_IB_QUAD]->d3dBuf, 0, &box, &quadIb[0], 0, 0);
}

eGeoBufferDx11 * eGraphicsDx11::_createGeoBuffer(eBool isVb, eU32 size, eBool dynamic)
{
    eGeoBufferDx11 *buf = m_geoBufs.append(new eGeoBufferDx11);
    buf->isVb = isVb;
    buf->size = size;
    buf->pos = 0;
    buf->allocs = 0;

    D3D11_BUFFER_DESC desc;
    eMemSet(&desc, 0, sizeof(desc));
    desc.Usage = (dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT);
    desc.ByteWidth = size;
    desc.BindFlags = (isVb ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER);
    desc.CPUAccessFlags = (dynamic ? D3D11_CPU_ACCESS_WRITE : 0);
    eCallDx(m_dev->CreateBuffer(&desc, nullptr, &buf->d3dBuf));

#ifdef eEDITOR
    m_engineStats.usedGpuMemGeo += buf->size;
#endif
    return buf;
}

void eGraphicsDx11::_activateConstBuffers()
{
    ePROFILER_FUNC();

    // update constant buffers
    for (eU32 i=0; i<m_cbufs.size(); i++)
        m_cbufs[i].inUse = eFALSE;

    ID3D11Buffer *cbufs[eST_COUNT][eGFX_MAXCBS];
    eMemSet(cbufs, 0, sizeof(cbufs));

    for (eInt i=0; i<eGFX_MAXCBS; i++)
    {
        eIConstBufferDx11 *cb = m_rsEdit.constBufs[i];
        ID3D11Buffer *d3dCb = nullptr;
        eU32 j;

        if (!cb)
            continue;

        for (j=0; j<m_cbufs.size(); j++)
        {
            if (!m_cbufs[j].inUse && m_cbufs[j].size == cb->size)
            {
                d3dCb = m_cbufs[j].d3dBuf;
                m_cbufs[j].inUse = eTRUE;

                D3D11_MAPPED_SUBRESOURCE msr;
                eCallDx(m_devCtx->Map(d3dCb, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
                eMemCopy(msr.pData, cb->dataPtr, cb->size);
                m_devCtx->Unmap(d3dCb, 0);
                break;
            }
        }

        if (j >= m_cbufs.size())
        {
            D3D11_BUFFER_DESC desc;
            eMemSet(&desc, 0, sizeof(desc));
            desc.ByteWidth = eAlign(cb->size, 16); // has to be 16-byte aligned
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            D3D11_SUBRESOURCE_DATA id;
            id.pSysMem = cb->dataPtr;

            BufferData &bd = m_cbufs.append();
            bd.size = cb->size;
            bd.inUse = eTRUE;
            eCallDx(m_dev->CreateBuffer(&desc, &id, &bd.d3dBuf));

            d3dCb = bd.d3dBuf;
        }

        eASSERT(d3dCb);

        if (cb->shaders&eST_PS)
            cbufs[0][i] = d3dCb;
        if (cb->shaders&eST_VS)
            cbufs[1][i] = d3dCb;
        if (cb->shaders&eST_GS)
            cbufs[2][i] = d3dCb;
        if (cb->shaders&eST_CS)
            cbufs[3][i] = d3dCb;
    }

    // set constant buffers
    if (!eMemEqual(m_rsEdit.constBufs, m_rsActive.constBufs, sizeof(m_rsActive.constBufs)))
    {
        m_devCtx->PSSetConstantBuffers(0, eGFX_MAXCBS, &cbufs[0][0]);
        m_devCtx->VSSetConstantBuffers(0, eGFX_MAXCBS, &cbufs[1][0]);
        m_devCtx->GSSetConstantBuffers(0, eGFX_MAXCBS, &cbufs[2][0]);
        m_devCtx->CSSetConstantBuffers(0, eGFX_MAXCBS, &cbufs[3][0]);
    }
}

void eGraphicsDx11::_activateTextures()
{
    if (!eMemEqual(m_rsEdit.textures, m_rsActive.textures, sizeof(m_rsEdit.textures)))
    {
        ID3D11ShaderResourceView *srvs[eGFX_MAXTEX];
        for (eU32 i=0; i<eGFX_MAXTEX; i++)
            srvs[i] = (m_rsEdit.textures[i] ? m_rsEdit.textures[i]->d3dSrv : nullptr);

        m_devCtx->PSSetShaderResources(0, eGFX_MAXTEX, srvs);
        m_devCtx->CSSetShaderResources(0, eGFX_MAXTEX, srvs);
    }
}

void eGraphicsDx11::_activateUavBuffers()
{
    if (!eMemEqual(m_rsActive.uavBufs, m_rsEdit.uavBufs, sizeof(m_rsEdit.uavBufs)))
    {
        ID3D11UnorderedAccessView *d3dUavs[eGFX_MAXUAVS];
        eU32 d3dUavCounts[eGFX_MAXUAVS] = {-1};

        for (eU32 i=0; i<eGFX_MAXUAVS; i++)
            d3dUavs[i] = (m_rsEdit.uavBufs[i] ? m_rsEdit.uavBufs[i]->d3dUav : nullptr);

        m_devCtx->CSSetUnorderedAccessViews(0, eGFX_MAXUAVS, d3dUavs, d3dUavCounts);
    }
}

void eGraphicsDx11::_activateTargets()
{
    // first all D3D resource pointers are collected
    // as a backbuffer resize might alter them. in
    // that case comparing engine resources is not
    // sufficient in order to detect this change.
    ID3D11RenderTargetView *rtvs[eGFX_MAXMRT];
    ID3D11DepthStencilView *dsv = nullptr;
			
    for (eU32 i=0; i<eGFX_MAXMRT; i++)
    {
        if (!m_rsEdit.targets[i])
            rtvs[i] = nullptr;
        else if (m_rsEdit.targets[i] == TARGET_SCREEN)
            rtvs[i] = m_rtvScreen;
        else
            rtvs[i] = m_rsEdit.targets[i]->d3dRtv;
    }

    if (!m_rsEdit.depthTarget || m_rsEdit.depthTarget == TARGET_SCREEN)
        dsv = m_dsvScreen;
    else
        dsv = m_rsEdit.depthTarget->d3dDsv;

    if (dsv != m_dsvActive || !eMemEqual(rtvs, m_rtvsActive, sizeof(rtvs)))
    {
        m_devCtx->OMSetRenderTargets(eGFX_MAXMRT, rtvs, dsv);
        m_dsvActive = dsv;
        eMemCopy(m_rtvsActive, rtvs, sizeof(rtvs));
    }
}

void eGraphicsDx11::_activateRenderState()
{
    ePROFILER_FUNC();

    _activateConstBuffers();

    eU32 i;

    // activate depth stencil state
    if (m_rsActive.depthHash != m_rsEdit.depthHash)
    {
        for (i=0; i<m_depthStates.size() && m_depthStates[i].hash!=m_rsEdit.depthHash; i++);

        if (i >= m_depthStates.size())
        {
            D3D11_DEPTH_STENCIL_DESC desc;
            eMemSet(&desc, 0, sizeof(desc));
            desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            desc.BackFace = desc.FrontFace;
            desc.DepthEnable = m_rsEdit.depthTest;
            desc.DepthWriteMask = (m_rsEdit.depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO);
            desc.DepthFunc = (D3D11_COMPARISON_FUNC)(m_rsEdit.depthFunc+1); // enums offset by 1
            desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
            desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

            eStateInfoDx11 &si = m_depthStates.append();
            si.rs = m_rsEdit;
            si.hash = m_rsEdit.depthHash;
            eCallDx(m_dev->CreateDepthStencilState(&desc, &si.d3dDss));
            i = m_depthStates.size()-1;
        }

        m_devCtx->OMSetDepthStencilState(m_depthStates[i].d3dDss, 1);
    }

    // activate blend state
    if (m_rsActive.blendHash != m_rsEdit.blendHash)
    {
        for (i=0; i<m_blendStates.size() && m_rsEdit.blendHash!=m_blendStates[i].hash; i++);

        if (i >= m_blendStates.size())
        {
            D3D11_BLEND_DESC desc;
            eMemSet(&desc, 0, sizeof(desc));
            desc.RenderTarget[0].BlendEnable = m_rsEdit.blending;
            desc.RenderTarget[0].BlendOp = (D3D11_BLEND_OP)(m_rsEdit.blendOp+1); // enums offset by 1
            desc.RenderTarget[0].BlendOpAlpha = desc.RenderTarget[0].BlendOp;
            desc.RenderTarget[0].SrcBlend = (D3D11_BLEND)(m_rsEdit.blendSrc+1); // enums offset by 1
            desc.RenderTarget[0].SrcBlendAlpha = desc.RenderTarget[0].SrcBlend;
            desc.RenderTarget[0].DestBlend = (D3D11_BLEND)(m_rsEdit.blendDst+1); // enums offset by 1
            desc.RenderTarget[0].DestBlendAlpha = desc.RenderTarget[0].DestBlend;
            desc.RenderTarget[0].RenderTargetWriteMask = (m_rsEdit.colorWrite ? D3D11_COLOR_WRITE_ENABLE_ALL : 0);

            eStateInfoDx11 &si = m_blendStates.append();
            si.hash = m_rsEdit.blendHash;
            si.rs = m_rsEdit;
            eCallDx(m_dev->CreateBlendState(&desc, &si.d3dBs));
            i = m_blendStates.size()-1;
        }

        const eF32 blendFactor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_devCtx->OMSetBlendState(m_blendStates[i].d3dBs, blendFactor, 0xffffffff);
    }

    // activate sampler states
    if (!eMemEqual(m_rsActive.texFlags, m_rsEdit.texFlags, sizeof(m_rsEdit.texFlags)))
    {
        ID3D11SamplerState *ss[eGFX_MAXTEX];

        for (eU32 j=0; j<eGFX_MAXTEX; j++)
        {
            const eInt tf = m_rsEdit.texFlags[j];
            for (i=0; i<m_samplerStates.size() && m_samplerStates[i].hash!=tf; i++);

            if (i >= m_samplerStates.size())
            {
                D3D11_SAMPLER_DESC desc;
                eMemSet(&desc, 0, sizeof(desc));
                desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                desc.MaxLOD = D3D11_FLOAT32_MAX;
                desc.MaxAnisotropy = 16;
                desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

                if (tf&eTMF_WRAPX)
                    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
                else if (tf&eTMF_MIRRORX)
                    desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
                else // default clamp
                    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;

                if (tf&eTMF_WRAPY)
                    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
                else if (tf&eTMF_MIRRORY)
                    desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
                else // default clamp
                    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;

                if (tf&eTMF_NEAREST)
                    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                else if (tf&eTMF_TRILINEAR)
                    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                else // default bilinear
                    desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;

                if (tf&eTMF_PCF)
                {
                    desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
                    desc.ComparisonFunc = D3D11_COMPARISON_LESS;
                }

                eStateInfoDx11 &si = m_samplerStates.append();
                si.hash = tf;
                si.rs = m_rsEdit;
                eCallDx(m_dev->CreateSamplerState(&desc, &si.d3dSs));
                i = m_samplerStates.size()-1;
            }

            ss[j] = m_samplerStates[i].d3dSs;
        }

        m_devCtx->PSSetSamplers(0, eGFX_MAXTEX, ss);
        m_devCtx->CSSetSamplers(0, eGFX_MAXTEX, ss);
    }

    // activate raster state
    if (m_rsActive.rasterHash != m_rsEdit.rasterHash)
    {
        for (i=0; i<m_rasterStates.size() && m_rasterStates[i].hash!=m_rsEdit.rasterHash; i++);

        if (i >= m_rasterStates.size())
        {
            static const eInt depthBiasVal[] = {0, 1500, -1500};

            D3D11_RASTERIZER_DESC desc;
            eMemSet(&desc, 0, sizeof(desc));
            desc.CullMode = (D3D11_CULL_MODE)(m_rsEdit.cullMode+1); // enums offset by 1
            desc.FillMode = (m_rsEdit.wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID);
            desc.DepthBias = depthBiasVal[m_rsEdit.depthBias];
            desc.DepthClipEnable = TRUE;
            desc.ScissorEnable = m_rsEdit.scissorTest;

            eStateInfoDx11 &si = m_rasterStates.append();
            si.hash = m_rsEdit.rasterHash;
            si.rs = m_rsEdit;
            eCallDx(m_dev->CreateRasterizerState(&desc, &si.d3dRs));
            i = m_rasterStates.size()-1;
        }

        m_devCtx->RSSetState(m_rasterStates[i].d3dRs);
    }

    // set scissor rect
    if (m_rsActive.scissorRect != m_rsEdit.scissorRect)
    {
        D3D11_RECT r;
        r.left = m_rsEdit.scissorRect.left;
        r.top = m_rsEdit.scissorRect.top;
        r.bottom = m_rsEdit.scissorRect.bottom;
        r.right = m_rsEdit.scissorRect.right;
        m_devCtx->RSSetScissorRects(1, &r);
    }

    // set viewport
    if (!eMemEqual(&m_rsActive.viewport, &m_rsEdit.viewport, sizeof(m_rsEdit.viewport)))
    {
        eRect vp = m_rsEdit.viewport;

        if (!vp.right)
            vp.right = vp.left+m_wndWidth;
        if (!vp.bottom)
            vp.bottom = vp.top+m_wndHeight;

        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = (eF32)vp.left;
        viewport.TopLeftY = (eF32)vp.top;
        viewport.Width = (eF32)vp.getWidth();
        viewport.Height = (eF32)vp.getHeight();
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_devCtx->RSSetViewports(1, &viewport);
    }
   
    // activate shaders
    if (m_rsActive.ps != m_rsEdit.ps)
        m_devCtx->PSSetShader(m_rsEdit.ps ? m_rsEdit.ps->d3dPs : nullptr, nullptr, 0);
    if (m_rsActive.vs != m_rsEdit.vs)
        m_devCtx->VSSetShader(m_rsEdit.vs ? m_rsEdit.vs->d3dVs : nullptr, nullptr, 0);
    if (m_rsActive.gs != m_rsEdit.gs)
        m_devCtx->GSSetShader(m_rsEdit.gs ? m_rsEdit.gs->d3dGs : nullptr, nullptr, 0);
    if (m_rsActive.cs != m_rsEdit.cs)
        m_devCtx->CSSetShader(m_rsEdit.cs ? m_rsEdit.cs->d3dCs : nullptr, nullptr, 0);

    // check if textures are used as targets or
    // they other way around
#ifdef eDEBUG
    for (eU32 i=0; i<eGFX_MAXTEX; i++)
    {
        for (eU32 j=0; j<eGFX_MAXMRT; j++)
            eASSERT(!m_rsEdit.textures[i] || m_rsEdit.textures[i] != m_rsEdit.targets[j]);

        for (eU32 j=0; j<eGFX_MAXUAVS; j++)
            if (m_rsEdit.uavBufs[j])
                eASSERT(!m_rsEdit.textures[i] || m_rsEdit.textures[i] != m_rsEdit.uavBufs[j]->tex);
    }
#endif

    // determine order of how to set targets/UAVs and texture,
    // in order to avoid D3D11 warning of still bound targets
    // when setting textures/UAVs
    eBool targetsUavsFirst = eFALSE;
    for (eU32 i=0; i<eGFX_MAXTEX; i++)
    {
        const eITexture *tex = m_rsEdit.textures[i];
        if (tex)
        {
            targetsUavsFirst |= (tex == m_rsActive.depthTarget);

            for (eU32 j=0; j<eGFX_MAXMRT; j++)
                targetsUavsFirst |= (tex == m_rsActive.targets[j]);
            for (eU32 j=0; j<eGFX_MAXUAVS; j++)
                targetsUavsFirst |= (m_rsActive.uavBufs[j] && tex == m_rsActive.uavBufs[j]->tex);
        }
    }

    if (targetsUavsFirst)
    {
        _activateTargets();
        _activateUavBuffers();
        _activateTextures();
    }
    else
    {
        _activateTextures();
        _activateUavBuffers();
        _activateTargets();
    }

    // replace active state
    m_rsActive = m_rsEdit;
}

eIShaderDx11 * eGraphicsDx11::_findShader(const eChar *src, const eChar *define)
{
    const eU32 hash = eHashPtr(src)^eHashPtr(define);
    
    for (eU32 i=0; i<m_shaders.size(); i++)
        if (m_shaders[i]->hash == hash)
            return m_shaders[i];

    return nullptr;
}

void eGraphicsDx11::_compileShader(const eChar *src, const eChar *define, const eChar *sm, eByteArray &data) const
{
    ID3DBlob *errMsg = nullptr;
    ID3DBlob *binary = nullptr;
    D3D_SHADER_MACRO d3dDefines[2];
    HRESULT res;

    eMemSet(d3dDefines, 0, sizeof(d3dDefines));
    if (define)
    {
        d3dDefines[0].Name = define;
        d3dDefines[0].Definition = 0;
    }

#ifdef eDEBUG
    WCHAR fileNameW[1024];
    MultiByteToWideChar(CP_ACP, 0, src, -1, fileNameW, 1024);

    res = D3DCompileFromFile(fileNameW, d3dDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                             "main", sm, D3DCOMPILE_PACK_MATRIX_ROW_MAJOR|D3DCOMPILE_SKIP_OPTIMIZATION, 0, &binary, &errMsg);
#else
    res = D3DCompile(src, eStrLength(src), nullptr, d3dDefines, &m_ShaderIncludeHandler,
		             "main", sm, D3DCOMPILE_PACK_MATRIX_ROW_MAJOR|D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &binary, &errMsg);
#endif

    if (FAILED(res))
    {
        if (errMsg)  {
#ifdef eEDITOR
			eWriteToLog((eChar *)errMsg->GetBufferPointer());
#endif
            eShowError((eChar *)errMsg->GetBufferPointer());
		}
        else
            eShowError(eString("Couldn't load shader \"")+src+"\"!");


#ifdef eDEBUG
        return; // shader editing => chance to fix syntax error
#else
        eFatal(-1); // no editing => quit, cause shader cannot be fixed
#endif
    }

    data.resize(binary->GetBufferSize());
    eMemCopy(&data[0], binary->GetBufferPointer(), binary->GetBufferSize());
}

void eGraphicsDx11::_loadShader(const eChar *src, const eChar *define, eIShaderDx11 *shader)
{
    static const eChar SM[9][7] = {"", "ps_5_0", "vs_5_0", "", "gs_5_0", "", "", "", "cs_5_0"};
    eByteArray data;

    _compileShader(src, define, SM[shader->type], data);    

    if (!data.isEmpty())
    {
        shader->hash = eHashPtr(src)^eHashPtr(define);
#ifdef eDEBUG
        shader->lastTime = getFileChangedTime(src);
        shader->filePath = src;
        shader->define = define;
#endif
        eIShader oldShader = *shader;

        switch (shader->type)
        {
        case eST_PS:
            eCallDx(m_dev->CreatePixelShader(&data[0], data.size(), nullptr, &shader->d3dPs));
            break;
        case eST_VS:
            eCallDx(m_dev->CreateVertexShader(&data[0], data.size(), nullptr, &shader->d3dVs));
            break;
        case eST_GS:
            eCallDx(m_dev->CreateGeometryShader(&data[0], data.size(), nullptr, &shader->d3dGs));
            break;
        case eST_CS:
            eCallDx(m_dev->CreateComputeShader(&data[0], data.size(), nullptr, &shader->d3dCs));
            break;
        }

        // reset shader if loading unsuccessful
        if (!shader->d3dPs) // PS, VS, GS and CS are in a union
            *shader = oldShader;
    }
}