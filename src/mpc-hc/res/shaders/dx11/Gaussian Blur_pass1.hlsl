// $MinimumShaderProfile: ps_4_0
/*
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

(dx11) Gaussian Blur_pass1: (gBlur7, Horizontal pass) by butterw
First pass of 2-pass shader. As with all blur shaders, use pre-upscaling for maximum effect !  
uses a separable Gaussian Kernel, without hw linear sampling optimization. 
Perf per pass: (7 texture, 11 arithmetic), 26/24 instruction slots used

This 2-pass shader can be run multiple times to achieve a stronger blur (ex: 3 times).

Tested in mpc-hc v1.9.8.38  
- loading xxx_pass1.hlsl now automatically loads xxx_pass2.hlsl

The code can be modified to use different kernel parameters. Please ensure the parameters are the same between both passes.
default: 7-tap Gaussian Kernel, sigma=1.5 
http://dev.theomader.com/gaussian-kernel-calculator/
*/
//#define K	float4(0.324225, 0.233638, 0.087348, 0.016901) //sigma=1.2
#define K	float4(0.266346, 0.215007, 0.113085, 0.038735) //sigma=1.5 (default) 
//#define K	float4(0.230781, 0.198557, 0.126451, 0.059602) //sigma=1.8
//#define K	float4(	0.214607, 0.189879, 0.131514, 0.071303) //sigma=2.0

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float  px;
};

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
    float4 c0 = K[0] * tex.Sample(samp, coord);
    c0 += K[1] * tex.Sample(samp, coord + float2(   px, 0));
    c0 += K[1] * tex.Sample(samp, coord + float2(  -px, 0));
    c0 += K[2] * tex.Sample(samp, coord + float2( 2*px, 0));
    c0 += K[2] * tex.Sample(samp, coord + float2(-2*px, 0));
    c0 += K[3] * tex.Sample(samp, coord + float2( 3*px, 0));
    c0 += K[3] * tex.Sample(samp, coord + float2(-3*px, 0));	
    return c0;
}