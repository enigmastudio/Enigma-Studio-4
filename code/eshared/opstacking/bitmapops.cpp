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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../eshared.hpp"

eDEF_OPERATOR_SOURCECODE(BITMAP);

eIBitmapOp::eIBitmapOp() :
    m_bmpWidth(m_res.width),
    m_bmpHeight(m_res.height),
    m_uav(m_res.uav)
{
    m_bmpWidth = 0;
    m_bmpHeight = 0;
    m_uav = nullptr;
}

eIBitmapOp::~eIBitmapOp()
{
    eGfx->removeUavBuffer(m_uav);
}

const eIBitmapOp::Result & eIBitmapOp::getResult() const
{
    return m_res;
}

eU32 eIBitmapOp::getResultSize() const
{
    return (m_uav ? m_uav->tex->width*m_uav->tex->height*m_uav->tex->pixelSize : 0);
}

void eIBitmapOp::freeResult()
{
    eGfx->removeUavBuffer(m_uav);
}

void eIBitmapOp::_preExecute()
{
    // bitmap operators always operate on
    // smallest input bitmap operator size
    if (getAboveOpCount() > 0)
    {
        eSize size(eS32_MAX, eS32_MAX);

        for (eU32 i=0; i<getAboveOpCount(); i++)
        {
            const Result &res = ((eIBitmapOp *)getAboveOp(i))->getResult();
            size.minComponents(eSize(res.uav->tex->width, res.uav->tex->height));
        }

        _reallocate(size.x, size.y);
    }
}

void eIBitmapOp::_reallocate(eU32 width, eU32 height)
{
    _allocateUav(width, height, m_uav);
    m_bmpWidth = width;
    m_bmpHeight = height;
}

void eIBitmapOp::_allocateUav(eU32 width, eU32 height, eUavBuffer *&uav) const
{
    eASSERT(eIsPowerOf2(width) && eIsPowerOf2(height));

    if (!uav || uav->tex->width != width || uav->tex->height != height)
    {
        eGfx->removeUavBuffer(uav);
        uav = eGfx->addUavBuffer(width, height, eTFO_ARGB8);
    }
}

void eIBitmapOp::_execCs(eComputeShader *cs, eInt texFlags0, eUavBuffer *dstUav,
                         eUavBuffer *srcUav, eIConstBuffer *cbExtra, const eSize &threadDim)
{
    // copy parameters to 1st constant buffer
    typedef eU8 CbParams[4*(20+40)];
    static eConstBuffer<CbParams, eST_CS> cbParams;
    eVector4 *dataPtr = (eVector4 *)cbParams.data;

    for (eU32 i=0, j=0; i<m_params.size(); i++)
    {
        const eParameter &p = *m_params[i];
        const eParamValue &animVal = p.getAnimValue();

        if (p.getClass() == ePC_COL)
            dataPtr[j++] = animVal.color;
        else if (p.getType() != ePT_LABEL) // labels are ignored
            eMemCopy(&dataPtr[j++], &animVal.flt, 4*p.getComponentCount());
    }

    // copy infos to 2nd constant buffer
    static eConstBuffer<eU32, eST_CS> cbInfos;
    cbInfos.data = m_aboveOps.size();

    // set render states
    eRenderState &rs = eGfx->freshRenderState();
    rs.constBufs[0] = &cbParams;
    rs.constBufs[1] = &cbInfos;
    rs.constBufs[2] = cbExtra;
    rs.uavBufs[0] = (dstUav ? dstUav : m_uav);
    rs.texFlags[0] = texFlags0;
    rs.cs = cs;

    if (!srcUav)
    {
        for (eU32 i=0, j=0; i<m_aboveOps.size(); i++)
            if (m_aboveOps[i]->getResultClass() == eOC_BMP)
                rs.textures[j++] = ((eIBitmapOp *)m_aboveOps[i])->getResult().uav->tex;
    }
    else
        rs.textures[0] = srcUav->tex;

    // execute compute shader
    eGfx->execComputeShader(threadDim.x ? threadDim.x : m_bmpWidth/8,
                            threadDim.y ? threadDim.y : m_bmpHeight/8, 1);
}

// Fill (bitmap) operator
// ----------------------
// Fills the whole bitmap with just one color.

#if defined(HAVE_OP_BITMAP_FILL) || defined(eEDITOR)
eOP_DEF_BMP(eFillOp, "Fill", 'f', 0, 0, eOP_INPUTS())
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eFillOp, cs_bmp_fill);
	}
 	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eFillOp, widthSel, "Width", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_ENUM(eFillOp, heightSel, "Height", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_RGBA(eFillOp, color, "Color", 0, 0, 0, 255,
		eOP_PAR_END))))
   {
        _reallocate(1<<widthSel, 1<<heightSel);
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_fill)));
    }
	eOP_EXEC2_END
eOP_END(eFillOp);
#endif

// Perlin noise (bitmap) operator
// ------------------------------
// Creates a tileable perlin noise bitmap.
#if defined(HAVE_OP_BITMAP_PERLIN) || defined(eEDITOR)
eOP_DEF_BMP(ePerlinOp, "Perlin", 'p', 0, 0, eOP_INPUTS())
	eOP_INIT() {
	   eREGISTER_OP_SHADER(ePerlinOp, cs_bmp_perlin);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(ePerlinOp, widthSel, "Width", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_ENUM(ePerlinOp, heightSel, "Height", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_INT(ePerlinOp, octaves, "Octaves", 1, 10, 7,
        eOP_PAR_INT(ePerlinOp, freqNum, "Frequency", 1, 8, 1,
        eOP_PAR_FLOAT(ePerlinOp, persis, "Persistence", 0.0f, 10.0f, 1.5f,
        eOP_PAR_FLOAT(ePerlinOp, amplify, "Amplify", 0.01f, 16.0f, 1.5f,
        eOP_PAR_RGBA(ePerlinOp, color0, "Color 0 ", 255, 255, 255, 255,
        eOP_PAR_RGBA(ePerlinOp, color1, "Color 1", 0, 0, 0, 255,
        eOP_PAR_INT(ePerlinOp, seed, "Seed", 0, 65535, 0,
		eOP_PAR_END))))))))))
    {
        _reallocate(1<<widthSel, 1<<heightSel);
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_perlin)));
    }
	eOP_EXEC2_END

