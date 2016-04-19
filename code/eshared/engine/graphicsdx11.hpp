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

#ifndef GRAPHICS_DX11_HPP
#define GRAPHICS_DX11_HPP

struct ID3D11Device;
struct ID3D11Buffer;
struct ID3D11UnorderedAccessView;
struct ID3D11Query;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct ID3D11Texture3D;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11ComputeShader;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3D11InputLayout;
struct ID3D11ShaderResourceView;
struct ID3D11Resource;
struct ID3D11DeviceChild;
struct IDXGIFactory;
struct IDXGIFactory1;
struct IDXGIAdapter;
struct IDXGIAdapter1;
struct IDXGIOutput;
struct IDXGISwapChain;

//   debug: load text shaders from file (for live editing),
// release: load binary shaders from arrays (for size)

#define eSHADERCONTENT(name, suffix)           eString(eTOKENPASTE(name, _hlsl##suffix))

#ifdef eDEBUG
#define eSHADER(name)           "../code/eshared/engine/shaders/"#name".hlsl"
#else
#ifdef eEDITOR
#define eSHADER(name)           eTOKENPASTE(name, _hlsl)
#else
#define eSHADER(name)           eEngine::demodata_shader_indices[eSHADER_IDX_##name]
#endif
#endif

// every shader type is just a normal shader
// (contains all D3D interfaces in a union)
typedef struct eIShaderDx11 ePixelShaderDx11;
typedef struct eIShaderDx11 eVertexShaderDx11;
typedef struct eIShaderDx11 eGeometryShaderDx11;
typedef struct eIShaderDx11 eComputeShaderDx11;

// callback to fill dynamic geometry buffers
typedef void (* eGeoFillCallback)(struct eGeometryDx11 *geo, ePtr param);

struct eITextureDx11
{
    ID3D11ShaderResourceView *  d3dSrv;
};

struct eTexture2dDx11 : public eITextureDx11
{
    eU32                        width;
    eU32                        height;
    eU32                        pixelSize;
    eU32                        mipLevels;
    eBool                       target;
    eBool                       dynamic;
    eBool                       mipmaps;
    eTextureFormat              format;
    ID3D11Texture2D *           d3dTex;
    ID3D11Texture2D *           d3dReadTex;
    ID3D11DepthStencilView *    d3dDsv;
    ID3D11RenderTargetView *    d3dRtv;
};

struct eTexture3dDx11 : public eITextureDx11
{
    eU32                        width;
    eU32                        height;
    eU32                        depth;
    eU32                        pixelSize;
    eU32                        mipLevels;
    eBool                       dynamic;
    eBool                       mipmaps;
    eTextureFormat              format;
    ID3D11Texture3D *           d3dTex;
};

struct eTextureCubeDx11 : public eITextureDx11
{
    eU32                        size;
    eU32                        pixelSize;
    eBool                       target;
    eBool                       dynamic;
    eTextureFormat              format;
    ID3D11Texture2D *           d3dTex;
    eTexture2dDx11              faces[eCMF_COUNT];
};

struct eUavBufferDx11
{
    eTexture2dDx11 *            tex;
    ID3D11UnorderedAccessView * d3dUav;
};

struct eGeoBufferDx11
{
    eBool                       isVb;
    eU32                        size;
    eU32                        pos;
    eU32                        allocs;
    ID3D11Buffer *              d3dBuf;
};

struct eIShaderDx11
{
    eShaderType                 type;
    eU32                        hash;
#ifdef eDEBUG
    eString                     define;
    eString                     filePath;
    eS64                        lastTime;
#endif

    union
    {
        ID3D11PixelShader *     d3dPs;
        ID3D11VertexShader *    d3dVs;
        ID3D11GeometryShader *  d3dGs;
        ID3D11ComputeShader *   d3dCs;
    };
};

struct eGeometryDx11
{
    eBool                       loading;
    eBool                       indexed;
    eBool                       dynamic;
    eGeoPrimitiveType           primType;
    eVertexType                 vtxType;
    eU32                        vtxSize;
    eU32                        idxSize;
    eU32                        maxVerts;
    eU32                        maxIndices;
    eU32                        usedVerts;
    eU32                        usedIndices;
    eU32                        vbPos;
    eU32                        ibPos;
    eGeoFillCallback            fillCb;
    ePtr                        fcParam;
    eGeoBufferDx11 *            vb;
    eGeoBufferDx11 *            ib;
};

