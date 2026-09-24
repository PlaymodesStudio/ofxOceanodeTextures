//OutputMode:0:0:5, WindowRadius:2:1:4, Regularization:0.0001:0.000001:0.01, ConfidenceThreshold:0.05:0:1, MotionThreshold:0.1:0:8, MaxMotion:16:0.1:64, Gain:1:0:8, DisplayScale:4:0.1:64, GridSpacing:24:8:128
// @description Estimates dense small-motion Lucas-Kanade optical flow between consecutive input updates. Input history is supplied automatically by simpleEffect. Raw output stores signed forward displacement in RG, confidence in B, and one in A. Start with a resized input for interactive performance.
// @param OutputMode: Zero Raw Flow, one Direction Colour, two Magnitude, three Motion Mask, four Confidence, five Vector Overlay. A single selected mode is emitted.
// @param WindowRadius: Integer neighborhood radius; two uses a 5x5 window. Larger windows cost more and mix nearby motions.
// @param Regularization: Scalar stabilizer for the local solve and confidence estimate; larger values suppress weak texture. Assumes input RGB in the zero-to-one range.
// @param ConfidenceThreshold: Rejects flow below this local texture-confidence estimate. Confidence is not a calibrated correctness probability.
// @param MotionThreshold: Rejects displacements below this many pixels before Gain; also defines the motion mask.
// @param MaxMotion: Limits estimated displacement length in pixels before Gain. Raising this does not improve large-motion tracking.
// @param Gain: Multiplies accepted vectors; one preserves pixel displacement per processed input update.
// @param DisplayScale: Motion in pixels after Gain that reaches full brightness in direction and magnitude previews.
// @param GridSpacing: Scalar spacing of vector-overlay arrows in pixels. Preview arrows are capped to fit their cells.
#version 410
#pragma include "../SimpleEffectCommon.inc"

uniform sampler2D tSource;
uniform sampler2D tPreviousSource;
uniform int tPreviousSourceConnected;
uniform vec2 uResolution;

uniform float OutputMode;
uniform float WindowRadius;
uniform float Regularization;
uniform float GridSpacing;

uniform float ConfidenceThreshold;
uniform sampler2D ConfidenceThresholdTex;
uniform int ConfidenceThresholdTexConnected;
uniform float MotionThreshold;
uniform sampler2D MotionThresholdTex;
uniform int MotionThresholdTexConnected;
uniform float MaxMotion;
uniform sampler2D MaxMotionTex;
uniform int MaxMotionTexConnected;
uniform float Gain;
uniform sampler2D GainTex;
uniform int GainTexConnected;
uniform float DisplayScale;
uniform sampler2D DisplayScaleTex;
uniform int DisplayScaleTexConnected;

out vec4 out_color;

float luminanceAt(sampler2D image, ivec2 pixel)
{
    return seLuma(clamp(texelFetch(image, pixel, 0).rgb, vec3(0.0), vec3(1.0)));
}

// Returns forward displacement (previous -> current) and local confidence.
// Positive X/Y follow increasing source texture coordinates, before display flips.
vec3 flowAt(ivec2 center)
{
    if(tPreviousSourceConnected == 0){
        return vec3(0.0);
    }

    int radius = clamp(int(round(WindowRadius)), 1, 4);
    ivec2 size = ivec2(uResolution);
    vec3 tensor = vec3(0.0); // xx, xy, yy
    vec2 temporal = vec2(0.0);
    float count = 0.0;
    for(int y = -radius; y <= radius; ++y){
        for(int x = -radius; x <= radius; ++x){
            ivec2 p = center + ivec2(x, y);
            // Exclude samples without a full derivative stencil at image borders.
            if(any(lessThan(p, ivec2(1))) || any(greaterThanEqual(p, size - ivec2(1)))){
                continue;
            }
            ivec2 dx = ivec2(1, 0);
            ivec2 dy = ivec2(0, 1);
            vec2 gradient = 0.25 * vec2(
                luminanceAt(tSource, p + dx) - luminanceAt(tSource, p - dx) +
                luminanceAt(tPreviousSource, p + dx) - luminanceAt(tPreviousSource, p - dx),
                luminanceAt(tSource, p + dy) - luminanceAt(tSource, p - dy) +
                luminanceAt(tPreviousSource, p + dy) - luminanceAt(tPreviousSource, p - dy));
            float difference = luminanceAt(tSource, p) - luminanceAt(tPreviousSource, p);
            tensor += vec3(gradient.x * gradient.x, gradient.x * gradient.y, gradient.y * gradient.y);
            temporal += gradient * difference;
            count += 1.0;
        }
    }
    tensor /= max(count, 1.0);
    temporal /= max(count, 1.0);

    float stabilizer = clamp(Regularization, 0.000001, 0.01);
    float discriminant = length(vec2(tensor.x - tensor.z, 2.0 * tensor.y));
    float minEigenvalue = max(0.5 * (tensor.x + tensor.z - discriminant), 0.0);
    float confidence = minEigenvalue / (minEigenvalue + stabilizer);

    float a = tensor.x + stabilizer;
    float b = tensor.y;
    float c = tensor.z + stabilizer;
    float determinant = max(a * c - b * b, 1.0e-12);
    vec2 flow = vec2(b * temporal.y - c * temporal.x,
                     b * temporal.x - a * temporal.y) / determinant;

    vec2 uv = (vec2(center) + 0.5) / uResolution;
    float confidenceThreshold = seUnit(ConfidenceThresholdTex, ConfidenceThresholdTexConnected, ConfidenceThreshold, uv);
    float motionThreshold = seRange(MotionThresholdTex, MotionThresholdTexConnected, MotionThreshold, 0.0, 8.0, uv);
    float maxMotion = seRange(MaxMotionTex, MaxMotionTexConnected, MaxMotion, 0.1, 64.0, uv);
    float gain = seRange(GainTex, GainTexConnected, Gain, 0.0, 8.0, uv);
    float speed = length(flow);
    if(confidence <= confidenceThreshold || speed <= motionThreshold){
        flow = vec2(0.0);
    }else{
        flow *= min(1.0, maxMotion / max(speed, SE_EPSILON)) * gain;
    }
    return vec3(flow, confidence);
}