eOP_END(ePerlinOp);
#endif

// Rect (bitmap) operator
// ----------------------
// Draws a colored rectangle on the bitmap.

#if defined(HAVE_OP_BITMAP_RECT) || defined(eEDITOR)
eOP_DEF_BMP(eRectOp, "Rect", ' ', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eRectOp, cs_bmp_rect);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eRectOp, start, "Rect start", 0.0f, 1.0f, 0.25f, 0.25f,
        eOP_PAR_FXY(eRectOp, end, "Rect end", 0.0f, 1.0f, 0.75f, 0.75f,
        eOP_PAR_RGBA(eRectOp, col, "Color", 255, 255, 255, 255,
		eOP_PAR_END))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_rect)));
    }
	eOP_EXEC2_END
eOP_END(eRectOp);
#endif

// Glow (bitmap) operator
// ----------------------
// Generates a glow bitmap, often used as
// base texture for particles.

#if defined(HAVE_OP_BITMAP_GLOW) || defined(eEDITOR)
eOP_DEF_BMP(eGlowOp, "Glow", 'g', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eGlowOp, cs_bmp_glow);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eGlowOp, relCenter, "Center", -4, 4, 0.5f, 0.5f,
        eOP_PAR_FXY(eGlowOp, relRadius, "Radius", 0, 4, 0.25f, 0.25f,
        eOP_PAR_RGBA(eGlowOp, col, "Color", 255, 255, 255, 255,
        eOP_PAR_INT(eGlowOp, alphaVal, "Alpha", 0, 255, 255,
        eOP_PAR_INT(eGlowOp, gammaVal, "Gamma", 0, 255, 128,
		eOP_PAR_END))))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_glow)));
    }
	eOP_EXEC2_END
eOP_END(eGlowOp);
#endif

// Merge (bitmap) operator
// -----------------------
// Merges multiple bitmap operators together.
// The merging mode can be selected.

#if defined(HAVE_OP_BITMAP_MERGE) || defined(eEDITOR)
eOP_DEF_BMP(eBitmapMergeOp, "Merge", 'm', 1, 4, eOP_INPUTS(eOP_4INPUTS(eOC_BMP)))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eBitmapMergeOp, cs_bmp_merge);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eBitmapMergeOp, mode, "Mode", "Add|Sub|Mul|Difference|Average|Minimum|Maximum", 0,
		eOP_PAR_END))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_merge)));
    }
	eOP_EXEC2_END

eOP_END(eBitmapMergeOp);
#endif

// Adjust (bitmap) operator
// ------------------------
// Adjusts brightness, contrast and hue and
// saturation (using HSV color space) of bitmap.

#if defined(HAVE_OP_BITMAP_ADJUST) || defined(eEDITOR)
eOP_DEF_BMP(eAdjustOp, "Adjust", 'a', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eAdjustOp, cs_bmp_adjust);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eAdjustOp, brightness, "Brightness", 0, 255.0f, 1.0f,
        eOP_PAR_FLOAT(eAdjustOp, contrast, "Contrast", 0.0f, 128.0f, 1.0f,
        eOP_PAR_FLOAT(eAdjustOp, hueVal, "Hue", 0.0f, 2.0f, 1.0f,
        eOP_PAR_FLOAT(eAdjustOp, saturation, "Saturation", 0.0f, 2.0f, 1.0f,
		eOP_PAR_END)))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_adjust)));
    }
	eOP_EXEC2_END
eOP_END(eAdjustOp);
#endif

// Sine plasma (bitmap) operator
// -----------------------------
// Creates a sine plasma.

#if defined(HAVE_OP_BITMAP_SINE_PLASMA) || defined(eEDITOR)
eOP_DEF_BMP(eSinePlasmaOp, "Sine plasma", 's', 0, 0, eOP_INPUTS())
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eSinePlasmaOp, cs_bmp_sineplasma);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eSinePlasmaOp, widthSel, "Width", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_ENUM(eSinePlasmaOp, heightSel, "Height", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_IXY(eSinePlasmaOp, count, "Count", 1, eU16_MAX, 3, 3,
        eOP_PAR_FXY(eSinePlasmaOp, shift, "Shift", eF32_MIN, eF32_MAX, 0.0f, 0.0f,
        eOP_PAR_RGBA(eSinePlasmaOp, plasmaCol, "Plasma color", 255, 255, 255, 255,
        eOP_PAR_RGBA(eSinePlasmaOp, bgCol, "Background color", 0, 0, 0, 255,
		eOP_PAR_END)))))))
    {
        _reallocate(1<<widthSel, 1<<heightSel);
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_sineplasma)));
    }
	eOP_EXEC2_END
eOP_END(eSinePlasmaOp);
#endif

// Blur (bitmap) operator
// ----------------------
// Applies n passes of vertical and horizontal
// box blur, which approximates a gaussian
// blur when choosing n=3.

#if defined(HAVE_OP_BITMAP_BLUR) || defined(eEDITOR)
eOP_DEF_BMP(eBlurOp, "Blur", 'b', 1, 1, eOP_INPUTS(eOC_BMP))
    eOP_INIT()
    {
        m_uavTemp = nullptr;
	   eREGISTER_OP_SHADER(eBlurOp, cs_bmp_blur);
    }

    eOP_DEINIT()
    {
        eGfx->removeUavBuffer(m_uavTemp);
    }

	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eBlurOp, passes, "Passes", "1|2|3|4|5|6|7|8|9|10", 0,
        eOP_PAR_FXY(eBlurOp, amount, "Amount", 0.0f, 16.0f, 0.01f, 0.01f,
        eOP_PAR_FLOAT(eBlurOp, amplify, "Amplify", 0.0f, 16.0f, 1.0f,
		eOP_PAR_END))))
    {
        _allocateUav(m_bmpWidth, m_bmpHeight, m_uavTemp);

        const eInt threadCount = 128;
        const eF32 kernelHalfX = amount.x*(eF32)m_bmpWidth;
        const eF32 kernelHalfY = amount.y*(eF32)m_bmpHeight;
        const eInt tgx = eCeil((eF32)m_bmpWidth/((eF32)threadCount-2.0f*kernelHalfX));
        const eInt tgy = eCeil((eF32)m_bmpHeight/((eF32)threadCount-2.0f*kernelHalfY));

        eUavBuffer *inUav = ((eIBitmapOp *)getAboveOp(0))->getResult().uav;
        eComputeShader *csBlurH = eGfx->loadComputeShader(eSHADER(cs_bmp_blur), "HBLUR");
        eComputeShader *csBlurV = eGfx->loadComputeShader(eSHADER(cs_bmp_blur), "VBLUR");

        for (eS32 i=0; i<=passes; i++)
        {
            _execCs(csBlurH, eTMF_NEAREST|eTMF_CLAMP, m_uavTemp, (!i ? inUav : m_uav), nullptr, eSize(tgx, m_bmpHeight));
            _execCs(csBlurV, eTMF_NEAREST|eTMF_CLAMP, m_uav, m_uavTemp, nullptr, eSize(m_bmpWidth, tgy));
        }
    }
	eOP_EXEC2_END

    eOP_VAR(eUavBuffer *m_uavTemp);