struct eIConstBufferDx11
{
    ePtr                        dataPtr;
    eU32                        size;
    eInt                        shaders;
};

template<class TYPE, eInt SHADERS> struct eConstBufferDx11 : public eIConstBufferDx11
{
    eConstBufferDx11()
    {
        dataPtr = &data;
        size    = sizeof(data);
        shaders = SHADERS;
    }

    TYPE                        data;
};

#pragma pack(push, 1)
struct eRenderStateDx11
{
    union
    {
        struct // 5 byte
        {
            eDepthFunc          depthFunc       : 4; // new 4 byte
            eBool               depthTest       : 2; // new 1 byte
            eBool               depthWrite      : 2;
        };

        eU64                    depthHash       : 64;
    };

    union
    {
        struct // 5 byte
        {
            eCullMode           cullMode        : 3; // new 4 byte
            eDepthBias          depthBias       : 3;
            eBool               scissorTest     : 2; // new 1 byte
            eBool               wireframe       : 2;
        };

        eU64                    rasterHash      : 64;
    };

    union
    {
        struct // 5 byte
        {
            eBlendMode          blendSrc        : 5; // new 4 byte
            eBlendMode          blendDst        : 5;
            eBlendOp            blendOp         : 4;
            eBool               blending        : 2; // new 1 byte
            eBool               colorWrite      : 2;
        };

        eU64                    blendHash       : 64;
    };

    eUavBufferDx11 *            uavBufs[eGFX_MAXUAVS];
    eITextureDx11 *             textures[eGFX_MAXTEX];
    eInt                        texFlags[eGFX_MAXTEX];
    eTexture2dDx11 *            targets[eGFX_MAXMRT];
    eTexture2dDx11 *            depthTarget;
    eRect                       viewport;
    eRect                       scissorRect;
    ePixelShaderDx11 *          ps;
    eVertexShaderDx11 *         vs;
    eGeometryShaderDx11 *       gs;
    eComputeShaderDx11 *        cs;
    eIConstBufferDx11 *         constBufs[eGFX_MAXCBS];
};
#pragma pack(pop)

struct eStateInfoDx11
{
    union
    {
      ID3D11DepthStencilState * d3dDss;
        ID3D11RasterizerState * d3dRs;
        ID3D11SamplerState *    d3dSs;
        ID3D11BlendState *      d3dBs;
    };

    eU64                        hash;
    eRenderStateDx11            rs;
};

class eGraphicsDx11
{
public:
    static eTexture2dDx11 *     TARGET_SCREEN;

public:
    eGraphicsDx11();
    ~eGraphicsDx11();

    void                        initialize();
    void                        shutdown();
    void                        openWindow(eU32 width, eU32 height, eInt windowFlags=0, ePtr hwnd=nullptr);
    void                        setWindowTitle(const eString &title);
    void                        handleMessages(eMessage &msg);
    void                        resizeBackbuffer(eU32 width, eU32 height);
    void                        clear(eInt clearMode, const eColor &col);
    void                        beginFrame();
    void                        endFrame();

#ifdef eDEBUG
    void                        reloadEditedShaders();
#endif

    eU32                        getResolutionCount() const;
    const eSize &               getResolution(eU32 index) const;
#ifdef eEDITOR
    const eRenderStats &        getRenderStats() const;
    const eEngineStats &        getEngineStats() const;
#endif
    eBool                       getFullScreen() const;
    eU32                        getWndWidth() const;
    eU32                        getWndHeight() const;
    eSize                       getWndSize() const;

    void                        setMatrices(const eMatrix4x4 &modelMtx, const eMatrix4x4 &viewMtx, const eMatrix4x4 &projMtx);
    const eMatrix4x4 &          getViewMatrix() const;
    const eMatrix4x4 &          getModelMatrix() const;
    const eMatrix4x4 &          getProjMatrix() const;
    void                        getBillboardVectors(eVector3 &right, eVector3 &up, eVector3 *view=nullptr);

