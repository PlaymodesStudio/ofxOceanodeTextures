R"(
#version 410

// Blend mode functions - simplified and optimized
#define BlendAddf(base, blend) min(base + blend, 1.0)
#define BlendSubstractf(base, blend) max(base + blend - 1.0, 0.0)
#define BlendLightenf(base, blend) max(blend, base)
#define BlendDarkenf(base, blend) min(blend, base)
#define BlendScreenf(base, blend) (1.0 - ((1.0 - base) * (1.0 - blend)))
#define BlendOverlayf(base, blend) mix(2.0 * base * blend, 1.0 - 2.0 * (1.0 - base) * (1.0 - blend), step(0.5, base))
#define BlendSoftLightf(base, blend) mix(2.0 * base * blend + base * base * (1.0 - 2.0 * blend), sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend), step(0.5, blend))
#define BlendColorDodgef(base, blend) ((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0))
#define BlendColorBurnf(base, blend) ((blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0))
#define BlendVividLightf(base, blend) mix(BlendColorBurnf(base, 2.0 * blend), BlendColorDodgef(base, 2.0 * (blend - 0.5)), step(0.5, blend))
#define BlendPinLightf(base, blend) mix(BlendDarkenf(base, 2.0 * blend), BlendLightenf(base, 2.0 * (blend - 0.5)), step(0.5, blend))
#define BlendHardMixf(base, blend) step(0.5, BlendVividLightf(base, blend))
#define BlendReflectf(base, blend) ((blend == 1.0) ? blend : min(base * base / (1.0 - blend), 1.0))

// Vector blend functions
#define Blend(base, blend, funcf) vec3(funcf(base.r, blend.r), funcf(base.g, blend.g), funcf(base.b, blend.b))
#define BlendMultiply(base, blend) (base * blend)
#define BlendAverage(base, blend) ((base + blend) * 0.5)
#define BlendAdd(base, blend) min(base + blend, vec3(1.0))
#define BlendSubstract(base, blend) max(base + blend - vec3(1.0), vec3(0.0))
#define BlendDifference(base, blend) abs(base - blend)
#define BlendNegation(base, blend) (vec3(1.0) - abs(vec3(1.0) - base - blend))
#define BlendExclusion(base, blend) (base + blend - 2.0 * base * blend)
#define BlendScreen(base, blend) Blend(base, blend, BlendScreenf)
#define BlendOverlay(base, blend) Blend(base, blend, BlendOverlayf)
#define BlendSoftLight(base, blend) Blend(base, blend, BlendSoftLightf)
#define BlendHardLight(base, blend) BlendOverlay(blend, base)
#define BlendColorDodge(base, blend) Blend(base, blend, BlendColorDodgef)
#define BlendColorBurn(base, blend) Blend(base, blend, BlendColorBurnf)
#define BlendVividLight(base, blend) Blend(base, blend, BlendVividLightf)
#define BlendPinLight(base, blend) Blend(base, blend, BlendPinLightf)
#define BlendHardMix(base, blend) Blend(base, blend, BlendHardMixf)
#define BlendReflect(base, blend) Blend(base, blend, BlendReflectf)
#define BlendGlow(base, blend) BlendReflect(blend, base)
#define BlendPhoenix(base, blend) (min(base, blend) - max(base, blend) + vec3(1.0))

// HSL conversion functions
vec3 RGBToHSL(vec3 color)
{
	vec3 hsl;
	float fmin = min(min(color.r, color.g), color.b);
	float fmax = max(max(color.r, color.g), color.b);
	float delta = fmax - fmin;
	hsl.z = (fmax + fmin) * 0.5;
	
	if (delta == 0.0)
	{
		hsl.xy = vec2(0.0);
	}
	else
	{
		hsl.y = delta / mix(fmax + fmin, 2.0 - fmax - fmin, step(0.5, hsl.z));
		vec3 deltaRGB = (((vec3(fmax) - color) / 6.0) + (delta * 0.5)) / delta;
		hsl.x = (color.r == fmax) ? deltaRGB.b - deltaRGB.g :
				(color.g == fmax) ? (1.0 / 3.0) + deltaRGB.r - deltaRGB.b :
								   (2.0 / 3.0) + deltaRGB.g - deltaRGB.r;
		hsl.x = fract(hsl.x);
	}
	return hsl;
}

float HueToRGB(float f1, float f2, float hue)
{
	hue = fract(hue);
	return mix(f1, mix(f2, mix(f1 + (f2 - f1) * 6.0 * hue, f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0, step(2.0/3.0, hue)), step(0.5, hue)), step(1.0/6.0, hue));
}

vec3 HSLToRGB(vec3 hsl)
{
	if (hsl.y == 0.0)
		return vec3(hsl.z);
	
	float f2 = mix(hsl.z * (1.0 + hsl.y), (hsl.z + hsl.y) - (hsl.y * hsl.z), step(0.5, hsl.z));
	float f1 = 2.0 * hsl.z - f2;
	
	return vec3(
		HueToRGB(f1, f2, hsl.x + (1.0/3.0)),
		HueToRGB(f1, f2, hsl.x),
		HueToRGB(f1, f2, hsl.x - (1.0/3.0))
	);
}