eOP_END(eBlurOp);
#endif

// Distort (bitmap) operator
// -------------------------
// Distorts input bitmap 0 on x-axis using color
// information from red channel and on y-axis
// using color information from green channel.

#if defined(HAVE_OP_BITMAP_DISTORT) || defined(eEDITOR)
eOP_DEF_BMP(eDistortOp, "Distort", 'd', 2, 2, eOP_INPUTS(eOC_BMP, eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eDistortOp, cs_bmp_distort);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eDistortOp, amountVal, "Amount", -4.0f, 4.0f, 0.0f, 0.0f,
		eOP_PAR_END))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_distort)), eTMF_BILINEAR|eTMF_WRAP);
    }
	eOP_EXEC2_END
eOP_END(eDistortOp);
#endif

// Twirl (bitmap) operator
// -----------------------
// Twirls (rotates) the input bitmap at the given
// center, using the given radius and strength.

#if defined(HAVE_OP_BITMAP_TWIRL) || defined(eEDITOR)
eOP_DEF_BMP(eTwirlOp, "Twirl", ' ', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eTwirlOp, cs_bmp_twirl);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eTwirlOp, centerVal, "Center", 0.0f, 2.0f, 0.5f, 0.5f,
        eOP_PAR_FXY(eTwirlOp, radiusVal, "Radius", eALMOST_ZERO, 2.0f, 0.25f, 0.25f,
        eOP_PAR_FLOAT(eTwirlOp, strength, "Strength", -16.0f, 16.0f, 0.0f,
		eOP_PAR_END))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_twirl)), eTMF_BILINEAR|eTMF_CLAMP);
    }
	eOP_EXEC2_END
eOP_END(eTwirlOp);
#endif

// Color (bitmap) operator
// -----------------------
// Multi-purpose operator which supports different
// operations on bitmap (add, subtracting and
// multiplying a color, grayscale and invert bitmap).

#if defined(HAVE_OP_BITMAP_COLOR) || defined(eEDITOR)
eOP_DEF_BMP(eColorOp, "Color", 'c', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eColorOp, cs_bmp_color);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eColorOp, mode, "Mode", "Add|Sub|Multiply|Grayscale|Invert", 0,
        eOP_PAR_RGBA(eColorOp, col, "Color", 0, 0, 0, 255,
		eOP_PAR_END)))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_color)));
    }
	eOP_EXEC2_END
eOP_END(eColorOp);
#endif

// Rotozoom (bitmap) operator
// --------------------------
// This operator can rotate, scroll and
// zoom a bitmap.

#if defined(HAVE_OP_BITMAP_ROTOZOOM) || defined(eEDITOR)
eOP_DEF_BMP(eRotoZoomOp, "Rotozoom", 'r', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eRotoZoomOp, cs_bmp_rotzoom);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FLOAT(eRotoZoomOp, angleVal, "Angle", eF32_MIN, eF32_MAX, 0.0f,
        eOP_PAR_FXY(eRotoZoomOp, zoomVal, "Zoom", 0.0f, 16.0f, 1.0f, 1.0f,
        eOP_PAR_FXY(eRotoZoomOp, scrollVal, "Scroll", eF32_MIN, eF32_MAX, 0.5f, 0.5f,
        eOP_PAR_FLAGS(eRotoZoomOp, clampBorders, "Clamp borders", "X axis|Y axis", 0,
		eOP_PAR_END)))))
    {
        const eBool xClamp = eGetBit(clampBorders, 0);
        const eBool yClamp = eGetBit(clampBorders, 1);
        const eU32 texAddr = (xClamp ? eTMF_CLAMPX : eTMF_WRAPX)|(yClamp ? eTMF_CLAMPY : eTMF_WRAPY);
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_rotzoom)), eTMF_BILINEAR|texAddr);
    }
	eOP_EXEC2_END
eOP_END(eRotoZoomOp);
#endif

// Text (bitmap) operator
// ----------------------
// Writes text on its input bitmap.

