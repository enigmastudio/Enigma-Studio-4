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

Texture2D<float4>   input:  register(t0);
Texture2D<float4>   normal: register(t1);
RWTexture2D<float4> output: register(u0);

cbuffer Params : register(b0)
{
    float4  ambientCol: packoffset(c0.x);
    float4  diffuseCol: packoffset(c1.x);
    float4  specCol:    packoffset(c2.x);
    float3  posVal:     packoffset(c3.x);
    float   specAmount: packoffset(c4.x);
    float   bumpAmount: packoffset(c5.x);
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint x = threadId.x;
    uint y = threadId.y;
    uint2 xy = int2(x, y);

    float3 pos = posVal;
    pos.x -= 0.5f;
    pos.y = -pos.y+0.5f;
    pos.z = pos.z-0.5f;

    float3 n = normal[xy].rgb;
    n *= 2.0f;
    n -= 1.0f;
    n = normalize(n);

    // compute the angle between normal and light
    // position. angle is scaled by bump amount
    float angle = max(0.0f, dot(n, pos)*bumpAmount);

    // evaluate phong model:
    // color = ambientCol+diffuseCol*angle+angle^2*specCol
    float r = ambientCol.r+angle*(diffuseCol.r+angle*specCol.r*specAmount);
    float g = ambientCol.g+angle*(diffuseCol.g+angle*specCol.g*specAmount);
    float b = ambientCol.b+angle*(diffuseCol.b+angle*specCol.b*specAmount);

    float4 inCol = input[xy];
    output[xy] = float4(float3(r, g, b)*inCol.rgb, inCol.a);
}