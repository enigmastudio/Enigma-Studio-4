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

#include "../eshared.hpp"
#include "engine.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

eFont::eFont() :
    m_tex(nullptr),
    m_numLetters(0),
    m_height(0)
{
    eMemSet(m_letters, 0, sizeof(m_letters));
}

eFont::~eFont()
{
    eGfx->removeTexture2d(m_tex);
}

void eFont::create(const eChar *fontFace, const eChar *letters, eU32 height, eColor color)
{
    LOGFONT lf;
    eMemSet(&lf, 0, sizeof(lf));
    eStrCopy(lf.lfFaceName, fontFace);
    lf.lfHeight = -(eInt)height; 
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET; 
    lf.lfOutPrecision = OUT_TT_PRECIS; 
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS; 
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
    HFONT font = CreateFontIndirect(&lf);
    eASSERT(font);

    HDC hdc = GetDC(nullptr);
    eASSERT(hdc);
    HDC compDc = CreateCompatibleDC(hdc);
    eASSERT(compDc);

    SetTextColor(compDc, RGB(color.b, color.g, color.r));
    SetBkMode(compDc,TRANSPARENT);
    SelectObject(compDc, font);

    TEXTMETRIC met;
    GetTextMetrics(compDc, &met);
    m_height = met.tmHeight;

    const eU32 border = 2;
    const eU32 numLetters = eStrLength(letters);
    const eF32 chrPerLine = eSqrt((eF32)numLetters);
    const eU32 texWidth = eNextPowerOf2(eFtoL((eF32)(met.tmAveCharWidth+border)*chrPerLine));
    const eU32 texHeight = eNextPowerOf2(eFtoL((eF32)(met.tmHeight+border)*chrPerLine));

    HBITMAP bmp = CreateCompatibleBitmap(hdc, texWidth, texHeight);
    eASSERT(bmp);
    SelectObject(compDc, bmp);

    m_bmp.resize(texWidth*texHeight);
    eMemSet(&m_bmp[0], 0, m_bmp.size()*sizeof(eColor));

    BITMAPINFO bmi;
    eMemSet(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = texWidth;
    bmi.bmiHeader.biHeight = -(eInt)texHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    SetDIBits(compDc, bmp, 0, texHeight, &m_bmp[0],&bmi,DIB_RGB_COLORS);

    eU32 x = 0;
    eU32 y = 0;

    for (eU32 i=0; i<numLetters; i++)
    {
        const eChar c = letters[i];
        const eChar text[] = {c, '\0'};
        SIZE size;
        GetTextExtentPoint32(compDc, text, 1, &size);

        if (x+size.cx >= texWidth)
        {
            x = 0;
            y += met.tmHeight+border;
        }

        eFontLetter &l = m_letters[c];
        l.chr = c;
        l.width = size.cx;
        l.used = eTRUE;
        l.rect.set(x, y, x+l.width, y+met.tmHeight);
        l.uv.set((eF32)x/(eF32)texWidth, (eF32)y/(eF32)texHeight);

        ExtTextOut(compDc, x, y, 0, nullptr, text, 1, nullptr);
        x += l.width+border;
    }

    GetDIBits(compDc,bmp, 0, texHeight, &m_bmp[0], &bmi, DIB_RGB_COLORS);
    DeleteObject(font);
    DeleteObject(bmp);
    DeleteDC(compDc);
    DeleteDC(hdc);

    for (eU32 i=0; i<m_bmp.size(); i++)
        m_bmp[i].a = 255;

    if (!m_tex || m_tex->width != texWidth || m_tex->height != texHeight)
    {
        eGfx->removeTexture2d(m_tex);
        m_tex = eGfx->addTexture2d(texWidth, texHeight, eTEX_NOMIPMAPS, eTFO_ARGB8);
    }

    eGfx->updateTexture2d(m_tex, &m_bmp[0]);
}

eTexture2d * eFont::getTextureAtlas() const
{
    return m_tex;
}

eU32 eFont::getHeight() const
{
    return m_height;
}

eU32 eFont::getLetterCount() const
{
    return m_numLetters;
}

const eFontLetter & eFont::getLetter(eChar chr) const
{
    eASSERT(chr >= 0);
    eASSERT(m_letters[chr].used);
    return m_letters[chr];
}