#if defined(HAVE_OP_BITMAP_TEXT) || defined(eEDITOR)
eOP_DEF_BMP(eTextOp, "Text", 't', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eTextOp, fltPos, "Position", -4.0f, 4.0f, 0.0f, 0.0f,
        eOP_PAR_FLOAT(eTextOp, sizeVal, "Size", 0.0f, 16.0f, 0.125f,
        eOP_PAR_FLOAT(eTextOp, stretchVal, "Stretch", 0.0f, 16.0f, 0.0f,
        eOP_PAR_FLOAT(eTextOp, leadingVal, "Leading", 0.0f, 16.0f, 0.1f,
        eOP_PAR_FLOAT(eTextOp, kerningVal, "Kerning", 0.0f, 16.0f, 0.0f,
        eOP_PAR_RGBA(eTextOp, col, "Color", 255, 255, 255, 255,
        eOP_PAR_STRING(eTextOp, fontName, "Font", "Arial",
        eOP_PAR_FLAGS(eTextOp, flags, "Flags", "Bold|Italic|Underline", 0,
        eOP_PAR_TEXT(eTextOp, text, "Text", "Brain Control",
		eOP_PAR_END))))))))))
    {
        // relative to absolute calculations
        const eInt size = eFtoL(sizeVal*m_bmpHeight);
        const eU32 stretch = eFtoL(stretchVal*m_bmpWidth);
        const eU32 leading = eFtoL(leadingVal*m_bmpHeight);
        const eU32 kerning = eFtoL(kerningVal*m_bmpWidth);
        const eBool bold = eGetBit(flags, 0);
        const eBool italic = eGetBit(flags, 1);
        const eBool underline = eGetBit(flags, 2);
        const ePoint pos(eFtoL(fltPos.x*m_bmpWidth), eFtoL(fltPos.y*m_bmpHeight));

        // prepare DCs, bitmap and font
        HDC hdc = GetDC(nullptr);
        HDC compDc = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, m_bmpWidth, m_bmpHeight);

        LOGFONT lf;
        eMemSet(&lf, 0, sizeof(lf));
        eStrCopy(lf.lfFaceName, fontName);
        lf.lfCharSet = ANSI_CHARSET;
        lf.lfHeight = -size;
        lf.lfWidth = stretch;
        lf.lfQuality = ANTIALIASED_QUALITY;
        lf.lfWeight = (bold ? FW_BOLD : FW_NORMAL);
        lf.lfItalic = italic;
        lf.lfUnderline = underline;
        HFONT font = CreateFontIndirect(&lf);

        SelectObject(compDc, bmp);
        SetTextColor(compDc, RGB(col.b, col.g, col.r));
        SetBkMode(compDc, TRANSPARENT);
        SelectObject(compDc, font);

        // write bitmap data to DIB so text can
        // be rendered onto an existing bitmap
        eArray<eColor> inputBmp, tempBmp;
        eGfx->readTexture2d(((eIBitmapOp *)getAboveOp(0))->getResult().uav->tex, inputBmp);
        tempBmp = inputBmp;

        BITMAPINFO bmi;
        eMemSet(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biWidth = m_bmpWidth;
        bmi.bmiHeader.biHeight = -(eInt)m_bmpHeight;
        bmi.bmiHeader.biCompression = BI_RGB;
        SetDIBits(compDc, bmp, 0, m_bmpHeight, &tempBmp[0], &bmi, DIB_RGB_COLORS);

        // render text line by line and character
        // by character so that leading and kerning
        // can be applied (this isn't possible using
        // the windows API functions)
        RECT r;
        r.bottom = m_bmpHeight;
        r.right = m_bmpWidth;
        r.left = pos.x;

        eU32 lineNum = 0;
        eU32 i = 0;

        while (text[i] != '\0')
        {
            if (text[i] != '\n')
            {
                const eChar chr[2] = {text[i], '\0'};
                SIZE s;

                GetTextExtentPoint32(compDc, chr, 1, &s);
                r.top = pos.y+lineNum*leading;
                DrawText(compDc, chr, 1, &r, 0);
                r.left += s.cx+kerning;
            }
            else
            {
                lineNum++;
                r.left = pos.x;
            }
            
            i++;
        }

        // read back bitmap data with text on
        GetDIBits(compDc, bmp, 0, m_bmpHeight, &tempBmp[0], &bmi, DIB_RGB_COLORS);
        for (eU32 i=0; i<m_bmpWidth*m_bmpHeight; i++)
            tempBmp[i].a = inputBmp[i].a;

        eGfx->updateTexture2d(m_uav->tex, &tempBmp[0]);

        // free GDI objects
        DeleteObject(font);
        DeleteObject(bmp);
        DeleteDC(compDc);
        DeleteDC(hdc);
    }
	eOP_EXEC2_END
eOP_END(eTextOp);
#endif

// Normals (bitmap) operator
// -------------------------
// Generates a normal map of its input bitmap,
// which is used for bump mapping for example.
// The sobel operator is used here in x and y
// direction (normally used for edge detection).

#if defined(HAVE_OP_BITMAP_NORMALS) || defined(eEDITOR)
eOP_DEF_BMP(eNormalsOp, "Normals", 'n', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eNormalsOp, cs_bmp_normals);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
       eOP_PAR_FLOAT(eNormalsOp, strength, "Strength", 0, 1.0f, 32,
       eOP_PAR_FLOAT(eNormalsOp, intensity, "Intensity", 0.0f, 1.0f, 0.5f,
		eOP_PAR_END)))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_normals)), eTMF_NEAREST|eTMF_WRAP);
    }
	eOP_EXEC2_END
eOP_END(eNormalsOp);
#endif

// Line (bitmap) operator
// ----------------------
// Draws a line.

#if defined(HAVE_OP_BITMAP_LINE) || defined(eEDITOR)
eOP_DEF_BMP(eLineOp, "Line", 'l', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eLineOp, cs_bmp_line);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eLineOp, startVal, "Start", 0.0f, 1.0f, 0.0f, 0.0f,
        eOP_PAR_FXY(eLineOp, endVal, "End", 0.0f, 1.0f, 1.0f, 1.0f,
        eOP_PAR_FLOAT(eLineOp, thickness, "Thickness", 0.1f, eF32_MAX, 1.2f,
        eOP_PAR_FLOAT(eLineOp, decay, "Decay", 0.0f, eF32_MAX, 1.0f,
        eOP_PAR_RGBA(eLineOp, col, "Color", 255, 255, 255, 255,
		eOP_PAR_END))))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_line)));
    }
	eOP_EXEC2_END
eOP_END(eLineOp);
#endif

// Circle (bitmap) operator
// ------------------------
// Draws a circle.

#if defined(HAVE_OP_BITMAP_CIRCLE) || defined(eEDITOR)
eOP_DEF_BMP(eCircleOp, "Circle", ' ', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eCircleOp, cs_bmp_circle);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_FXY(eCircleOp, posVal, "Position", eF32_MIN, eF32_MAX, 0.5f, 0.5f,
        eOP_PAR_FLOAT(eCircleOp, radiusVal, "Radius", 0.0f, eF32_MAX, 0.2f,
        eOP_PAR_FLOAT(eCircleOp, thickness, "Thickness", 0.1f, eF32_MAX, 1.2f,
        eOP_PAR_FLOAT(eCircleOp, decay, "Decay", 1.0f, eF32_MAX, 1.0f,
        eOP_PAR_RGBA(eCircleOp, col, "Color", 255, 255, 255, 255,
		eOP_PAR_END))))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_circle)));
    }
	eOP_EXEC2_END
