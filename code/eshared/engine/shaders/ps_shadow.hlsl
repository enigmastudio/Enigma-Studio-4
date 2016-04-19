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

TextureCube g_texIndirMap:  register(t0);
Texture2D   g_texDistMap:   register(t1);

SamplerState g_ssIndirMap:  register(s0);
//[
SamplerComparisonState g_ssDistMap : register(s1)
{
   Filter           = MIN_MAG_LINEAR_MIP_POINT;
   AddressU         = MIRROR;
   AddressV         = MIRROR;
   ComparisonFunc   = LESS;
   ComparisonFilter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
};
//]
struct ShaderInput 
{
	float4  hpos: SV_Position;
    float3  lightVec: TEXCOORD0;
};

float main(const ShaderInput input) : SV_Target0
{
    // transform depth from A to B space
    const float2 uv    = g_texIndirMap.Sample(g_ssIndirMap, input.lightVec).xy;
	const float3 absLv = abs(input.lightVec);
    const float ma     = max(max(absLv.x, absLv.y), absLv.z);
    const float depth  = (-1.0f/ma) * c_camProjShadowScale + c_camProjShadowBias - c_lightShadowBias;

    float4 tapDiag, tapHv;

	tapDiag.x = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2( 1,  1)).x;
	tapDiag.y = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2(-1,  1)).x;
	tapDiag.z = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2( 1, -1)).x;
	tapDiag.w = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2(-1, -1)).x;

	tapHv.x = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2( 1,  0)).x;
	tapHv.y = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2(-1,  0)).x;
	tapHv.z = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2( 0, -1)).x;
	tapHv.w = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth, int2( 0, -1)).x;

	const float tapCenter = g_texDistMap.SampleCmpLevelZero(g_ssDistMap, uv, depth).x;
	const float occlusion = dot(tapDiag+tapHv+float4(tapCenter, 0.0f, 0.0f, 0.0f), 1.0f/9.0f);

	return 1.0f-occlusion;
}