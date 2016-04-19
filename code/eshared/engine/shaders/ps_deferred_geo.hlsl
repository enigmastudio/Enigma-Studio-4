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

struct ShaderInput
{
    float4 hpos:        SV_Position;// position (clip-space)
    float2 texCoord:    TEXCOORD0;  // texture coordinate
    float3 vpos:        TEXCOORD1;  // position (view-space)
    float3 normal:      TEXCOORD2;  // normal (view-space), has to be normalized
    float3 color:       COLOR0;     // color
};

struct ShaderOutput
{
    float4 color0:      SV_Target0; // diffCol
    float4 color1:      SV_Target1; // normal
    float4 color2:      SV_Target2; // specular
    float4 color3:      SV_Target3; // position + material index
    float4 color4:      SV_Target4; // environment map
};

Texture2D s_texDiffuse: register(t0);
Texture2D s_texNormal:  register(t1);
Texture2D s_texSpec:    register(t2);
Texture2D s_texDepth:   register(t3);
Texture3D s_texEnv:     register(t4);

SamplerState s_samDiffuse:  register(s0);
SamplerState s_samNormal:   register(s1);
SamplerState s_samSpec:     register(s2);
SamplerState s_samDepth:    register(s3);
SamplerState s_samEnv:      register(s4);

ShaderOutput main(const ShaderInput input)
{
    // map normal from tangent-space to view-space
	const float3x3 tf = calcTangentFrame(input.normal, input.vpos, input.texCoord);
    float3 normal = s_texNormal.Sample(s_samNormal, input.texCoord).xyz;
    normal = normalize(mul(unpackNormalT(normal), tf));

    // read other samplers and output data
    const float3 diffCol = s_texDiffuse.Sample(s_samDiffuse, input.texCoord).rgb;
    const float3 specCol = s_texSpec.Sample(s_samSpec, input.texCoord).rgb;

    // envmapping
    const float3 view = normalize(input.vpos);
	const float3 envCoord = reflect(view, normal);
    const float3 envCol = s_texEnv.Sample(s_samEnv, envCoord).rgb;

    ShaderOutput output;

    output.color0 = float4(diffCol*c_matDiffuseCol*input.color, 1.0);
    output.color1 = float4(normal, 0.0);
    output.color2 = float4(specCol*c_matSpecCol, c_matShininess);
    output.color3 = float4(input.vpos, 0);
    output.color4 = float4(envCol, 1.0);

    return output;
}