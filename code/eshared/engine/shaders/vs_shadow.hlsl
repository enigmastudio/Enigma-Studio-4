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

	float4	normalMtx0:	TEXCOORD5;	// per instance (model matrix)^(-1)^T
	float4	normalMtx1:	TEXCOORD6;
	float4	normalMtx2:	TEXCOORD7;
	float4	normalMtx3:	TEXCOORD8;
};

struct ShaderOutput 
{
	float4 hpos:        SV_Position;// position (clip space)
	float3 lightVec:    TEXCOORD0;  // vector from vertex to light (world spcae)
};

ShaderOutput main(const ShaderInput input)
{
    const float4x4 modelMtx = float4x4(input.modelMtx0, input.modelMtx1, input.modelMtx2, input.modelMtx3);
    const float4 wpos = mul(input.pos, modelMtx);
	
    ShaderOutput output = (ShaderOutput)0;

    output.hpos = mul(mul(wpos, c_viewMtx), c_projMtx);
	output.lightVec = wpos.xyz-c_lightWorldPos;

	return output;
}