eOP_END(eCircleOp);
#endif

// Mask (bitmap) operator
// ----------------------
// Uses third input bitmap as "blend map" (mask)
// for combining first and second input bitmaps.

#if defined(HAVE_OP_BITMAP_MASK) || defined(eEDITOR)
eOP_DEF_BMP(eMaskOp, "Mask", 'k', 3, 3, eOP_INPUTS(eOC_BMP, eOC_BMP, eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eMaskOp, cs_bmp_mask);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,eOP_PAR_END)
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_mask)));
    }
	eOP_EXEC2_END
eOP_END(eMaskOp);
#endif

// Pixels (bitmap) operator
// ------------------------
// Puts randomly pixels on bitmap. The color
// for each pixel is randomly interpolated
// between two colors.

#if defined(HAVE_OP_BITMAP_PIXELS) || defined(eEDITOR)
eOP_DEF_BMP(ePixelsOp, "Pixels", ' ', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(ePixelsOp, cs_bmp_pixels);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_RGBA(ePixelsOp, col0, "Color 0", 0, 0, 0, 255,
        eOP_PAR_RGBA(ePixelsOp, col1, "Color 1", 255, 255, 255, 255,
        eOP_PAR_FLOAT(ePixelsOp, percent, "Percent", 0.0f, 100.0f, 1.0f,
        eOP_PAR_INT(ePixelsOp, seed, "Seed", 0, 65535, 0,
		eOP_PAR_END)))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_pixels)));
    }
	eOP_EXEC2_END
eOP_END(ePixelsOp);
#endif

// Multiply (bitmap) operator
// --------------------------
// This operator will call rotozoom multiple
// times and merge the result with the original.

#if defined(HAVE_OP_BITMAP_MULTIPLY) || defined(eEDITOR)
eOP_DEF_BMP(eBitmapMultiplyOp, "Multiply", ' ', 1, 1, eOP_INPUTS(eOC_BMP))
    eOP_INIT()
    {
        m_uavTemp = nullptr;
  	    eREGISTER_OP_SHADER(eBitmapMultiplyOp, cs_bmp_multiply);
    }

	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_INT(eBitmapMultiplyOp, count, "Count", 1, 256, 1,
        eOP_PAR_ENUM(eBitmapMultiplyOp, mode, "Mode", "Add|Sub|Mul|Difference|Average|Minimum|Maximum", 0,
        eOP_PAR_FLOAT(eBitmapMultiplyOp, angleVal, "Angle", eF32_MIN, eF32_MAX, 0.0f,
        eOP_PAR_FXY(eBitmapMultiplyOp, zoomVal, "Zoom", 0.0f, 16.0f, 1.0f, 1.0f,
        eOP_PAR_FXY(eBitmapMultiplyOp, scrollVal, "Scroll", -4.0f, 4.0f, 0.5f, 0.5f,
        eOP_PAR_FLAGS(eBitmapMultiplyOp, clampBorders, "Clamp borders", "X axis|Y axis", 0,
		eOP_PAR_END)))))))
    {
        const eBool xClamp = eGetBit(clampBorders, 0);
        const eBool yClamp = eGetBit(clampBorders, 1);
        const eU32 texAddr = (xClamp ? eTMF_CLAMPX : eTMF_WRAPX)|(yClamp ? eTMF_CLAMPY : eTMF_WRAPY);
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_multiply)), eTMF_BILINEAR|texAddr);
    }
	eOP_EXEC2_END

    eOP_VAR(eUavBuffer *m_uavTemp);
eOP_END(eBitmapMultiplyOp);
#endif

// CellularGrowth (bitmap) operator
// ----------------------
// Simulates cellular growth. (According to the book "The Algorithmic Beauty of Sea Shells")

