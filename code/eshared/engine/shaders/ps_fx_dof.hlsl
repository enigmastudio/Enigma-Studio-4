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

Texture2D s_texTarget : register(t0);
Texture2D s_texPosG : register(t2);

SamplerState s_samTarget : register(s0);
SamplerState s_samPosG : register(s2);

struct ShaderInput
{
	float4 hpos:        SV_Position;
	float2 texCoord:    TEXCOORD0;
};

cbuffer Params : register(eCBI_FX_PARAMS)
{
    float c_focalDepth;
    float c_nearDofStart;
    float c_nearDofDist;
    float c_farDofStart;
    float c_farDofDist;
    float c_maxBlur;
    float c_threshBlur;

    int c_ringCount;
    int c_ringSamples;
    
    int c_vignettingActive;
    float c_vignettingOut;
    float c_vignettingIn;
    float c_vignettingFade;

    float c_highlightingThreshold;
    float c_highlightingGain;

    int c_noiseActive;
    float c_noiseAmount;
};

float vignette(float2 uv, float fstop, float c_vignettingOut, float c_vignettingIn, float c_vignettingFade)
{
	float dist = distance(uv, float2(0.5,0.5));
	dist = smoothstep(c_vignettingOut+(fstop/c_vignettingFade), c_vignettingIn+(fstop/c_vignettingFade), dist);
	return clamp(dist,0.0,1.0);
}

float calcBlurAmount(float depth)
{
	float a = depth-c_focalDepth; //focal plane
	float b = (a-c_farDofStart)/c_farDofDist; //far DoF
	float c = (-a-c_nearDofStart)/c_nearDofDist; //near Dof
	float blur = (a>0.0)?b:c;
    return clamp(blur, 0.0f, 1.0f);
}

float4 main(const ShaderInput input) : SV_TARGET 
{
    float fstop = 4.0;                  //f-stop value
    float bias = 0.5;                   //bokeh edge bias
    float fringe = 0.7;                 //bokeh chromatic aberration/fringing

    float2 texDim;
    s_texTarget.GetDimensions(texDim.x, texDim.y);
    float2 texel = float2(1.0/texDim.x,1.0/texDim.y);

	//scene depth calculation
	float depth = length(s_texPosG.Sample(s_samPosG, input.texCoord));
	
	//dof blur factor calculation
	float blur = calcBlurAmount(depth);
	
	// calculation of pattern for ditering
	float2 noise = rand(input.texCoord, texDim, c_noiseActive)*c_noiseAmount*blur;
	
	// getting blur x and y step factor
	float w = (1.0/texDim.x)*blur*c_maxBlur+noise.x;
	float h = (1.0/texDim.y)*blur*c_maxBlur+noise.y;
	
    int iii=0;

	// calculation of final color
	float4 src = s_texTarget.Sample(s_samTarget, input.texCoord);
	float3 col = src.rgb;
	if (blur >= 0.05)
	{
		texel *= fringe*blur;
		const float2 texely =float2(0.0,1.0)*texel;
		float s = 1.0;
		int c_ringCountSamples;
		
        float2 uvs[3] = { float2(0.0,1.0), float2(-0.866,-0.5), float2(0.866,-0.5) };
		for(int i = 0; i < 3; i++)
			uvs[i] *= texel;
		const float3 lumcoeff = float3(0.299,0.587,0.114);

		for (int i = 1; i <= c_ringCount; i++)
		{   
			c_ringCountSamples = i * c_ringSamples;
			float biasLerp = lerp(1.0,(float(i)) / (float(c_ringCount)), bias);
			float step = ePI * 2.0 / float(c_ringCountSamples);

			for (int j = 0 ; j < c_ringCountSamples ; j++)
			{
                float pw, ph;
                sincos((float)j*step, ph, pw);
                ph *= (float)i;
                pw *= (float)i;

                float2 coords = input.texCoord + float2(pw*w,ph*h);
	            float b0 = calcBlurAmount(length(s_texPosG.SampleLevel(s_samPosG, coords + texely, 0.0)));

                // apply color bleeding reduction
                if (abs(blur-b0) > c_threshBlur)// || abs(blur-b1) > c_threshBlur || abs(blur-b2) > c_threshBlur)
                    continue;

                float3 col2 = s_texTarget.SampleLevel(s_samTarget, coords + uvs[iii%3], 0.0).rgb;
                iii++;

	            float lum = dot(col2.rgb, lumcoeff);
	            float thresh = max((lum-c_highlightingThreshold)*c_highlightingGain, 0.0);
	
                col += (col2 + lerp(float3(0.0, 0.0, 0.0),col2,thresh*blur)) * biasLerp;
				s += biasLerp;   
			}
		}

		col /= s; // divide by sample count
	}
	
	if (c_vignettingActive)
		col *= vignette(input.texCoord, fstop, c_vignettingOut, c_vignettingIn, c_vignettingFade);

    return float4(col, src.a);
}
