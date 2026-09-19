//LayerB:texture, Progress:0:0:1, Softness:0.05:0:0.5, Roundness:1:0:1, Rotation:0:-180:180, CenterX:0.5:0:1, CenterY:0.5:0:1
// @description Reveals a second texture through an expanding circular or rectangular iris.
// @param LayerB: Image revealed inside the iris.
// @param Progress: Transition completion from source zero to LayerB one.
// @param Softness: Feather width around the iris boundary.
// @param Roundness: Crossfades a square iris into a circular iris.
// @param Rotation: Rotation of the square component in degrees.
// @param CenterX: Horizontal iris center.
// @param CenterY: Vertical iris center.
#version 410
#pragma include "../SimpleEffectCommon.inc"
uniform sampler2D tSource; uniform sampler2D LayerB; uniform int LayerBConnected; uniform vec2 uResolution;
uniform float Progress; uniform sampler2D ProgressTex; uniform int ProgressTexConnected; uniform float Softness; uniform sampler2D SoftnessTex; uniform int SoftnessTexConnected;
uniform float Roundness; uniform sampler2D RoundnessTex; uniform int RoundnessTexConnected; uniform float Rotation; uniform sampler2D RotationTex; uniform int RotationTexConnected;
uniform float CenterX; uniform sampler2D CenterXTex; uniform int CenterXTexConnected; uniform float CenterY; uniform sampler2D CenterYTex; uniform int CenterYTexConnected; out vec4 out_color;
void main(){ vec2 uv=gl_FragCoord.xy/uResolution; vec4 a=texture(tSource,uv); if(LayerBConnected==0){out_color=a;return;} vec4 b=texture(LayerB,uv); float progress=seUnit(ProgressTex,ProgressTexConnected,Progress,uv); if(progress<=0.0){out_color=a;return;} if(progress>=1.0){out_color=b;return;} vec2 center=vec2(seUnit(CenterXTex,CenterXTexConnected,CenterX,uv),seUnit(CenterYTex,CenterYTexConnected,CenterY,uv)); float aspect=uResolution.x/uResolution.y; vec2 p=(uv-center)*2.0; p.x*=aspect; float angle=radians(seRange(RotationTex,RotationTexConnected,Rotation,-180.0,180.0,uv)); p=mat2(cos(angle),-sin(angle),sin(angle),cos(angle))*p; float distance=mix(max(abs(p.x),abs(p.y)),length(p),seUnit(RoundnessTex,RoundnessTexConnected,Roundness,uv)); float maxRadius=length(vec2(max(center.x,1.0-center.x)*2.0*aspect,max(center.y,1.0-center.y)*2.0)); float radius=progress*maxRadius, soft=seRange(SoftnessTex,SoftnessTexConnected,Softness,0.0,0.5,uv); float matte=soft<=SE_EPSILON?step(distance,radius):1.0-smoothstep(radius-soft,radius+soft,distance); out_color=mix(a,b,matte); }
