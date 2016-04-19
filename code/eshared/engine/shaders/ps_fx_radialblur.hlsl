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

Texture2D s_texTarget: register(t0);
SamplerState s_ssTarget: register(s0);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float2          c_origin;
    float           c_distance;
    float           c_strength;
	int				c_steps;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
    float2 dir = c_origin-input.texCoord;
    const float dist = length(dir);
    dir = dir/dist*c_distance;

    const float4 color = s_texTarget.Sample(s_ssTarget, input.texCoord);
    
    float4 sum = color;
    for (int i=0; i<c_steps; i++)
        sum += s_texTarget.Sample(s_ssTarget, input.texCoord + dir * (float(i-c_steps/2) / 1000));
	
    sum *= (1.0f/(c_steps*1.4));
    return float4(lerp(color, sum, saturate(dist*c_strength)).rgb, color.a);
}