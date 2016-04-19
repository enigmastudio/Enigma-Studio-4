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

#include "globals.hlsli"

Texture2D s_texSource0: register(t0);
Texture2D s_texSource1: register(t1);
SamplerState s_ssSource0: register(s0);
SamplerState s_ssSource1: register(s1);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float4  vpSrc0;
    float4  vpSrc1;
    float4  clearCol;
    int     mergeMode;
    float2  blendRatios;
	int		depthTestOn;
};

struct ShaderInput
{
	float4 hpos:    SV_Position;
	float2 uv:      TEXCOORD0;
};

static const int MODE_ADD      = 0;
static const int MODE_SUB      = 1;
static const int MODE_MUL      = 2;
static const int MODE_BRIGHTER = 3;
static const int MODE_DARKER   = 4;
static const int MODE_NONE     = 5;

float4 getPixel(float2 uv, Texture2D tex, SamplerState ss, float4 vp)
{
    if (uv.x < vp.x || uv.x > vp.z || uv.y < vp.y || uv.y > vp.w)
        return clearCol;

    const float2 uv2 = float2(uv.x-vp.x, uv.y-vp.y)/float2(vp.z-vp.x, vp.w-vp.y);
    return tex.Sample(ss, uv2);
}

float4 main(const ShaderInput input) : SV_Target0
{
    float4 col0 = getPixel(input.uv, s_texSource0, s_ssSource0, vpSrc0);
    float4 col1 = getPixel(input.uv, s_texSource1, s_ssSource1, vpSrc1);
    const float ratio0 = blendRatios.x;
    const float ratio1 = blendRatios.y;
	
	float4 result = 0;

    if (mergeMode == MODE_ADD)
        result = ratio0*col0+ratio1*col1;
    else if (mergeMode == MODE_SUB)
        result = ratio0*col0-ratio1*col1;
    else if (mergeMode == MODE_MUL)
        result = ratio0*col0*ratio1*col1;
    else if (mergeMode == MODE_BRIGHTER)
        result = (grayScale(col0.rgb) > grayScale(col1.rgb) ? col0 : col1);
    else if (mergeMode == MODE_DARKER)
        result = (grayScale(col0.rgb) < grayScale(col1.rgb) ? col0 : col1);
    else if (mergeMode == MODE_NONE)
        result = ratio0*col0*(1.0f-col1.a)+ratio1*col1;

    return result;
}