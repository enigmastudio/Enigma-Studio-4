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

#ifndef EFFECT_HPP
#define EFFECT_HPP

#ifdef eEDITOR
#define eSHADERPTR(DEF)	_addShader(#DEF, eSHADERCONTENT(DEF,))
#define eEFFECT_DEFINE_CONSTRUCTOR(OPNAME)	OPNAME()
#define eEFFECT_MAKE_CONSTRUCTOR(OPNAME, DEF)									\
	eREGISTER_EFFECT_SHADER(#OPNAME, DEF)										\
	OPNAME::OPNAME() { eSHADERPTR(DEF); }
struct eEFFECT_SHADER_ENTRY {
	eString name;
	eString shaderContent;
};
#define eEFFECT_MAKE_EMPTY_CONSTRUCTOR(OPNAME)									\
	OPNAME::OPNAME() { }
#else
#define eSHADERPTR(DEF)
#define eEFFECT_DEFINE_CONSTRUCTOR(OPNAME)	OPNAME()
//#define eEFFECT_MAKE_CONSTRUCTOR(OPNAME, DEF) OPNAME::OPNAME() { eGfx->loadPixelShader(eSHADER(DEF)); }
#define eEFFECT_MAKE_CONSTRUCTOR(OPNAME, DEF) OPNAME::OPNAME() { eIEffect::addPixelShader(eSHADER(DEF)); }
#define eEFFECT_MAKE_EMPTY_CONSTRUCTOR(OPNAME)									
#endif

class eIEffect
{
    friend class eMergeEffect;

public:
    eIEffect();

    void                run(eF32 time, eTexture2d *target, eTexture2d *depthTarget);
    void                addInput(eIEffect *fx);
	void                addOutput(eIEffect *fx);
    void                clearInputs();
    static void         shutdown();

#ifdef eEDITOR
	eArray<eEFFECT_SHADER_ENTRY>	m_usedShaders;
	void							_addShader(const eString& str, const char* ptr);
#else
	static eArray<const eChar*>		m_pixelShadersToLoad;
	static void						addPixelShader(const eChar* src);
	static void						loadPostponedPixelShaders();
	static eBool					m_doPostponeShaderLoading;
#endif

public:
    struct Target
    {
        eTexture2d *    colTarget;
		eTexture2d *	depthTarget;
        eU32            refCount;
        eTextureFormat  format;
        eBool           wasUsed;
		ePtr			owner;
    };

    typedef eArray<Target *> TargetPtrArray;
    typedef eArray<Target> TargetArray;

protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &wndSize) = 0;
	Target *			_findTarget(ePtr owner);
    Target *            _acquireTarget(const eSize &size, const eCamera *cam = nullptr, eTextureFormat format=eTFO_ARGB8);
    void                _releaseTarget(Target *target);
    eRect               _viewportToRect(const eVector4 &vp, const eSize &size);
    void                _copyTarget(Target *dst, const eRect &dstRect, Target *src);
    void                _renderQuad(eTexture2d *colTarget, eTexture2d *depthTarget, eTexture2d *tex0, eTexture2d *tex1=nullptr) const;
    Target *            _renderSimple(Target *src, ePixelShader *ps, eTextureFormat format=eTFO_ARGB8);

private:
    Target *            _runHierarchy(eF32 time, const eSize &targetSize);
    void                _garbageCollectTargets();

protected:
    eVector4            m_viewport;
    eArray<eIEffect *>  m_inputFx;
	eArray<eIEffect *>  m_outputFx;

private:
    static TargetArray  m_targetPool;
};

class eIIterableEffect : public eIEffect
{
public:
    eIIterableEffect(eU32 iters=1);

    void                setIterations(eU32 iters);
    eU32                getIterations() const;

protected:
    Target *            _renderPingPong(Target *src, Target *dst) const;

protected:
    eU32                m_iters;
};

class eInputEffect : public eIEffect
{
public:
    eInputEffect(const eScene &scene, const eCamera &cam, const eVector4 &viewport);

protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

private:
    const eScene *      m_scene;
    eCamera             m_cam;
};

class eDistortEffect : public eIEffect
{
public:
	eEFFECT_DEFINE_CONSTRUCTOR(eDistortEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eVector2        intensity;
        eVector2        offset;
    };

public:
    ShaderConsts        sc;
    eTexture2d *        distMap;
};

class eAdjustEffect : public eIIterableEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eAdjustEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eVector4        brightnessAdjCol;
        eVector4        subCol;
        eVector4        addCol;
        eF32            contrast;
    }
    sc;
};

class eBlurEffect : public eIIterableEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eBlurEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
	struct ShaderConsts
	{
		eVector2        dist;
	} sc;
};

class eDofEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eDofEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eF32            focalDepth;
        eF32            nearDofStart; 
        eF32            nearDofDist;
        eF32            farDofStart;
        eF32            farDofDist; 
        eF32            maxBlur; 
        eF32            threshBlur;
        eInt            rings; 
        eInt            ringSamples; 
        eInt            vignettingActive;
        eF32            vignettingOut;
        eF32            vignettingIn; 
        eF32            vignettingFade;
        eF32            highlightingThreshold; 
        eF32            highlightingGain;
        eInt            noiseActive;
        eF32            noiseAmount;
    }
    sc;
};

class eSsaoEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eSsaoEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eF32            scale;
        eF32            intensity;
        eF32            bias;
        eF32            radius;
        eF32            noiseAmount;
    }
    sc;
};

class eFxaaEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eFxaaEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);
};

class eColorGradingEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eColorGradingEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    eTexture2d *       lookupMap;
};

class eRadialBlurEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eRadialBlurEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eVector2        origin;
        eF32            dist;
        eF32            strength;
		eU32			steps;
    }
    sc;
};

class eFogEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eFogEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eFogType        type;
        eF32            start;
        eF32            end;
        eF32            density;
        eVector4        col;
    }
    sc;
};

class eDownsampleEffect : public eIIterableEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eDownsampleEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    eVector2            amount;
};

class eRippleEffect : public eIIterableEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eRippleEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    enum Mode
    {
        MODE_STANDARD,
        MODE_CONCENTRIC
    };

    struct ShaderConsts
    {
        eVector2    offset;
        eF32        ampli;
        eF32        length;
        eF32        speed;
        eF32        time;
        eU32        mode;
    }
    sc;
};

class eMicroscopeEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eMicroscopeEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eVector3        foregroundCol1;
		eVector3        foregroundCol2;
        eVector3        backgroundCol;
        eF32            depth;
        eF32            noiseAmount;
    }
    sc;
};

class eMotionBlurEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eMotionBlurEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eMatrix4x4      invViewMatrix;
        eMatrix4x4      lastViewMatrix;
        eMatrix4x4      projMatrix;
        eF32            strength;
    }
    sc;
};

class eHaloEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eHaloEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
		eVector4		color;
        eVector4		position;
        eVector2        size;
		eF32			power;
		eInt			occlusionTest;
    }
    sc;
};

class eCausticsEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eCausticsEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
		eMatrix4x4      invViewMatrix;
		eMatrix4x4      transViewMatrix;
		eF32			scale;
		eF32			time;
		eF32			amplitude;
    }
    sc;
};

class eDebugEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eDebugEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
		eVector2 depthRange;
    }
    sc;
};

class eInterferenceEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eInterferenceEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eVector2        moveR;
        eVector2        moveG;
        eVector2        moveB;
        eVector2        amount;
        eF32            time;
    }
    sc;
};

class eSpaceEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eSpaceEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eF32            param;
    }
    sc;
};

class eSaveEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eSaveEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    eTexture2d *       renderTarget;
    eTexture2d *       depthTarget;
};

class eMergeEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eMergeEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    enum Mode
    {
        MODE_ADD,
        MODE_SUB,
        MODE_MUL,
        MODE_BRIGTHER,
        MODE_DARKER,
        MODE_NONE
    };

    struct ShaderConsts
    {
        eVector4        viewport0;
        eVector4        viewport1;
        eVector4        clearCol;
        eInt            mergeMode;
        eVector2        blendRatios;
		eInt			depthTest;
    }
    sc;
};

// generates a raymarched terrain
class eTerrainEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eTerrainEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eVector3        camPosition;
		eVector3        camFov;
        eVector3        sunPosition;
        eVector3        sunColor;       
        eVector3        colSnow;
		eVector3        colRock;
        eVector3        colGrass;
        eVector3        colEarth;
        eVector3        terrainScale;
        eVector3        noiseScale;
        eVector3        skyColor;
        eVector3        planetCenter;
		eVector4		altitudeMax;
		eVector4		altitudeMin;
		eVector4		altitudeRel;
		eVector4		slopeMax;
		eVector4		slopeMin;
		eVector4		slopeRel;
		eVector4		skewAmo;
		eVector4		skewAzim;

		eVector3		uu;
		eVector3		vv;
		eVector3		ww;

        eF32            camRange;
        eF32            fogAmount;
        eF32            shadowAmount;
        eF32            planetRadius;
        eF32            terrainWarping;
        eF32            time;
        eF32            waterLevel;
        eF32            waterAmplitude;
        eF32            waterFrequency;
        eF32            waterShininess;

        eF32            skyLevel;
        eF32            skyHorizon;
        eF32            cloudScale;
        eF32            cloudTreshold;
    }
    sc;

public:
    eTexture2d *        noiseMap;
    eTexture2d *        terrainMap;
    eTexture2d *        grassTexture; 
    eTexture2d *        earthTexture; 
    eTexture2d *        rockTexture;
    eTexture2d *        foamTexture;
};

// generates a raymarched planet
class ePlanetEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(ePlanetEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eMatrix4x4      invViewMatrix;
		eVector3        camFov;
        eVector3        sunPosition;

        eVector3        shallowColor;
        eVector3        shoreColor;
        eVector3        sandColor;
        eVector3        grassColor;
        eVector3        dirtColor;

        eF32            planetRadius;
        eF32            planetHillTop;
        eF32            planetSeed;
        eF32            planetFreq;

        eF32            time;
        eF32            waterLevel;
        eF32            waterAmplitude;
        eF32            waterFrequency;
        eF32            waterTransparency;

        eF32            cloudScale;
        eF32            cloudTreshold;

        eF32            waterExtinction;
        eF32            snowFrequency;

        eF32            textureNoise;
        eF32            scatterStrength;
        eF32            fogAmount;
    }
    sc;
};

// generates a raytraced sphere
class eRaytraceEffect : public eIEffect
{
public:
    eEFFECT_DEFINE_CONSTRUCTOR(eRaytraceEffect);
protected:
    virtual Target *    _run(eF32 time, TargetPtrArray &srcs, const eSize &targetSize);

public:
    struct ShaderConsts
    {
        eMatrix4x4      invViewMatrix;
		eVector3        camFov;

        eF32            time;
        eF32            sphereRadius;
    }
    sc;
};

#endif // EFFECT_HPP