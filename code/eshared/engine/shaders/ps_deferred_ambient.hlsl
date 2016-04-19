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

Texture2D    s_texDiffuse:   register(t0);
Texture2D    s_texEnv:       register(t4);
Texture2D    s_texPositionG: register(t3);

SamplerState s_samDiffuse:    register(s0);
SamplerState s_samEnv:        register(s4);
SamplerState s_samPositionG : register(s3);

struct ShaderInput 
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

float4 main(const ShaderInput input) : SV_Target0
{
	const float4 diffCol = s_texDiffuse.Sample(s_samDiffuse, input.texCoord);
	const float3 envCol = s_texEnv.Sample(s_samEnv, input.texCoord).rgb;
	const float3 pos = s_texPositionG.Sample(s_samPositionG, input.texCoord).xyz;
	
    return float4(envCol, length(pos)) + diffCol*float4(c_lightTotalAmbientCol, 0.0f);
}