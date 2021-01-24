// $MinimumShaderProfile: ps_4_0

/* --- Settings --- */

#define ColorTone float3(1.40, 1.10, 0.90) //[0.00 to 2.55, 0.00 to 2.55, 0.00 to 2.55] What color to tint the image
#define GreyPower  0.11                    //[0.00 to 1.00] How much desaturate the image before tinting it
#define SepiaPower 0.58                    //[0.00 to 1.00] How much to tint the image

/* ---  Defining Constants --- */

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



/* ---  Main code --- */

/*------------------------------------------------------------------------------
						SEPIA
------------------------------------------------------------------------------*/

float4 SepiaPass( float4 colorInput )
{
	float3 sepia = colorInput.rgb;
	
	// calculating amounts of input, grey and sepia colors to blend and combine
	float grey = dot(sepia, float3(0.2126, 0.7152, 0.0722));
	
	sepia *= ColorTone;
	
	float3 blend2 = (grey * GreyPower) + (colorInput.rgb / (GreyPower + 1));

	colorInput.rgb = lerp(blend2, sepia, SepiaPower);
	
	// returning the final color
	return colorInput;
}


/* --- Main --- */

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target 
{
	float4 c0 = tex.Sample(samp, coord);

	c0 = SepiaPass(c0);
	return c0;
}