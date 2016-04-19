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

Texture2D<float4>   input0: register(t0);
Texture2D<float4>   input1: register(t1);
RWTexture2D<float4> output: register(u0);

cbuffer Params : register(b0)
{
    uint    mode;
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint2 xy = threadId.xy;

    float3 colIn0 = input0[xy].rgb;
    float3 colAlpha = (mode <= 1 ? colIn0 : input1[xy].rgb);
    float a = grayScale(colAlpha);
    output[xy] = float4(colIn0.rgb, (mode == 0 || mode == 2 ? a : 1.0f-a));
}