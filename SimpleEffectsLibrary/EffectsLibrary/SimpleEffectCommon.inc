const float SE_EPSILON = 0.00001;
const float SE_PI = 3.14159265358979323846;
const vec3 SE_LUMA_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);

float seLuma(vec3 color)
{
    return dot(color, SE_LUMA_WEIGHTS);
}

float seUnit(sampler2D controlTexture, int connected, float fallback, vec2 uv)
{
    return connected != 0
         ? clamp(texture(controlTexture, uv).r, 0.0, 1.0)
         : fallback;
}

float seRange(sampler2D controlTexture,
              int connected,
              float fallback,
              float minimum,
              float maximum,
              vec2 uv)
{
    return connected != 0
         ? mix(minimum, maximum, clamp(texture(controlTexture, uv).r, 0.0, 1.0))
         : fallback;
}

bool seInside(vec2 uv)
{
    return all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)));
}

vec3 seRgbToHsv(vec3 color)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(color.bg, K.wz), vec4(color.gb, K.xy), step(color.b, color.g));
    vec4 q = mix(vec4(p.xyw, color.r), vec4(color.r, p.yzx), step(p.x, color.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 seHsvToRgb(vec3 color)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(color.xxx + K.xyz) * 6.0 - K.www);
    return color.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), color.y);
}