    eRenderStateDx11 &          pushRenderState();
    eRenderStateDx11 &          popRenderState();
    eRenderStateDx11 &          freshRenderState();
    eRenderStateDx11 &          getRenderState();

    eGeometryDx11 *             addGeometry(eInt flags, eVertexType vtxType, eGeoPrimitiveType primType, eGeoFillCallback fillCb=nullptr, ePtr fcParam=nullptr);
    void                        removeGeometry(eGeometryDx11 *&geo);
    void                        beginLoadGeometry(eGeometryDx11 *geo, eU32 vertexCount, ePtr *vertices, eU32 indexCount=0, ePtr *indices=nullptr);
    void                        endLoadGeometry(eGeometryDx11 *geo, eInt vertexCount=-1, eInt indexCount=-1);
    void                        renderGeometry(eGeometryDx11 *geo, const eArray<eInstVtx> &insts=eArray<eInstVtx>());

    eTexture2dDx11 *            addTexture2d(eU32 width, eU32 height, eInt flags, eTextureFormat format);
    eTexture3dDx11 *            addTexture3d(eU32 width, eU32 height, eU32 depth, eInt flags, eTextureFormat format);
    eTextureCubeDx11 *          addTextureCube(eU32 size, eInt flags, eTextureFormat format);
    void                        removeTexture2d(eTexture2dDx11 *&tex);
    void                        removeTexture3d(eTexture3dDx11 *&tex);
    void                        removeTextureCube(eTextureCubeDx11 *&tex);
    void                        updateTexture2d(eTexture2dDx11 *tex, eConstPtr data);
    void                        updateTexture2d(eTexture2dDx11 *dst, eTexture2dDx11 *src, const ePoint &dstPos, const eRect &srcRegion);
    void                        updateTexture3d(eTexture3dDx11 *tex, eConstPtr data);
    void                        updateTextureCube(eTextureCubeDx11 *tex, eConstPtr data, eCubeMapFace face);
    void                        readTexture2d(eTexture2dDx11 *tex, eArray<eColor> &texData) const;

    eUavBufferDx11 *            addUavBuffer(eU32 width, eU32 height, eTextureFormat format);
    void                        removeUavBuffer(eUavBufferDx11 *&uav);
    void                        execComputeShader(eU32 numThreadsX, eU32 numThreadsY, eU32 numThreadsZ);

    ePixelShaderDx11 *          loadPixelShader(const eChar *src, const eChar *define=nullptr);
    eVertexShaderDx11 *         loadVertexShader(const eChar *src, const eChar *define=nullptr);
    eGeometryShaderDx11 *       loadGeometryShader(const eChar *src, const eChar *define=nullptr);
    eComputeShaderDx11 *        loadComputeShader(const eChar *src, const eChar *define=nullptr);
    void                        removeShader(eIShaderDx11 *&shader);

    eTexture2dDx11 *            createChessTexture(eU32 width, eU32 height, eU32 step, const eColor &col0, const eColor &col1);

private:
    ePtr                        _createWindow(eU32 width, eU32 height, eBool fullScreen);
    void                        _createDeviceAndSwapChain();
    eU32                        _getAvailableGpuMemory();
    void                        _createInputLayouts();
    void                        _createRenderTargetView();
    void                        _createDepthStencilView();
    void                        _createDynamicBuffers();
    eGeoBufferDx11 *            _createGeoBuffer(eBool isVb, eU32 size, eBool dynamic);
    void                        _activateConstBuffers();
    void                        _activateTargets();
    void                        _activateTextures();
    void                        _activateUavBuffers();
    void                        _activateRenderState();
    eIShaderDx11 *              _findShader(const eChar *src, const eChar *define);
    void                        _compileShader(const eChar *src, const eChar *define, const eChar *sm, eByteArray &data) const;
    void                        _loadShader(const eChar *src, const eChar *define, eIShaderDx11 *shader);

private:
    enum GeoBufferId
    {
        GBID_VB_DYN,
        GBID_IB_DYN16,
        GBID_IB_DYN32,
        GBID_IB_QUAD,
        GBID_VB_INST,
        GBID_BUFS_USER
    };