#if defined(HAVE_OP_BITMAP_CELLGROWTH) || defined(eEDITOR)
eOP_DEF_BMP(eCellGrowthOp, "CellGrowth", 'g', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eCellGrowthOp, ranSeed, "RandomSeed", 1, 100000, 1,
        eOP_PAR_ENUM(eCellGrowthOp, widthSel, "Width", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_ENUM(eCellGrowthOp, heightSel, "Height", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_RGBA(eCellGrowthOp, color0, "Color0", 255, 255, 255, 255,
        eOP_PAR_RGBA(eCellGrowthOp, color1, "Color1", 255, 0, 0, 255,
        eOP_PAR_FXYZW(eCellGrowthOp, initial, "Initial", 0, eF32_MAX, 1.0f, 1.0f, 1.0f, 1.0f,
		// the inhibitor diffusion rate must we way higher (at least 7 times) than the activator (page 20)
        eOP_PAR_FXYZW(eCellGrowthOp, diffusion, "Diffusion", 0, 1.0f, 0.005f, 0.043f, 0.043f, 0.043f,
		// the inhibitor decrease rate must be higher than the activator (page 21)
        eOP_PAR_FXYZW(eCellGrowthOp, removal, "Removal", 0.0f, eF32_MAX, 0.6f, 0.45f, 0.45f, 0.45f,
        eOP_PAR_LINK_PATH(eCellGrowthOp, additionOp, "Addition", eOC_PATH, eTRUE,
        eOP_PAR_FXYZW(eCellGrowthOp, saturation, "Saturation", 0, eF32_MAX, 0.0f, 0.0f, 0.0f, 0.0f,
//        eOP_PAR_ENUM(eCellGrowthOp, mode, "Mode", "Activator-Inhibitor|Activator-Depletion|Activator-Depletion(Ext)", 0,
        eOP_PAR_ENUM(eCellGrowthOp, mode, "Mode", "Activator-Inhibitor|Activator-Depletion", 0,
		eOP_PAR_END))))))))))))
    {
        const ePath4 &addPath = additionOp->getResult().path;

		eU32 rseed = ranSeed;
		eU32 width = 1<<widthSel;
		eU32 height = 1<<heightSel;

        eArray<eColor> bitmap(width*height);

		m_substance.clear();
		m_prodrate.clear();
		m_baseAdd.clear();
		for(eU32 i = 0; i < width; i++) {
			m_substance.append(initial);
			const eVector4& va = addPath.evaluateUnitTime((eF32)i / (eF32)(width - 1));
			m_baseAdd.append(va);
			// source density is proportional to decay rate plus 1% statistical fluctuation
			m_prodrate.append(removal.x * (0.995f + 0.01f * eRandomF(rseed)));
		}

        for (eU32 k=0; k<height; k++) {
			m_dd.clear();
	        for (eU32 i=0; i<width; i++) {
				eU32 leftIdx = ((i == 0) ? i : ((eS32)i - 1)); 
				eU32 rightIdx = ((i == (width - 1)) ? i : (i + 1)); 
				// calculate second derivative over position (needed for diffusion (page 23))
				m_dd.append((m_substance[rightIdx] - m_substance[i]) - (m_substance[i] - m_substance[leftIdx]));
			}

	        for (eU32 i=0; i<width; i++) {
				eF32 asqr = (m_substance[i].x * m_substance[i].x);

				if(mode == 0) 
					m_substance[i] += eVector4(
							m_prodrate[i] * (asqr / ((saturation.y + m_substance[i].y) * (1.0f + saturation.x * asqr)) + m_baseAdd[i].x) - removal.x * m_substance[i].x + diffusion.x * m_dd[i].x,
							m_prodrate[i] * asqr - removal.y * m_substance[i].y + diffusion.y * m_dd[i].y + m_baseAdd[i].y,
							0,
							0); 

				if(mode == 1) {
					eF32 astar2 = (asqr / (1.0f + saturation.x * asqr)) + m_baseAdd[i].x;
					eF32 terma = m_prodrate[i] * m_substance[i].y * astar2;
					m_substance[i] += eVector4(
								terma - removal.x * m_substance[i].x + diffusion.x * m_dd[i].x,
								m_baseAdd[i].y - terma - removal.y * m_substance[i].y + diffusion.y * m_dd[i].y,
								0,
								0); 
				}
/*
				if(mode == 2) {
					eF32 astar2 = (asqr + m_baseAdd[i].x) / (1.0f + saturation.x * asqr);
					eF32 terma = (astar2 * m_prodrate[i] * m_substance[i].y) / (saturation.y + saturation.z * m_substance[i].z);
					m_substance[i] += eVector4(
								terma - removal.x * m_substance[i].x + diffusion.x * m_dd[i].x,
								m_baseAdd[i].y - terma - removal.y * m_substance[i].y + diffusion.y * m_dd[i].y,
								removal.z * (m_substance[i].x - m_substance[i].z) + diffusion.z * m_dd[i].z,
								0); 
				}
*/
				for(eU32 d = 0; d < 4; d++) {
					eASSERT(eIsNumber(m_substance[i][d]));
					m_substance[i][d] = eClamp(0.000001f, m_substance[i][d], 1000.0f);
				}

				eF32 val0 = eClamp(0.0f, m_substance[i].x, 1.0f);
				eF32 val1 = eClamp(0.0f, m_substance[i].y, 1.0f);
				eASSERT(!eIsNan(val0));
				eASSERT(!eIsNan(val1));
				eASSERT(eIsNumber(val0));
				eASSERT(eIsNumber(val1));

				eColor col = color0.lerp(color1, val0);
//				eColor col = (m_substance[i].x > threshold ? color1 : color0);
		         bitmap[k * width + i] = col;
			}
		}

		_reallocate(width, height);
		eGfx->updateTexture2d(m_uav->tex, &bitmap[0]);
    }
	eOP_EXEC2_END

	eArray<eF32>		m_prodrate;
	eArray<eVector4>	m_substance;
	eArray<eVector4>	m_baseAdd;
	eArray<eVector4>	m_dd;
eOP_END(eCellGrowthOp);
#endif

// SkinStamp (bitmap) operator
// ----------------------
// Simulates skin

#if defined(HAVE_OP_BITMAP_SKINSTAMP) || defined(eEDITOR)
eOP_DEF_BMP(eSkinStampOp, "SkinStamp", 'g', 1, 2, eOP_INPUTS(eOC_BMP, eOC_BMP))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_INT(eSkinStampOp, ranSeed, "RandomSeed", 1, 100000, 1,
        eOP_PAR_ENUM(eSkinStampOp, widthSel, "Width", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_ENUM(eSkinStampOp, heightSel, "Height", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_INT(eSkinStampOp, cellsX, "CellsX", 1, 100000, 16,
        eOP_PAR_INT(eSkinStampOp, cellsY, "CellsY", 1, 100000, 16,
        eOP_PAR_FXY(eSkinStampOp, randomize, "Randomize", 0.0f, 1.0f, 1, 1,
		eOP_PAR_END)))))))
    {
		eU32 rseed = ranSeed;
		eU32 width = 1<<widthSel;
		eU32 height = 1<<heightSel;
        eArray<eColor> bitmap(width*height);

		const eIBitmapOp::Result& inpRes = ((eIBitmapOp *)getAboveOp(0))->getResult();
		const eIBitmapOp::Result* colMapRes = (getAboveOpCount() > 1 ? &((eIBitmapOp *)getAboveOp(1))->getResult() : nullptr);

        eArray<eColor> inp, colmap;
        eGfx->readTexture2d(inpRes.uav->tex, inp);

        if (colMapRes)
            eGfx->readTexture2d(colMapRes->uav->tex, colmap);

		// init
        for (eU32 k=0; k<height; k++) 
	        for (eU32 i=0; i<width; i++) 
				bitmap[k * width + i] = eCOL_BLACK;

		eF32 cellSizeX = (eF32)width * 2.0f / (1.0f + (eF32)cellsX); 
		eF32 cellSizeY = (eF32)height * 2.0f / (1.0f + (eF32)cellsY); 
//		eF32 cellSizeX = (eF32)width / ((eF32)cellsX); 
//		eF32 cellSizeY = (eF32)height / (1.0f + (eF32)cellsY); 

		eF32 cellDistX = 0.5f * cellSizeX; 
		eF32 cellDistY = 0.5f * cellSizeY; 

		// stamp the source image
		for(eS32 sy = cellsY + 1; sy >= 0; sy--) {
			eF32 offsetX = (sy % 2 == 0) ? -1.0f : 0.0f;
			for(eU32 sx = 0; sx < cellsX + 1; sx++) {
				eF32 sxp = (eF32)sx + (eRandomF(rseed) - 0.5f) * cellDistX * randomize.x;
				eF32 syp = (eF32)sy + (eRandomF(rseed) - 0.5f) * cellDistY * randomize.y;
				eS32 px = eFtoL(((offsetX + sxp * 2.0f) * cellDistX));
				eS32 py = eFtoL(((-1.0f + syp) * cellDistY));
				eU32 dx = 1 + (eU32)eFtoL(cellSizeX);
				eU32 dy = 1 + (eU32)eFtoL(cellSizeY);

				eS32 centerX = (colMapRes ? ((px + (dx >> 1)) * colMapRes->width) / width : 0);
				eS32 centerY = (colMapRes ? ((py + (dy >> 1)) * colMapRes->height) / height : 0);
				eColor structureCol = (colMapRes ? colmap[eMax(0, eMin((eS32)colMapRes->height - 1, centerY)) * colMapRes->width + eMax(0, eMin((eS32)colMapRes->width - 1, centerX))] 
				                              : eColor(0,0,0,0));
				for(eU32 ly = 0; ly < dy; ly++) {
					for(eU32 lx = 0; lx < dx; lx++) {
						eS32 cx = px + (eS32)lx;
						eS32 cy = py + (eS32)ly;
						if((cx >= 0) && (cx < (eS32)width) && (cy >= 0) && (cy < (eS32)height)) {
							eU32 sourceX = (lx * inpRes.width) / dx; 
							eU32 sourceY = (ly * inpRes.height) / dy;
							const eColor& maskSrc = inp[sourceY * inpRes.width + sourceX];
							eColor& dst = bitmap[cy * width + cx];
							const eColor scol = maskSrc.lerp(structureCol, (eF32)structureCol.a / 255.f);
							dst = dst.lerp(scol, (eF32)maskSrc.a / 255.f);
							dst.a = 255;	
						}
					}
				}
			}
		}

        eGfx->updateTexture2d(m_uav->tex, &bitmap[0]);
    }
	eOP_EXEC2_END
