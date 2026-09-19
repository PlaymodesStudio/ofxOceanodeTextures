//LayerB:texture, Progress:0:0:1, StartAngle:0:-180:180, Clockwise:1:0:1, Softness:0.02:0:0.5, CenterX:0.5:0:1, CenterY:0.5:0:1
// @description Reveals a second texture around a configurable center like a clock wipe.
// @param LayerB: Image revealed as the wipe advances.
// @param Progress: Transition completion from source zero to LayerB one.
// @param StartAngle: Angle where the radial reveal begins.
// @param Clockwise: Crossfades between counterclockwise and clockwise reveal direction.
// @param Softness: Feather width around the moving radial boundary.
// @param CenterX: Horizontal center of the radial wipe.
// @param CenterY: Vertical center of the radial wipe.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected; uniform float StartAngle; uniform sampler2D StartAngleTex; uniform int StartAngleTexConnected;
uniform float Clockwise; uniform sampler2D ClockwiseTex; uniform int ClockwiseTexConnected; uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected; uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv); float p=seUnit(ProgressTex,ProgressTexConnected,Progress,uv); if(p<=0.0){out_color=a;return;} if(p>=1.0){out_color=b;return;} vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); float angle=fract((atan(uv.y-center.y,uv.x-center.x)-radians(seRange(StartAngleTex,StartAngleTexConnected,StartAngle,-180.0,180.0,uv)))/(2.0*SE_PI)); angle=mix(fract(1.0-angle),angle,seUnit(ClockwiseTex,ClockwiseTexConnected,Clockwise,uv)); float soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv); float matte=soft<=SE_EPSILON?step(angle,p):1.0-smoothstep(p-soft,p+soft,angle); out_color=mix(a,b,matte); }
