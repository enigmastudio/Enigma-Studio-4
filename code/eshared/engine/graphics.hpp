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

#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

enum eGraphicsConsts
{
    eGFX_MAXUAVS = 8,
    eGFX_MAXTEX = 10,
    eGFX_MAXMRT = 5,
    eGFX_MAXCBS = 6,
};

enum eClearMode
{
    eCM_COLOR   = 1,
    eCM_DEPTH   = 2,
    eCM_STENCIL = 4,
    eCM_ALL     = eCM_COLOR|eCM_DEPTH|eCM_STENCIL,
};

enum eCullMode
{
    eCULL_NONE,
    eCULL_FRONT,
    eCULL_BACK
};

enum eDepthFunc
{
    eDF_NEVER,
    eDF_LESS,
    eDF_EQUAL,
    eDF_LEQUAL,
    eDF_GREATER,
    eDF_NOTEQUAL,
    eDF_GEQUAL,
    eDF_ALWAYS
};

enum eFogType
{
    eFOG_LINEAR,
    eFOG_EXP,
    eFOG_EXPSQR
};

enum eBlendMode
{
    eBM_ZERO,
    eBM_ONE,
    eBM_SRCCOLOR,
    eBM_INVSRCCOLOR,
    eBM_SRCALPHA,
    eBM_INVSRCALPHA,
    eBM_DSTALPHA,
    eBM_INVDSTALPHA,
    eBM_DSTCOLOR,
    eBM_INVDSTCOLOR,
};

enum eBlendOp
{
    eBO_ADD,
    eBO_SUB,
    eBO_INVSUB,
    eBO_MIN,
    eBO_MAX
};

enum eTextureMaterialFlags
{
    eTMF_NEAREST   = 1, // filtering
    eTMF_BILINEAR  = 2,
    eTMF_TRILINEAR = 4,
    eTMF_PCF       = 8, // hardware PCF
    eTMF_WRAPX     = 16, // addressing
    eTMF_WRAPY     = 32,
    eTMF_CLAMPX    = 64, 
    eTMF_CLAMPY    = 128,
    eTMF_MIRRORX   = 256,
    eTMF_MIRRORY   = 512,
    eTMF_WRAP      = eTMF_WRAPX|eTMF_WRAPY,
    eTMF_CLAMP     = eTMF_CLAMPX|eTMF_CLAMPY,
    eTMF_MIRROR    = eTMF_MIRRORX|eTMF_MIRRORY,
};

enum eTextureFormat
{
    eTFO_ARGB8,
    eTFO_ARGB16,
    eTFO_ARGB16F,
    eTFO_DEPTH32F,
    eTFO_R16F,
    eTFO_R32F,
    eTFO_GR16F,
    eTFO_GR32F,
    eTFO_R8,
};

enum eTextureFlags
{
    eTEX_DYNAMIC     = 1,
    eTEX_NOMIPMAPS   = 2,
    eTEX_TARGET      = 4,
    eTEX_UAV         = 8,
};

enum eMessage
{
    eMSG_BUSY,
    eMSG_IDLE,
    eMSG_QUIT
};

enum eConstBufferIndex
{
    eCBI_CAMERA,
    eCBI_LIGHT,
    eCBI_MATERIAL,
    eCBI_FX_PARAMS,
    eCBI_PASS_AMBIENT,
    eCBI_PASS_SHADOW,
};

enum eShaderType
{
    eST_PS    = 1,
    eST_VS    = 2,
    eST_GS    = 4,
    eST_CS    = 8,
    eST_COUNT = 4,
};

enum eVertexType
{
    eVTX_FULL,
    eVTX_SIMPLE,
    eVTX_PARTICLE,
    eVTX_INSTANCE,
};

enum eCubeMapFace
{
    eCMF_POSX,
    eCMF_NEGX,
    eCMF_POSY,
    eCMF_NEGY,
    eCMF_POSZ,
    eCMF_NEGZ,
    eCMF_COUNT
};

enum eDepthBias
{
    eDB_NONE,
    eDB_BEFORE,
    eDB_BEHIND
};

enum eWindowFlags
{
    eWF_FULLSCREEN = 0x01,
    eWF_VSYNC      = 0x02,
};

enum eGeometryFlags
{
    eGEO_STATIC  = 1,
    eGEO_DYNAMIC = 2,
    eGEO_IB16    = 4,
    eGEO_IB32    = 8,
};

enum eGeoPrimitiveType
{
    eGPT_TRILIST,
    eGPT_QUADLIST,
    eGPT_LINELIST,
    eGPT_SPRITELIST, // billboards parallel to view plane
    eGPT_TRISTRIPS,
    eGPT_LINESTRIPS,
};

enum eMouseButton
{
    eMB_NONE,
    eMB_LEFT,
    eMB_MIDDLE,
    eMB_RIGHT
};

struct eRenderStats
{
    eU32        batches;
    eU32        instances;
    eU32        triangles;
    eU32        vertices;
    eU32        lines;
};

struct eEngineStats
{
    eU32        numTex2d;
    eU32        numTex3d;
    eU32        numTexCube;
    eU32        numGeos;
    eU32        numGeoBuffs;
    eU32        numShaders;
    eU32        numStates;

    eU32        availGpuMem;
    eU32        usedGpuMem;
    eU32        usedGpuMemTex;
    eU32        usedGpuMemGeo;
};

struct eUserInput
{
    ePoint      mousePos;
    ePoint      mouseDelta;
    eInt        mouseBtns;
};

struct eFullVtx
{
    void set(const eVector3 &newPos, const eVector3 &newNormal, const eVector2 &newUv, const eColor &newCol)
    {
        pos = newPos;
        normal = newNormal;
        uv = newUv;
        col = newCol;
    }

    eFXYZ       pos;
    eFXYZ       normal;
    eFXY        uv;
    eColor      col;
};

struct eSimpleVtx
{
    void set(const eVector3 &newPos, const eVector2 &newUv)
    {
        pos = newPos;
        uv = newUv;
    }

    eFXYZ       pos;
    eFXY        uv;
};

struct eParticleVtx
{
    void set(const eVector3 &newPos, const eVector2 &newUv, const eColor &newCol)
    {
        pos = newPos;
        uv = newUv;
        col = newCol;
    }

    eFXYZ       pos;
    eFXY        uv;
    eColor      col;
};

struct eInstVtx
{
    eMatrix4x4  modelMtx;
    eMatrix3x3  normalMtx;
};

const eU32 eVERTEX_SIZES[] =
{
    sizeof(eFullVtx),
    sizeof(eSimpleVtx),
    sizeof(eParticleVtx),
    sizeof(eInstVtx)
};

#endif // GRAPHICS_HPP