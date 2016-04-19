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

Texture2D s_texTarget : register(t0);
Texture2D s_texPosMap : register(t2);

SamplerState s_samTarget : register(s0);
SamplerState s_samPosMap : register(s2);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    int             c_type;
    float           c_start;
    float           c_end;
    float           c_density;
    float3          c_color;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

static const int TYPE_LINEAR = 0;
static const int TYPE_EXP    = 1;
static const int TYPE_EXPSQR = 2;

float linearFog(const float start, const float end, const float depth)
{
    return saturate((end-depth)/(end-start));
}

float expFog(const float density, const float depth)
{
    return exp(-depth*density);
}

float expSqrFog(const float density, const float depth)
{
    const float e = depth*density;
    return exp(-e*e);
}

float4 main(const ShaderInput input) : SV_Target0
{
    const float4 col = s_texTarget.Sample(s_samTarget, input.texCoord);
    const float4 pos = s_texPosMap.Sample(s_samPosMap, input.texCoord);
    const float depth = length(pos);

    float f = 0.0f;

    if (c_type == TYPE_LINEAR)
        f = linearFog(c_start, c_end, depth);
    else if (c_type == TYPE_EXP)
        f = expFog(c_density, depth);
    else if (c_type == TYPE_EXPSQR)
        f = expSqrFog(c_density, depth);
    
    return lerp(float4(c_color, col.a), col, f);
}