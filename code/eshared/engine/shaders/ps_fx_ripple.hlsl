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
    float2  c_offset;
    float   c_amplification;
    float   c_length;
    float   c_speed;
    float   c_time;
    uint    c_mode;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

static const uint  MODE_STANDARD   = 0;
static const uint  MODE_CONCENTRIC = 1; 

float4 main(const ShaderInput input) : SV_Target0
{
    float2 uv = input.texCoord;

    if (c_mode == MODE_STANDARD)
        uv += sin(uv * c_length + c_time * c_speed + c_offset) * c_amplification;	
    else if (c_mode == MODE_CONCENTRIC)
        uv += sin(length(uv - c_offset) * c_length + c_time * c_speed) * c_amplification;

    return s_texTarget.Sample(s_ssTarget, uv);
}