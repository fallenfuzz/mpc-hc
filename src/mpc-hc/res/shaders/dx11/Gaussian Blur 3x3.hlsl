// $MinimumShaderProfile: ps_4_0
/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2013 see Authors.txt
 *
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
 *
 */

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float  px;
	float  py;
	float2 wh;
	uint   counter;
	float  clock;
};

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	// Pixels definition: original, blurred
	float4 orig;
	float4 blurred;

	// Get neighbor points
	// [ 1,    2, 3 ]
	// [ 4, orig, 5 ]
	// [ 6,    7, 8 ]

	orig = tex.Sample(samp, coord);
	float4 c1 = tex.Sample(samp, coord + float2(-px, -py));
	float4 c2 = tex.Sample(samp, coord + float2(  0, -py));
	float4 c3 = tex.Sample(samp, coord + float2( px, -py));
	float4 c4 = tex.Sample(samp, coord + float2(-px,   0));
	float4 c5 = tex.Sample(samp, coord + float2( px,   0));
	float4 c6 = tex.Sample(samp, coord + float2(-px,  py));
	float4 c7 = tex.Sample(samp, coord + float2(  0,  py));
	float4 c8 = tex.Sample(samp, coord + float2( px,  py));

	// Computation of the blurred image (gaussian filter)
	// to normalize the values, we need to divide by the coeff sum
	// 1/(1+2+1+2+4+2+1+2+1) = 1/16 = 0.0625
	blurred = (c1 + c3 + c6 + c8 + 2 * (c2 + c4 + c5 + c7) + 4 * orig) * 0.0625;
	return blurred;
}