// HSL blend functions
vec3 BlendHue(vec3 base, vec3 blend)
{
	vec3 baseHSL = RGBToHSL(base);
	baseHSL.x = RGBToHSL(blend).x;
	return HSLToRGB(baseHSL);
}

vec3 BlendSaturation(vec3 base, vec3 blend)
{
	vec3 baseHSL = RGBToHSL(base);
	baseHSL.y = RGBToHSL(blend).y;
	return HSLToRGB(baseHSL);
}

vec3 BlendColor(vec3 base, vec3 blend)
{
	vec3 blendHSL = RGBToHSL(blend);
	blendHSL.z = RGBToHSL(base).z;
	return HSLToRGB(blendHSL);
}

vec3 BlendLuminosity(vec3 base, vec3 blend)
{
	vec3 baseHSL = RGBToHSL(base);
	baseHSL.z = RGBToHSL(blend).z;
	return HSLToRGB(baseHSL);
}

uniform sampler2D base;
uniform sampler2D blendTgt;
uniform int mode;
uniform float opacity;
out vec4 fragColor;

void main()
{
	vec2 coord = gl_FragCoord.xy;
	vec4 baseCol = texelFetch(base, ivec2(coord), 0);
	vec4 blendCol = texture(blendTgt, coord / vec2(textureSize(base, 0)));
	
	// Calculate blend alpha (layer alpha * opacity)
	float blendAlpha = blendCol.a * opacity;
	
	// If blend layer is fully transparent, return base
	if (blendAlpha == 0.0)
	{
		fragColor = baseCol;
		return;
	}
	
	vec3 result;
	
	// Apply blend mode to RGB channels
	switch(mode)
	{
		case 0: result = blendCol.rgb; break; // Normal
		case 1: result = BlendMultiply(baseCol.rgb, blendCol.rgb); break;
		case 2: result = BlendAverage(baseCol.rgb, blendCol.rgb); break;
		case 3: result = BlendAdd(baseCol.rgb, blendCol.rgb); break;
		case 4: result = BlendSubstract(baseCol.rgb, blendCol.rgb); break;
		case 5: result = BlendDifference(baseCol.rgb, blendCol.rgb); break;
		case 6: result = BlendNegation(baseCol.rgb, blendCol.rgb); break;
		case 7: result = BlendExclusion(baseCol.rgb, blendCol.rgb); break;
		case 8: result = BlendScreen(baseCol.rgb, blendCol.rgb); break;
		case 9: result = BlendOverlay(baseCol.rgb, blendCol.rgb); break;
		case 10: result = BlendSoftLight(baseCol.rgb, blendCol.rgb); break;
		case 11: result = BlendHardLight(baseCol.rgb, blendCol.rgb); break;
		case 12: result = BlendColorDodge(baseCol.rgb, blendCol.rgb); break;
		case 13: result = BlendColorBurn(baseCol.rgb, blendCol.rgb); break;
		case 14: // LinearLight
			result = mix(BlendColorBurn(baseCol.rgb, 2.0 * blendCol.rgb), 
						BlendColorDodge(baseCol.rgb, 2.0 * (blendCol.rgb - 0.5)), 
						step(0.5, blendCol.rgb));
			break;
		case 15: result = BlendVividLight(baseCol.rgb, blendCol.rgb); break;
		case 16: result = BlendPinLight(baseCol.rgb, blendCol.rgb); break;
		case 17: result = BlendHardMix(baseCol.rgb, blendCol.rgb); break;
		case 18: result = BlendReflect(baseCol.rgb, blendCol.rgb); break;
		case 19: result = BlendGlow(baseCol.rgb, blendCol.rgb); break;
		case 20: result = BlendPhoenix(baseCol.rgb, blendCol.rgb); break;
		case 21: result = BlendHue(baseCol.rgb, blendCol.rgb); break;
		case 22: result = BlendSaturation(baseCol.rgb, blendCol.rgb); break;
		case 23: result = BlendColor(baseCol.rgb, blendCol.rgb); break;
		case 24: result = BlendLuminosity(baseCol.rgb, blendCol.rgb); break;
		case 25: result = max(baseCol.rgb, blendCol.rgb); break; // Maximum
		case 26: result = min(baseCol.rgb, blendCol.rgb); break; // Minimum
		default: result = blendCol.rgb; break;
	}
	
	// Alpha compositing: blend result with base using blend alpha
	// This creates the Photoshop-like layer effect where alpha affects visibility
	vec3 finalRGB = mix(baseCol.rgb, result, blendAlpha);
	
	// Output alpha is the combination of base and blend alphas
	float finalAlpha = baseCol.a + blendAlpha * (1.0 - baseCol.a);
	
	fragColor = vec4(finalRGB, finalAlpha);
}
)"