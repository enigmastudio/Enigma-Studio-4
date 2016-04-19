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
RWTexture2D<float4> output: register(u0);

cbuffer Params : register(b0)
{
    float2  relCenter:  packoffset(c0.x);
    float2  relRadius:  packoffset(c1.x);
    float4  col:        packoffset(c2.x);
    int     alpha:      packoffset(c3.x);
    int     gammaVal:   packoffset(c4.x);
};

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint x = threadId.x;
    uint y = threadId.y;
    uint2 xy = int2(x, y);
    uint w, h;
    output.GetDimensions(w, h);
    uint2 wh = uint2(w, h);

    float  gamma = pow((float)gammaVal/255.0f*2.0f, 3.0f);
    float  agadj = (float)alpha/pow(255.0f, gamma)/255.0f;
    float2 center = relCenter*wh;
    float2 radius = 256.0f/(relRadius*wh);
        
    float2 d = (center-xy)*radius;
    float p = 255.0f-min(sqrt(d.x*d.x+d.y*d.y), 255.0f);
    float q = saturate(pow((float)p, gamma)*agadj);

    output[xy] = lerp(input[xy], col, q);
}