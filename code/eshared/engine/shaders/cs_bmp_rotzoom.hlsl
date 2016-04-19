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
SamplerState        ss:     register(s0);

cbuffer Params : register(b0)
{
    float   angleVal:   packoffset(c0.x);
    float2  zoomVal:    packoffset(c1.x);
    float2  scrollVal:  packoffset(c2.x);
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

    float s, c;
    sincos(angleVal*eTWOPI, s, c);
    float2 zoom = float2(pow(0.05f, zoomVal.x-1.0f), pow(0.05f, zoomVal.y-1.0f));
    float2 scroll = scrollVal*wh;
    float u = ((-0.5f*w+x)*c+(0.5f*h-y)*s)*zoom.x+scroll.x;
    float v = ((-0.5f*w+x)*s-(0.5f*h-y)*c)*zoom.y+scroll.y;

    output[xy] = input.SampleLevel(ss, float2(u, v)/wh, 0.0f);
}