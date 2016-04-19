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
    float   brightness: packoffset(c0.x);
    float   contrast:   packoffset(c1.x);
    float   hue:        packoffset(c2.x);
    float   saturation: packoffset(c3.x);
};

// HSV<->RGB conversion routines taken from Ian Taylor's blog:
// http://chilliant.blogspot.de/2010/11/rgbhsv-in-hlsl.html

float3 HsvToRgb(float3 hsv)
{
    float h = hsv.x;
    float3 rgb = saturate(float3(abs(h*6-3)-1, 2-abs(h*6-2), 2-abs(h*6-4)));
    return ((rgb-1.0f)*hsv.y+1.0f)*hsv.z;
}

float3 RgbToHsv(float3 rgb)
{
    float3 hsv = float3(0.0f, 0.0f, max(rgb.r, max(rgb.g, rgb.b)));
    float m = min(rgb.r, min(rgb.g, rgb.b));
    float c = hsv.z-m;

    if (c != 0)
    {
        hsv.y = c/hsv.z;
        float3 delta = (hsv.z-rgb)/c;
        delta.rgb -= delta.brg;
        delta.rg += float2(2, 4);

        if (rgb.r >= hsv.z)
            hsv.x = delta.b;
        else if (rgb.g >= hsv.z)
            hsv.x = delta.r;
        else
            hsv.x = delta.g;

        hsv.x = frac(hsv.x/6);
    }

    return hsv;
}

[numthreads(8, 8, 1)]
void main(uint3 threadId: SV_DispatchThreadID)
{
    uint x = threadId.x;
    uint y = threadId.y;
    uint2 xy = int2(x, y);

    float4 col = input[xy];
    float3 tmp = saturate(((col.rgb-0.5f)*contrast+0.5f)*brightness);
    float3 hsv = RgbToHsv(tmp);
    hsv.x = saturate(hsv.x+hue-1.0f);
    hsv.y = saturate(hsv.y+saturation-1.0f);
    output[xy] = float4(HsvToRgb(hsv), col.a);
}