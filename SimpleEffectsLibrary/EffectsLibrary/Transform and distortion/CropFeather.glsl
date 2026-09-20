//Left:0:0:1, Right:0:0:1, Top:0:0:1, Bottom:0:0:1, Feather:0:0:256, Premultiply:1:0:1
// @description Crops each image edge independently and feathers the resulting alpha boundary.
// @param Left: Fraction cropped from the left edge.
// @param Right: Fraction cropped from the right edge.
// @param Top: Fraction cropped from the top edge.
// @param Bottom: Fraction cropped from the bottom edge.
// @param Feather: Edge softness measured in source pixels.
// @param Premultiply: Multiplies RGB by the crop matte.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform vec2 uResolution;
uniform float Left; uniform sampler2D LeftTex; uniform int LeftTexConnected;
uniform float Right; uniform sampler2D RightTex; uniform int RightTexConnected;
uniform float Top; uniform sampler2D TopTex; uniform int TopTexConnected;
uniform float Bottom; uniform sampler2D BottomTex; uniform int BottomTexConnected;
uniform float Feather; uniform sampler2D FeatherTex; uniform int FeatherTexConnected;
uniform float Premultiply; uniform sampler2D PremultiplyTex; uniform int PremultiplyTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 s=texture(tSource,uv); float l=seUnit(LeftTex,LeftTexConnected,Left,uv), r=1.0-seUnit(RightTex,RightTexConnected,Right,uv); float b=seUnit(BottomTex,BottomTexConnected,Bottom,uv), t=1.0-seUnit(TopTex,TopTexConnected,Top,uv); float feather=seRange(FeatherTex,FeatherTexConnected,Feather,0.0,256.0,uv); vec2 f=vec2(feather)/uResolution; float matte=f.x<=SE_EPSILON?step(l,uv.x)*step(uv.x,r):smoothstep(l,l+f.x,uv.x)*(1.0-smoothstep(r-f.x,r,uv.x)); matte*=f.y<=SE_EPSILON?step(b,uv.y)*step(uv.y,t):smoothstep(b,b+f.y,uv.y)*(1.0-smoothstep(t-f.y,t,uv.y)); float pre=seUnit(PremultiplyTex,PremultiplyTexConnected,Premultiply,uv); out_color=vec4(s.rgb*mix(1.0,matte,pre),s.a*matte); }
