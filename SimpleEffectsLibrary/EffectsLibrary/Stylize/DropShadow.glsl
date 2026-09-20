//ShadowColor:color:0:0:0:1, OffsetX:12:-256:256, OffsetY:-12:-256:256, Blur:8:0:64, Spread:0:0:1, Opacity:0.75:0:1, SourceOpacity:1:0:1
// @description Composites a colored, offset, softened copy of source alpha behind the image.
// @param ShadowColor: Color of the generated shadow.
// @param OffsetX: Horizontal shadow offset in pixels.
// @param OffsetY: Vertical shadow offset in pixels.
// @param Blur: Radial shadow softness in pixels.
// @param Spread: Expands weak alpha values before opacity.
// @param Opacity: Overall shadow density.
// @param SourceOpacity: Opacity of the original image above the shadow.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution; uniform vec4 ShadowColor;
uniform float OffsetX; uniform sampler2D OffsetXTex; uniform int OffsetXTexConnected; uniform float OffsetY; uniform sampler2D OffsetYTex; uniform int OffsetYTexConnected;
uniform float Blur; uniform sampler2D BlurTex; uniform int BlurTexConnected; uniform float Spread; uniform sampler2D SpreadTex; uniform int SpreadTexConnected;
uniform float Opacity; uniform sampler2D OpacityTex; uniform int OpacityTexConnected; uniform float SourceOpacity; uniform sampler2D SourceOpacityTex; uniform int SourceOpacityTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); vec2 offset=vec2(seRange(OffsetXTex,OffsetXTexConnected,OffsetX,-256.0,256.0,uv),seRange(OffsetYTex,OffsetYTexConnected,OffsetY,-256.0,256.0,uv))/uResolution; float blur=seRange(BlurTex,BlurTexConnected,Blur,0.0,64.0,uv), matte=0.0; for(int i=0;i<12;i++){float a=2.0*SE_PI*float(i)/12.0;matte+=texture(tSource,uv-offset+vec2(cos(a),sin(a))*blur/uResolution).a;} matte/=12.0; matte=clamp(matte+seUnit(SpreadTex,SpreadTexConnected,Spread,uv)*(1.0-matte),0.0,1.0)*seUnit(OpacityTex,OpacityTexConnected,Opacity,uv)*ShadowColor.a; float sourceAlpha=s.a*seUnit(SourceOpacityTex,SourceOpacityTexConnected,SourceOpacity,uv); vec3 rgb=ShadowColor.rgb*matte*(1.0-sourceAlpha)+s.rgb*sourceAlpha; float alpha=matte+sourceAlpha*(1.0-matte); out_color=vec4(rgb,alpha); }
