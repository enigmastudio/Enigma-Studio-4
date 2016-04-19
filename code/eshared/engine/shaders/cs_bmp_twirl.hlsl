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
    float2  centerVal: packoffset(c0.x);
    float2  radiusVal: packoffset(c1.x);
    float   strength:  packoffset(c2.x);
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

    float2 center = centerVal*wh;
    float2 radius = radiusVal*wh;
    float rr = radius.x*radius.y;
    float2 d = (xy-center)/radius;
    float dist = length(d);

    if (dist < 1.0f)
    {
        float angle = (1.0f-sin(dist*ePI*0.5f))*strength;
        float2 pos = xy-center;

        float s, c;
        sincos(angle, s, c); 
        float2 newPos = float2(pos.x*c-pos.y*s, pos.x*s+pos.y*c)+center;

        output[xy] = input.SampleLevel(ss, (float2)newPos/(float2)wh, 0.0f);
    }
    else
        output[xy] = input[xy];
}