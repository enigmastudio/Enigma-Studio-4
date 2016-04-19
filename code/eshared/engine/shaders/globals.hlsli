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

#define eCBI_CAMERA			b0
#define eCBI_LIGHT			b1
#define eCBI_MATERIAL		b2
#define eCBI_FX_PARAMS		b3
#define eCBI_PASS_AMBIENT	b4
#define eCBI_PASS_SHADOW    b5

#define ePI                 (3.14159265f)
#define eTWOPI              (2.0f*ePI)

#define h2 smoothstep 
#define h1 saturate 
#define h0 normalize 
#define g9 length 
#define g8 const 
#define g7 return 
#define g6	float2x3
#define g5	float3x3
#define g4	float4x4
#define g3	float4
#define g2	float3
#define g1	float2
#define g0	float

cbuffer Camera : register(eCBI_CAMERA)
{
    float4x4    c_viewMtx;
    float4x4    c_projMtx;
    float4x4    c_mvpMtx;
    float4x4	c_itViewMtx;
    float3      c_camWorldPos;
	float4		c_camClearColor;
};

cbuffer Light : register(eCBI_LIGHT)
{
    float3	    c_lightViewPos;
    float3      c_lightWorldPos;
    float3	    c_lightDiffuseCol;
    float3	    c_lightSpecCol;
    float       __pad__;
    float	    c_lightInvRange;
    float	    c_lightPenumbraSize;
    float       c_lightShadowBias;
};

cbuffer Material : register(eCBI_MATERIAL)
{
    float3	    c_matDiffuseCol;
    float3	    c_matSpecCol;
    float	    c_matShininess;
};

cbuffer AmbientPass : register(eCBI_PASS_AMBIENT)
{
	float3	    c_lightTotalAmbientCol;
};

cbuffer ShadowPass : register(eCBI_PASS_SHADOW)
{
    float	    c_camProjShadowScale; // scale and bias needed for projecting depth for shadows
    float       c_camProjShadowBias;
};

// calculates tangent space coordinate system
// on a per pixel basis in the pixel shader
float3x3 calcTangentFrame(const float3 normal, const float3 pos, const float2 texCoord)
{
    const float3 dp0 = ddx(pos);
    const float3 dp1 = ddy(pos);
    const float2 duv0 = ddx(texCoord);
    const float2 duv1 = ddy(texCoord);

    const float3x3 m = float3x3(dp0, dp1, cross(dp0, dp1));
    const float2x3 invM = float2x3(cross(m[1], m[2]), cross(m[2], m[0]));

    const float3 tangent = mul(float2(duv0.x, duv1.x), invM);
    const float3 binormal = mul(float2(duv0.y, duv1.y), invM);

    return float3x3(normalize(tangent), normalize(binormal), normal);
}

// normal decoding for normal maps:
// mapping from [0,1] -> [-1, 1]
float3 unpackNormalT(const float3 normal)
{
	return 2.0f*normal-1.0f;
}

float2 postProjectionToScreenSpace(const float4 hpos)
{
    const float2 screenPos = hpos.xy/hpos.w;
    return 0.5f*(float2(screenPos.x, -screenPos.y)+1.0f);
}

float schlickPow(const float base, const float exp)
{
    return base/(exp-base*exp+base);
}

float3 phong(const float3 surfaceNormal, const float3 lightPos, const float3 surfacePos,
             const float invLightRange, const float3 lightDiffCol, const float3 lightSpecCol,
             const float3 diffuseCol, const float3 specCol, const float shininess)
{
	float3 lightVec = lightPos-surfacePos;
    const float3 viewVec = normalize(-surfacePos);
    const float3 halfVec = normalize(lightVec+viewVec);
	
    const float lightVecLen = length(lightVec);
	float att = saturate(1.0f-lightVecLen*invLightRange);
	lightVec /= lightVecLen;
	
	const float3 diff = saturate(dot(lightVec, surfaceNormal))*lightDiffCol;
    const float3 spec = schlickPow(saturate(dot(halfVec, surfaceNormal)), shininess)*lightSpecCol;

	return att*(diffuseCol*diff+specCol*spec);
}

float grayScale(const float3 color)
{
	return dot(float3(0.3f, 0.59f, 0.11f), color);
}

float2 noise2d(float2 coord)
{
    float noiseX = clamp(frac(sin(dot(coord ,float2(12.9898,78.233))) * 43758.5453),0.0,1.0)*2.0-1.0;
    float noiseY = clamp(frac(sin(dot(coord ,float2(12.9898,78.233)*2.0)) * 43758.5453),0.0,1.0)*2.0-1.0;
    return float2(noiseX,noiseY);
}

