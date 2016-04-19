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


Texture2D s_texColor : register(t0);
Texture2D s_texPosG : register(t2);

SamplerState s_samColor : register(s0);
SamplerState s_samPosG : register(s2);

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float4x4        c_invViewMatrix;
	float4          c_camFov;

	float           c_time;
    float           c_sphereRadius;
};

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

// intersect sphere in the origin
bool iSphere( float3 ro, float3 rd, float rad, out float i1, out float i2 )
{
    float b = 2*dot( ro, rd );
    float c = dot(ro, ro) - rad*rad;
    float h = b*b - 4*c;
    if (h<0)
		return false;

    h = sqrt(h);
    i1 = (-b - h) * 0.5;
    i2 = (-b + h) * 0.5;
    
    // are both intersections behind us?
    float m = max(i1,i2);
    if (m<0)
        return false;

    return true;
}

float4 main(const ShaderInput input) : SV_Target0
{
	float3 p = float3(2*input.texCoord-1, 1) * c_camFov;
    float3 rd = normalize(mul(p,(float3x3)c_invViewMatrix));
    float3 ro = c_invViewMatrix[3].xyz;

	float t1, t2;
	float z1 = 0, z2 = 0;
	float3 col = float3(1,0,0);

	if (iSphere(ro, rd, c_sphereRadius, t1, t2))
	{
		float3 pos = ro + rd*t1;
		z1 = t1;
		float3 nor = normalize(pos);
		col = float3(1,1,1) * dot(nor, float3(0,1,0));
	}

    const float4 pos = float4(s_texPosG.Sample(s_samPosG, input.texCoord).xyz, 1.0);
    const float4 worldPos = mul(pos, c_invViewMatrix);
    z2 = length(worldPos-ro);

	if (z1>0 && z2>0)
	{
		if(z1>z2)
			return float4(s_texColor.SampleLevel(s_samColor, input.texCoord, 0.0).xyz, 1.0f);

		return float4(col, 1.0f);
	}
	
	if (z2>0)
		return float4(s_texColor.SampleLevel(s_samColor, input.texCoord, 0.0).xyz, 1.0f);

    return float4(col, 1.0f);
}









