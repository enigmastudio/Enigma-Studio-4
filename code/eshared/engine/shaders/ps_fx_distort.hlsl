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

Texture2D    s_texTarget:     register(t0);
Texture2D    s_texDistortMap: register(t4);
SamplerState s_ssTarget:      register(s0);
SamplerState s_ssDistortMap:  register(s4);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float2  c_intensity;
    float2  c_offset;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
	const float2 coord = input.texCoord+c_offset;
    const float2 dist = unpackNormalT(s_texDistortMap.Sample(s_ssDistortMap, coord).xyz).xy;
	const float4 col = s_texTarget.Sample(s_ssTarget, input.texCoord+dist*c_intensity);
    return col;
}