eOP_END(eSkinStampOp);
#endif

#if defined(HAVE_OP_BITMAP_BMPCHECKSUM) || defined(eEDITOR)
eOP_DEF_BMP(eBmpChecksumOp, "BmpChecksum", 'g', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
		eOP_PAR_END)
    {
		const eIBitmapOp::Result& inpRes = ((eIBitmapOp *)getAboveOp(0))->getResult();
        eArray<eColor> inp;
        eGfx->readTexture2d(inpRes.uav->tex, inp);

		// calc checksum
		eF32 checkSum = 0;
        for (eU32 i=0; i<inp.size(); i++)
        {
            eColor& c = inp[i];
			checkSum += c.a;
			checkSum += c.r;
			checkSum += c.g;
			checkSum += c.b;
		}
		eWriteToLog(eString("Bitmap Checksum: ") + eFloatToStr(checkSum));
	}
	eOP_EXEC2_END
eOP_END(eBmpChecksumOp);
#endif


// Color grading (bitmap) operator
// -------------------------------
// Creates a 256x16 color grading map that
// is used as a lookup for color grading FX.

#if defined(HAVE_OP_BITMAP_COLOR_GRADING) || defined(eEDITOR)
eOP_DEF_BMP(eColorGradingOp, "Color grading", ' ', 0, 0, eOP_INPUTS())
	eOP_EXEC2(ENABLE_STATIC_PARAMS,
        eOP_PAR_RGBA(eColorGradingOp, colorR, "Red", 255, 0, 0, 255,
        eOP_PAR_RGBA(eColorGradingOp, colorG, "Green", 0, 255, 0, 255,
        eOP_PAR_RGBA(eColorGradingOp, colorB, "Blue", 0, 0, 255, 255,
        eOP_PAR_FLOAT(eColorGradingOp, steepnessR, "Red steepness", 0.1f, 10.0f, 1.0f,
        eOP_PAR_FLOAT(eColorGradingOp, steepnessG, "Green steepness", 0.1f, 10.0f, 1.0f,
        eOP_PAR_FLOAT(eColorGradingOp, steepnessB, "Blue steepness", 0.1f, 10.0f, 1.0f,
        eOP_PAR_ENUM(eColorGradingOp, stepsSel, "Steps", "2|4|8|16", 3,
		eOP_PAR_END))))))))
    {
        const eU32 COLOR_GRADING_SIZE = 16;
        const eU32 BM_WIDTH = COLOR_GRADING_SIZE * COLOR_GRADING_SIZE;
        const eU32 BM_HEIGHT = COLOR_GRADING_SIZE;
        eArray<eColor> bitmap(BM_WIDTH*BM_HEIGHT);
        eF32 stepLookup[COLOR_GRADING_SIZE];
        eF32 steps = (eF32)(COLOR_GRADING_SIZE>>(stepsSel+1));

        for (eU32 i=0; i<COLOR_GRADING_SIZE; i++)
        {
            const eU32 v = eFtoL(i/steps);
            stepLookup[i] = (eF32)v*steps/(eF32)COLOR_GRADING_SIZE;
        }

        _reallocate(BM_WIDTH, BM_HEIGHT);
        const eColor blackCol = eCOL_BLACK;

        for (eU32 z=0; z<COLOR_GRADING_SIZE; z++)
        {
            const eU32 lookupPosG = eFtoL(ePow(eF32(z) / COLOR_GRADING_SIZE, steepnessG) * COLOR_GRADING_SIZE);
            const eF32 posG = stepLookup[eMin<eU32>(lookupPosG, COLOR_GRADING_SIZE-1)];
            const eColor colG = blackCol.lerp(colorG, posG);
            const eU32 zoffset = z * COLOR_GRADING_SIZE;

            for (eU32 x=0; x<COLOR_GRADING_SIZE; x++)
            {
                const eU32 lookupPosR = eFtoL(ePow(eF32(x) / COLOR_GRADING_SIZE, steepnessR) * COLOR_GRADING_SIZE);
                const eF32 posR = stepLookup[eMin<eU32>(lookupPosR, COLOR_GRADING_SIZE-1)];
                const eColor colR = blackCol.lerp(colorR, posR);
                
                for (eU32 y=0; y<COLOR_GRADING_SIZE; y++)
                {
                    const eU32 lookupPosB = eFtoL(ePow(eF32(y) / COLOR_GRADING_SIZE, steepnessB) * COLOR_GRADING_SIZE);
                    const eF32 posB = stepLookup[eMin<eU32>(lookupPosB, COLOR_GRADING_SIZE-1)];
                    const eColor colB = blackCol.lerp(colorB, posB);

                    bitmap[y * BM_WIDTH + zoffset + x] = colR + colG + colB;
                }
            }
        }

        eGfx->updateTexture2d(m_uav->tex, &bitmap[0]);
    }
	eOP_EXEC2_END
