//LayerB:texture, Mix:1:0:1
// @description Multiplies the source with a second texture, producing a darker composite.
// @param LayerB: Secondary texture stretched to the source canvas.
// @param Mix: Effect strength, additionally modulated by LayerB alpha.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform vec2 uResolution;
uniform sampler2D LayerB;
uniform int LayerBConnected;
uniform float Mix; uniform sampler2D MixTex; uniform int MixTexConnected;
out vec4 out_color;

void main()
{
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 source = texture(tSource, uv);
    if(LayerBConnected == 0){
        out_color = source;
        return;
    }
    vec4 layer = texture(LayerB, uv);
    float amount = seUnit(MixTex, MixTexConnected, Mix, uv) * layer.a;
    out_color = vec4(mix(source.rgb, source.rgb * layer.rgb, amount), source.a);
}
