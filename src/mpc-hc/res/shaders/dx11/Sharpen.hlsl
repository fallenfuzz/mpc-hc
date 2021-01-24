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

#define val0 (2.0)
#define val1 (-0.125)
#define effect_width (1.6)

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float dx = effect_width * px;
	float dy = effect_width * py;

	float4 c1 = tex.Sample(samp, coord + float2(-dx, -dy)) * val1;
	float4 c2 = tex.Sample(samp, coord + float2(  0, -dy)) * val1;
	float4 c3 = tex.Sample(samp, coord + float2(-dx,   0)) * val1;
	float4 c4 = tex.Sample(samp, coord + float2( dx,   0)) * val1;
	float4 c5 = tex.Sample(samp, coord + float2(  0,  dy)) * val1;
	float4 c6 = tex.Sample(samp, coord + float2( dx,  dy)) * val1;
	float4 c7 = tex.Sample(samp, coord + float2(-dx, +dy)) * val1;
	float4 c8 = tex.Sample(samp, coord + float2(+dx, -dy)) * val1;
	float4 c9 = tex.Sample(samp, coord) * val0;

	float4 c0 = (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9);

	return c0;
}
