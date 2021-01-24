// $MinimumShaderProfile: ps_4_0
/*
 * (C) 2008-2013 see Authors.txt
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

// YV12 chroma upsampling fixer by Kurt Bernhard 'Leak' Pruenner

// Use with YV12 output if the half-resolution chroma gets upsampled
// in hardware by doubling the values instead of interpolating between them
// (i.e. if you're getting blocky red edges on dark backgrounds...)

Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer PS_CONSTANTS : register(b0)
{
	float2 pxy;
	float  width;
	float  height;
	uint   counter;
	float  clock;
};

float4 getPixel(float2 coord, float dx, float dy)
{
	coord.x += dx;
	coord.y += dy;

	return tex.Sample(samp, coord);
}

float4 rgb2yuv(float4 rgb)
{
	float4x4 coeffs = {
		 0.299,  0.587,  0.114, 0.000,
		-0.147, -0.289,  0.436, 0.000,
		 0.615, -0.515, -0.100, 0.000,
		 0.000,  0.000,  0.000, 0.000
	};

	return mul(coeffs, rgb);
}

float4 yuv2rgb(float4 yuv)
{
	float4x4 coeffs = {
		1.000,  0.000,  1.140, 0.000,
		1.000, -0.395, -0.581, 0.000,
		1.000,  2.032,  0.000, 0.000,
		0.000,  0.000,  0.000, 0.000
	};

	return mul(coeffs, yuv);
}

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target
{
	float dx = 1 / width;
	float dy = 1 / height;

	float4 yuv00 = rgb2yuv(getPixel(coord, -dx, -dy));
	float4 yuv01 = rgb2yuv(getPixel(coord, -dx,   0));
	float4 yuv02 = rgb2yuv(getPixel(coord, -dx,  dy));
	float4 yuv10 = rgb2yuv(getPixel(coord,   0, -dy));
	float4 yuv11 = rgb2yuv(getPixel(coord,   0,   0));
	float4 yuv12 = rgb2yuv(getPixel(coord,   0,  dy));
	float4 yuv20 = rgb2yuv(getPixel(coord,  dx, -dy));
	float4 yuv21 = rgb2yuv(getPixel(coord,  dx,   0));
	float4 yuv22 = rgb2yuv(getPixel(coord,  dx,  dy));

	float4 yuv =
		(yuv00 * 1 + yuv01 * 2 + yuv02 * 1 +
		 yuv10 * 2 + yuv11 * 4 + yuv12 * 2 +
		 yuv20 * 1 + yuv21 * 2 + yuv22 * 1) / 16;

	yuv.r = yuv11.r;

	return yuv2rgb(yuv);
}
