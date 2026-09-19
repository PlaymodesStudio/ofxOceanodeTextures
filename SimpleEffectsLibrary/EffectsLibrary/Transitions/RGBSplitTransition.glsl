//LayerB:texture, Progress:0:0:1, Split:30:0:300, Angle:0:-180:180, Softness:0.1:0:0.5
// @description Crossfades two textures while their red and blue channels separate around the midpoint.
// @param LayerB: Image replacing the source.
// @param Progress: Transition completion from source zero to LayerB one.
// @param Split: Peak red-blue separation in pixels.
// @param Angle: Direction of channel separation in degrees.
// @param Softness: Shapes the crossfade curve around the midpoint.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected; uniform float Split; uniform sampler2D SplitTex; uniform int SplitTexConnected;
uniform float Angle; uniform sampler2D AngleTex; uniform int AngleTexConnected; uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected; out vec4 out_color;
vec4 splitSample(sampler2D tex,vec2 uv,vec2 offset){vec4 c=texture(tex,uv);return vec4(texture(tex,uv+offset).r,c.g,texture(tex,uv-offset).b,c.a);}
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 original=texture(tSource,uv); if(LayerBConnected==0){out_color=original;return;} vec4 target=texture(LayerB,uv); float p=seUnit(ProgressTex,ProgressTexConnected,Progress,uv); if(p<=0.0){out_color=original;return;} if(p>=1.0){out_color=target;return;} float angle=radians(seRange(AngleTex,AngleTexConnected,Angle,-180.0,180.0,uv)); vec2 offset=vec2(cos(angle),sin(angle))*seRange(SplitTex,SplitTexConnected,Split,0.0,300.0,uv)*sin(p*SE_PI)/uResolution; float soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv); float blend=soft<=SE_EPSILON?step(0.5,p):smoothstep(0.5-soft,0.5+soft,p); out_color=mix(splitSample(tSource,uv,offset),splitSample(LayerB,uv,-offset),blend); }