// --------------------- START of SIMPLEX NOISE
// Description : Array and textureless HLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise

float2 mod289(float2 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float3 mod289(float3 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 mod289(float4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}
 
float3 permute(float3 x)
{
    return mod289(((x*34.0)+1.0)*x);
}

float4 permute(float4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

float snoise(float2 v)
{
    const float4 C = float4( 0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                             0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                            -0.577350269189626,  // -1.0 + 2.0 * C.x
                             0.024390243902439); // 1.0 / 41.0
    // First corner
    float2 i  = floor(v + dot(v, C.yy) );
    float2 x0 = v -   i + dot(i, C.xx);
 
    // Other corners
    float2 i1;
    //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
    //i1.y = 1.0 - i1.x;
    i1 = (x0.x > x0.y) ? float2(1.0, 0.0) : float2(0.0, 1.0);
    // x0 = x0 - 0.0 + 0.0 * C.xx ;
    // x1 = x0 - i1 + 1.0 * C.xx ;
    // x2 = x0 - 1.0 + 2.0 * C.xx ;
    float4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
 
    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    float3 p = permute( permute( i.y + float3(0.0, i1.y, 1.0 ))
                               + i.x + float3(0.0, i1.x, 1.0 ));
 
    float3 m = max(0.5 - float3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
    m = m*m ;
    m = m*m ;
 
    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
 
    float3 x = 2.0 * frac(p * C.www) - 1.0;
    float3 h = abs(x) - 0.5;
    float3 ox = floor(x + 0.5);
    float3 a0 = x - ox;
 
    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
 
    // Compute final noise value at P
    float3 g;
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}



float4 invsqrt(float4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(float3 v)
{
    const float2 C = float2(1.0/6.0, 1.0/3.0) ;
    const float4 D = float4(0.0, 0.5, 1.0, 2.0);

    float3 i = floor(v + dot(v, C.yyy) );
    float3 x0 = v - i + dot(i, C.xxx) ;
    
    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    float3 i1 = min( g.xyz, l.zxy );
    float3 i2 = max( g.xyz, l.zxy ); 

    float3 x1 = x0 - i1 + C.xxx;
    float3 x2 = x0 - i2 + C.yyy;
    float3 x3 = x0 - D.yyy;

    i = mod289(i); 
    float4 p = permute( permute( permute(
             i.z + float4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + float4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + float4(0.0, i1.x, i2.x, 1.0 ));

    float n_ = 0.142857142857;
    float3 ns = n_ * D.wyz - D.xzx;

    float4 j = p - 49.0 * floor(p * ns.z * ns.z);

    float4 x_ = floor(j * ns.z);
    float4 y_ = floor(j - 7.0 * x_ ); 

    float4 x = x_ *ns.x + ns.yyyy;
    float4 y = y_ *ns.x + ns.yyyy;
    float4 h = 1.0 - abs(x) - abs(y);

    float4 b0 = float4( x.xy, y.xy );
    float4 b1 = float4( x.zw, y.zw );

    float4 s0 = floor(b0)*2.0 + 1.0;
    float4 s1 = floor(b1)*2.0 + 1.0;
    float4 sh = -step(h, float4(0,0,0,0));

    float4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
    float4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

    float3 p0 = float3(a0.xy,h.x);
    float3 p1 = float3(a0.zw,h.y);
    float3 p2 = float3(a1.xy,h.z);
    float3 p3 = float3(a1.zw,h.w);

    float4 norm = invsqrt(float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    float4 m = max(0.6 - float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 42.0 * dot( m*m, float4( dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3) ) );
}

// --------------------- END of SIMPLEX NOISE

float2 rand(float2 coord, float2 texDim, bool noise) //generating noise/pattern texture for dithering
{
	if (noise)
	{
		return noise2d(coord);
	}
    else
    {
        float noiseX = ((frac(1.0-coord.x*(texDim.x/2.0))*0.25)+(frac(coord.y*(texDim.y/2.0))*0.75))*2.0-1.0;
	    float noiseY = ((frac(1.0-coord.x*(texDim.x/2.0))*0.75)+(frac(coord.y*(texDim.y/2.0))*0.25))*2.0-1.0;
        return float2(noiseX,noiseY);
    }
}

float hash( float n )
{
	return frac(sin(n)*43758.5453);
}

float noise( in float2 x )
{
	float2 p = floor(x);
	float2 f = frac(x);
    	f = f*f*(3.0-2.0*f);
    	float n = p.x + p.y*57.0;
    	float res = lerp(lerp( hash(n+  0.0), hash(n+  1.0),f.x), lerp( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y);
    	return res;
}
