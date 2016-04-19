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
    float4 pos:         POSITION0;	// position (object space)
    float3 normal:      NORMAL;		// normal (object space)
    float2 texCoord:    TEXCOORD0;	// texture coordinates
    float3 color:       COLOR0;		// color

	float4	modelMtx0:	TEXCOORD1;	// per instance model matrix
	float4	modelMtx1:  TEXCOORD2;
	float4	modelMtx2:  TEXCOORD3;
	float4	modelMtx3:  TEXCOORD4;

	float3	normalMtx0:	TEXCOORD5;	// per instance (model matrix)^(-1)^T
	float3	normalMtx1:	TEXCOORD6;
	float3	normalMtx2:	TEXCOORD7;
};

struct ShaderOutput
{
    float4 hpos:        SV_Position;// position (clip-space)
    float2 texCoord:    TEXCOORD0;	// texture coordinate
    float3 vpos:        TEXCOORD1;	// position (view-space)
    float3 normal:      TEXCOORD2;	// normal (view space)
    float3 color:       COLOR0;		// color
};

ShaderOutput main(ShaderInput input)
{
	const float4x4 modelMtx = float4x4(input.modelMtx0, input.modelMtx1, input.modelMtx2, input.modelMtx3);
	const float3x3 normalMtx = float3x3(input.normalMtx0, input.normalMtx1, input.normalMtx2);

    const float4 wpos = mul(input.pos, modelMtx);
    const float4 vpos = mul(wpos, c_viewMtx);
    const float4 hpos = mul(vpos, c_projMtx);

	ShaderOutput output = (ShaderOutput)0;

	output.hpos		= hpos;
	output.vpos		= vpos.xyz;
	output.normal	= normalize(mul(float4(mul(input.normal, normalMtx), 1.0f), c_itViewMtx).xyz);
	output.texCoord = input.texCoord;
    output.color	= input.color;

   	return output;
}