eOP_END(eColorGradingOp);
#endif

// Cells (bitmap) operator
// -----------------------
// Generates cellular textures, which can be used
// e.g. to create bitmaps that look like stone.

#if defined(HAVE_OP_BITMAP_CELLS) || defined(eEDITOR)
eOP_DEF_BMP(eCellsOp, "Cells", ' ', 0, 0, eOP_INPUTS())
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eCellsOp, cs_bmp_cells);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_ENUM(eCellsOp, widthSel, "Width", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_ENUM(eCellsOp, heightSel, "Height", "1|2|4|8|16|32|64|128|256|512|1024|2048|4096|8192", 8,
        eOP_PAR_INT(eCellsOp, numPoints, "Points", 1, 40, 5,
        eOP_PAR_FLOAT(eCellsOp, regularityVal, "Regularity", 0.0f, 1.0f, 0.25f,
        eOP_PAR_ENUM(eCellsOp, pattern, "Pattern", "Stone|Cobweb", 0,
        eOP_PAR_INT(eCellsOp, seed, "Seed", 0, 65535, 0,
        eOP_PAR_LABEL(eCellsOp, label0, "Coloring", "Coloring",
        eOP_PAR_FLOAT(eCellsOp, amplify, "Amplify", 0.0f, 16.0f, 1.0f,
        eOP_PAR_FLOAT(eCellsOp, gamma, "Gamma", 0.0f, 16.0f, 1.0f,
        eOP_PAR_RGBA(eCellsOp, col0, "Color 0", 255, 255, 255, 255,
        eOP_PAR_RGBA(eCellsOp, col1, "Color 1", 0, 0, 0, 255,
		eOP_PAR_END))))))))))))
    {
		typedef eVector2 PointData[2048];
		static eConstBuffer<PointData, eST_CS> cbPoints;

        // generate control points for cells
        eRandomize(seed);

        const eF32 regularity = 1.0f-regularityVal;

        for (eU32 y=0, index=0; y<numPoints; y++)
        {
            for (eU32 x=0; x<numPoints; x++)
            {
                eVector2 &p = cbPoints.data[index++];
                p.x = (x+0.5f+eRandomF(-0.5f, 0.5f)*regularity)/(eF32)numPoints;
                p.y = (y+0.5f+eRandomF(-0.5f, 0.5f)*regularity)/(eF32)numPoints;
            }
        }

        // generate cells in shader
        _reallocate(1<<widthSel, 1<<heightSel);
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_cells)), 0, nullptr, nullptr, &cbPoints);
    }
	eOP_EXEC2_END
eOP_END(eCellsOp);
#endif

// Bump (bitmap) operator
// ----------------------
// The bump operator simulates a lit 3D surface
// in a bitmap, using the phong shading model. 

#if defined(HAVE_OP_BITMAP_BUMP) || defined(eEDITOR)
eOP_DEF_BMP(eBumpOp, "Bump", 'u', 2, 2, eOP_INPUTS(eOC_BMP, eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eBumpOp, cs_bmp_bump);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
        eOP_PAR_RGB(eBumpOp, ambientCol, "Ambient", 255, 255, 255,
        eOP_PAR_RGB(eBumpOp, diffuseCol, "Diffuse", 255, 255, 255,
        eOP_PAR_RGB(eBumpOp, specCol, "Specular", 128, 128, 128,
        eOP_PAR_FXYZ(eBumpOp, position, "Position", -8.0f, 8.0f, 1.0f, 1.0f, 0.5f,
        eOP_PAR_FLOAT(eBumpOp, specAmount, "Specular amount", 0.0f, 4.0f, 1.0f,
        eOP_PAR_FLOAT(eBumpOp, bumpAmount, "Bump amount", 0.0f, 4.0f, 1.0f,
		eOP_PAR_END)))))))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_bump)));
    }
	eOP_EXEC2_END
eOP_END(eBumpOp);
#endif

// Alpha (bitmap) operator
// -----------------------
// Modifies the alpha channel.

#if defined(HAVE_OP_BITMAP_ALPHA) || defined(eEDITOR)
eOP_DEF_BMP(eAlphaOp, "Alpha", ' ', 1, 2, eOP_INPUTS(eOC_BMP, eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(eAlphaOp, cs_bmp_alpha);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
       eOP_PAR_ENUM(eAlphaOp, mode, "Mode", "This: grayscale|This: 1-grayscale|Input 2: grayscale|Input 2: 1-grayscale", 0,
		eOP_PAR_END))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_alpha)));
    }
	eOP_EXEC2_END
eOP_END(eAlphaOp);
#endif

// Pixelize (bitmap) operator
// -----------------------
// Pixelizes the image

#if defined(HAVE_OP_BITMAP_PIXELIZE) || defined(eEDITOR)
eOP_DEF_BMP(ePixelizeOp, "Pixelize", ' ', 1, 1, eOP_INPUTS(eOC_BMP))
	eOP_INIT() {
	   eREGISTER_OP_SHADER(ePixelizeOp, cs_bmp_pixelize);
	}
	eOP_EXEC2(DISABLE_STATIC_PARAMS,
		eOP_PAR_IXY(ePixelizeOp, amount, "Amount", 1, 256, 4, 4,
		eOP_PAR_END))
    {
        _execCs(eGfx->loadComputeShader(eSHADER(cs_bmp_pixelize)));
    }
	eOP_EXEC2_END
eOP_END(ePixelizeOp);
#endif