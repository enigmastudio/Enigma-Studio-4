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


Texture2D s_texDiffuse: register(t0);
SamplerState s_ssDiffuse: register(s0);

struct ShaderInput
{
    float4 hpos:        SV_Position;// position (clip-space)
    float2 texCoord:    TEXCOORD0;  // texture coordinate
    float3 vpos:        TEXCOORD1;  // position (view-space)
    float3 normal:      TEXCOORD2;  // normal (view-space)
    float3 color:       COLOR0;     // color
};

float4 main(const ShaderInput input) : SV_Target0 
{
	return s_texDiffuse.Sample(s_ssDiffuse, input.texCoord)*float4(input.color, 1.0f);
}