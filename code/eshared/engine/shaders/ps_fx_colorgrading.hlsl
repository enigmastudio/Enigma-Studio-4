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


Texture2D s_texSource: register(t0);
Texture3D s_texLookupMap: register(t4);

SamplerState s_ssSource: register(s0);
SamplerState s_ssLookupMap: register(s4);

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
	const float4 src = s_texSource.Sample(s_ssSource, input.texCoord);
    const float3 color = saturate(src.rgb);
    const float3 final = s_texLookupMap.SampleLevel(s_ssLookupMap, color, 0.0).xyz;
    return float4(final, src.a);
}