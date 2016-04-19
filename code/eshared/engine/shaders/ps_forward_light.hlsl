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

Texture2D s_texDiffuseMap:   register(t0);
Texture2D s_texNormalMap:    register(t1);
Texture2D s_texSpecMap:      register(t2);

SamplerState s_ssDiffuseMap: register(s0);
SamplerState s_ssNormalMap:  register(s1);
SamplerState s_ssSpecMap:    register(s2);

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
    // map normal from tangent-space to view-space
	const float3x3 tf = calcTangentFrame(input.normal, input.vpos, input.texCoord);
    float3 normal = s_texNormalMap.Sample(s_ssNormalMap, input.texCoord).xyz;
    normal = normalize(mul(unpackNormalT(normal), tf));

    // read other samplers and output color
	const float4 diffCol = s_texDiffuseMap.Sample(s_ssDiffuseMap, input.texCoord)*float4(c_matDiffuseCol*input.color, 1.0f);
	const float4 specCol = s_texSpecMap.Sample(s_ssSpecMap,input.texCoord)*float4(c_matSpecCol, 1.0f);

    // evaluate lighting model
    const float3 c = phong(normal, c_lightViewPos, input.vpos, c_lightInvRange, c_lightDiffuseCol,
                           c_lightSpecCol, diffCol.xyz, specCol.xyz, c_matShininess);

    return float4(c_lightTotalAmbientCol.xyz+c, diffCol.a);
}