float segmentDistance(vec2 p, vec2 a, vec2 b)
{
    vec2 direction = b - a;
    float position = clamp(dot(p - a, direction) / max(dot(direction, direction), SE_EPSILON), 0.0, 1.0);
    return length(p - a - position * direction);
}

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 uv = gl_FragCoord.xy / uResolution;
    int mode = clamp(int(round(OutputMode)), 0, 5);
    float spacing = clamp(round(GridSpacing), 8.0, 128.0);
    vec2 cellCenter = (floor(gl_FragCoord.xy / spacing) + 0.5) * spacing;
    // The overlay solves at each cell center so every arrow has one direction.
    ivec2 solvePixel = mode == 5 ? clamp(ivec2(cellCenter), ivec2(0), ivec2(uResolution) - 1) : pixel;
    vec3 motion = flowAt(solvePixel);
    float speed = length(motion.xy);
    float scale = max(seRange(DisplayScaleTex, DisplayScaleTexConnected, DisplayScale, 0.1, 64.0, uv), 0.1);
    float magnitude = clamp(speed / scale, 0.0, 1.0);
    float angle = speed > SE_EPSILON ? atan(motion.y, motion.x) : 0.0;
    vec3 directionColor = seHsvToRgb(vec3(fract(angle / (2.0 * SE_PI) + 1.0), 1.0, magnitude));

    if(mode == 0){
        out_color = vec4(motion, 1.0);
    }else if(mode == 1){
        out_color = vec4(directionColor, 1.0);
    }else if(mode == 2){
        out_color = vec4(vec3(magnitude), 1.0);
    }else if(mode == 3){
        out_color = vec4(vec3(speed > SE_EPSILON ? 1.0 : 0.0), 1.0);
    }else if(mode == 4){
        out_color = vec4(vec3(motion.z), 1.0);
    }else{
        vec4 source = texelFetch(tSource, pixel, 0);
        float arrow = 0.0;
        if(speed > SE_EPSILON){
            vec2 direction = motion.xy / speed;
            float arrowLength = min(speed, spacing * 0.7);
            vec2 start = cellCenter - direction * arrowLength * 0.5;
            vec2 tip = cellCenter + direction * arrowLength * 0.5;
            vec2 normal = vec2(-direction.y, direction.x);
            float head = min(arrowLength * 0.4, 5.0);
            float distanceToArrow = min(segmentDistance(gl_FragCoord.xy, start, tip),
                min(segmentDistance(gl_FragCoord.xy, tip, tip - direction * head + normal * head * 0.5),
                    segmentDistance(gl_FragCoord.xy, tip, tip - direction * head - normal * head * 0.5)));
            arrow = 1.0 - smoothstep(0.5, 1.5, distanceToArrow);
        }
        vec3 arrowColor = seHsvToRgb(vec3(fract(angle / (2.0 * SE_PI) + 1.0), 0.8, 1.0));
        out_color = vec4(mix(source.rgb, arrowColor, arrow), source.a);
    }
}