    enum GeoBufferElements
    {
        GBE_IB_QUAD             = 0x8000*6,     // 32768 quads
        GBE_VB_INST             = 65536
    };

    enum GeoBufferSize
    {
        GBS_VB_INST             = GBE_VB_INST*sizeof(eInstVtx), // 6.25 MB
        GBS_VB_DYN              = 24*1024*1024, // 16 MB
        GBS_IB_DYN              = 8*1024*1024,  // 6 MB
        GBS_IB_QUAD             = GBE_IB_QUAD*sizeof(eU16)
    };

    struct BufferData
    {
        ID3D11Buffer *          d3dBuf;
        eU32                    size;
        eBool                   inUse;
    };

private:
    static const int FRAMES = 2;

    ID3D11Query *               m_occlQuery[FRAMES];
    eBool                       m_startPull;
    eU32                        m_frameNum;

    eArray<BufferData>          m_cbufs;
    eArray<eTexture2dDx11 *>    m_texs2d;
    eArray<eTexture3dDx11 *>    m_texs3d;
    eArray<eTextureCubeDx11 *>  m_texsCube;
    eArray<eUavBufferDx11 *>    m_uavBufs;
    eArray<eGeometryDx11 *>     m_geos;
    eArray<eGeoBufferDx11 *>    m_geoBufs;
    eArray<eIShaderDx11 *>      m_shaders;
    eArray<eStateInfoDx11>      m_depthStates;
    eArray<eStateInfoDx11>      m_rasterStates;
    eArray<eStateInfoDx11>      m_samplerStates;
    eArray<eStateInfoDx11>      m_blendStates;
    eArray<eRenderStateDx11>    m_rsStack;
    eRenderStateDx11            m_rsActive;
    eRenderStateDx11            m_rsEdit;
    eByteArray                  m_geoMapData[2];
    eArray<eSize>               m_resolutions;
#ifdef eEDITOR
    eRenderStats                m_renderStats;
    eEngineStats                m_engineStats;
#endif
    IDXGIFactory1 *             m_dxgiFactory;
    IDXGIAdapter1 *             m_adapter;
    IDXGIOutput *               m_adapterOutput;
    IDXGISwapChain *            m_swapChain;
    ID3D11Device *              m_dev;
    ID3D11DeviceContext *       m_devCtx;
    ID3D11RenderTargetView *    m_rtvScreen;
    ID3D11Texture2D *           m_dsScreen;
    ID3D11DepthStencilView *    m_dsvScreen;
    ID3D11DepthStencilView *    m_dsvActive;
    ID3D11RenderTargetView *    m_rtvsActive[eGFX_MAXMRT];
    eArray<ID3D11InputLayout *> m_inputLayouts;
    ePtr                        m_hwnd;
    eBool                       m_ownWindow;
    eBool                       m_fullScreen;
    eBool                       m_vsync;
    eU32                        m_wndWidth;
    eU32                        m_wndHeight;
    eMatrix4x4                  m_modelMtx;
    eMatrix4x4                  m_viewMtx;
    eMatrix4x4                  m_projMtx;

	/*
public:
	int g_numAddsTex2d;
	int g_numAddsTex3d;
	int g_numAddsTexCube;
	int g_numRemsTex2d;
	int g_numRemsTex3d;
	int g_numRemsTexCube;
	int g_numAddsStaticGeo;
	int g_numRemsStaticGeo;
	*/
};

#define eGraphics               eGraphicsDx11
#define eITexture               eITextureDx11
#define eTexture2d              eTexture2dDx11
#define eTexture3d              eTexture3dDx11
#define eTextureCube            eTextureCubeDx11
#define eGeometry               eGeometryDx11
#define eIConstBuffer           eIConstBufferDx11
#define eConstBuffer            eConstBufferDx11
#define eIShader                eIShaderDx11
#define ePixelShader            ePixelShaderDx11
#define eVertexShader           eVertexShaderDx11
#define eGeometryShader         eGeometryShaderDx11
#define eComputeShader          eComputeShaderDx11
#define eRenderState            eRenderStateDx11
#define eUavBuffer              eUavBufferDx11

#endif // GRAPHICS_DX11_HPP