R"(
#version 410

uniform int type;
uniform vec2 size;
uniform float f;
uniform vec2 pos;
uniform vec2 scale;
uniform vec2 offset;
uniform float warping;
uniform float modulator;

uniform sampler2D scaleXTex;
uniform sampler2D scaleYTex;
uniform sampler2D offsetXTex;
uniform sampler2D offsetYTex;
uniform sampler2D fTex;

out vec4 out_color;

float customPow(float value, float pow){
    float k1 = 2*pow*0.99999;
    float k2 = (k1/((-pow*0.999999)+1));
    float k3 = k2 * abs(value) + 1;
    return (value * (k2+1) / k3);
}

//
// GLSL textureless classic 3D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-10-11
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/stegu/webgl-noise
//

vec3 mod289(vec3 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise(vec3 P)
{
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
  return 2.2 * n_xyz;
}

// Classic Perlin noise, periodic variant
float pnoise(vec3 P, vec3 rep)
{
  vec3 Pi0 = mod(floor(P), rep); // Integer part, modulo period
  vec3 Pi1 = mod(Pi0 + vec3(1.0), rep); // Integer part + 1, mod period
  Pi0 = mod289(Pi0);
  Pi1 = mod289(Pi1);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 * (1.0 / 7.0);
  vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 * (1.0 / 7.0);
  vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
  return 2.2 * n_xyz;
}


///////https://github.com/MaxBittker/glsl-voronoi-noise/blob/master/3d.glsl
const mat2 myt = mat2(.12121212, .13131313, -.13131313, .12121212);
const vec2 mys = vec2(1e4, 1e6);

vec2 rhash(vec2 uv) {
  uv *= myt;
  uv *= mys;
  return fract(fract(uv / mys) * uv);
}

vec3 hash(vec3 p) {
  return fract(
      sin(vec3(dot(p, vec3(1.0, 57.0, 113.0)), dot(p, vec3(57.0, 113.0, 1.0)),
               dot(p, vec3(113.0, 1.0, 57.0)))) *
      43758.5453);
}

vec3 voronoi3d(const in vec3 x) {
  vec3 p = floor(x);
  vec3 f = fract(x);

  float id = 0.0;
  vec2 res = vec2(100.0);
  for (int k = -1; k <= 1; k++) {
    for (int j = -1; j <= 1; j++) {
      for (int i = -1; i <= 1; i++) {
        vec3 b = vec3(float(i), float(j), float(k));
        vec3 r = vec3(b) - f + hash(p + b);
        float d = dot(r, r);

        float cond = max(sign(res.x - d), 0.0);
        float nCond = 1.0 - cond;

        float cond2 = nCond * max(sign(res.y - d), 0.0);
        float nCond2 = 1.0 - cond2;

        id = (dot(p + b, vec3(1.0, 57.0, 113.0)) * cond) + (id * nCond);
        res = vec2(d, res.x) * cond + res * nCond;

        res.y = cond2 * d + nCond2 * res.y;
      }
    }
  }

  return vec3(sqrt(res), abs(id));
}

//Gradient noise https://www.iquilezles.org/www/articles/gradientnoise/gradientnoise.htm
// returns 3D value noise
float gradientNoise( in vec3 x )
{
    // grid
    vec3 p = floor(x);
    vec3 w = fract(x);
    
    // quintic interpolant
    vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);

    
    // gradients
    vec3 ga = hash( p+vec3(0.0,0.0,0.0) );
    vec3 gb = hash( p+vec3(1.0,0.0,0.0) );
    vec3 gc = hash( p+vec3(0.0,1.0,0.0) );
    vec3 gd = hash( p+vec3(1.0,1.0,0.0) );
    vec3 ge = hash( p+vec3(0.0,0.0,1.0) );
    vec3 gf = hash( p+vec3(1.0,0.0,1.0) );
    vec3 gg = hash( p+vec3(0.0,1.0,1.0) );
    vec3 gh = hash( p+vec3(1.0,1.0,1.0) );
    
    // projections
    float va = dot( ga, w-vec3(0.0,0.0,0.0) );
    float vb = dot( gb, w-vec3(1.0,0.0,0.0) );
    float vc = dot( gc, w-vec3(0.0,1.0,0.0) );
    float vd = dot( gd, w-vec3(1.0,1.0,0.0) );
    float ve = dot( ge, w-vec3(0.0,0.0,1.0) );
    float vf = dot( gf, w-vec3(1.0,0.0,1.0) );
    float vg = dot( gg, w-vec3(0.0,1.0,1.0) );
    float vh = dot( gh, w-vec3(1.0,1.0,1.0) );
    
    // interpolation
    return va +
           u.x*(vb-va) +
           u.y*(vc-va) +
           u.z*(ve-va) +
           u.x*u.y*(va-vb-vc+vd) +
           u.y*u.z*(va-vc-ve+vg) +
           u.z*u.x*(va-vb-ve+vf) +
           u.x*u.y*u.z*(-va+vb+vc-vd+ve-vf-vg+vh);
}

//Value noise https://www.iquilezles.org/www/articles/morenoise/morenoise.htm
 vec4 valueNoise( in vec3 x )
 {
    vec3 p = floor(x);
    vec3 w = fract(x);

    // quintic interpolation
    vec3 u = w*vec3(w.xy, 1)*vec3(w.xy, 1)*(vec3(w.xy, 1)*(vec3(w.xy, 1)*6.0-15.0)+10.0);
    vec3 du = 30.0*vec3(w.xy, 1)*vec3(w.xy, 1)*(w*(w-2.0)+1.0);


    float a = length(hash( p+vec3(0,0,0) ));
    float b = length(hash( p+vec3(1,0,0) ));
    float c = length(hash( p+vec3(0,1,0) ));
    float d = length(hash( p+vec3(1,1,0) ));
    float e = length(hash( p+vec3(0,0,1) ));
    float f = length(hash( p+vec3(1,0,1) ));
    float g = length(hash( p+vec3(0,1,1) ));
    float h = length(hash( p+vec3(1,1,1) ));

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return vec4( -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z),
                 2.0* du * vec3( k1 + k4*u.y + k6*u.z + k7*u.y*u.z,
                                 k2 + k5*u.z + k4*u.x + k7*u.z*u.x,
                                 k3 + k6*u.x + k5*u.y + k7*u.x*u.y ) );
}

//Cellular https://www.iquilezles.org/www/articles/smoothvoronoi/smoothvoronoi.htm
float smoothVoronoi( in vec3 x )
{
    vec3 p = (floor( x ));
    vec3  f = fract( x );
    
    float res = 0.0;
    for (int k=-1; k<=1; k++ )
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        vec3 b = vec3( float(i), float(j), float(k) );
        vec3 r = vec3(b) - f + hash(p + b);
        float d = dot( r, r );
        
        res += exp( -4.0*d );
    }
    return -(1.0/4.0)*log( res );
}


//Metaballs: https://thebookofshaders.com/edit.php#12/metaballs.frag
vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

float metaballs(vec3 st) {
    // Tile the space
    vec2 i_st = floor(st.xy);
    vec2 f_st = fract(st.xy);

    float m_dist = 1.;  // minimum distance
    for (int j= -1; j <= 1; j++ ) {
        for (int i= -1; i <= 1; i++ ) {
            // Neighbor place in the grid
            vec2 neighbor = vec2(float(i),float(j));

            // Random position from current + neighbor place in the grid
            vec2 offset = random2(i_st + neighbor);

            // Animate the offset
            offset = 0.5 + 0.5*sin(st.z + 6.2831*offset);

            // Position of the cell
            vec2 pos = neighbor + offset - f_st;

            // Cell distance
            float dist = length(pos);

            // Metaball it!
            m_dist = min(m_dist, m_dist*dist);
        }
    }
    return m_dist;
    // Draw cells
   return step(0.060, m_dist);
}



////FBM from: https://github.com/yiwenl/glsl-fbm/blob/master/3d.glsl
float mod289_fbm(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289_fbm(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289_fbm(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}


float fbm(vec3 x, float octaves, float _modulator) {
    float v = 0.0;
    float a = 0.5;
    vec3 shift = vec3(100);
    for (int i = 0; i < ceil(octaves); ++i) {
        if(i == floor(octaves)){
            v += fract(octaves) * a * ((cnoise(x)+1)/2);
        }else{
            v += a * ((cnoise(x)+1)/2);
        }
        x = x * ((_modulator*10.0) +1.0) + shift;
        a *= 0.5;
    }
    return v;
}

void main()
{
    float result = 0;
    
    float warpingMod = warping;
    if(warpingMod == floor(warping)){
        warpingMod = 1.0f;
    }else{
        warpingMod = fract(warping);
    }


	vec2 _scale = scale;
	if(textureSize(scaleXTex, 0).xy != vec2(1.0f, 1.0f)){
		_scale.x = texture(scaleXTex, gl_FragCoord.xy/size, 0).r;
	}
	if(textureSize(scaleYTex, 0).xy != vec2(1.0f, 1.0f)){
		_scale.y = texture(scaleYTex, gl_FragCoord.xy/size, 0).r;
	}

	vec2 _offset = offset;
	if(textureSize(offsetXTex, 0).xy != vec2(1.0f, 1.0f)){
		_offset.x = texture(offsetXTex, gl_FragCoord.xy/size, 0).r;
	}
	if(textureSize(offsetYTex, 0).xy != vec2(1.0f, 1.0f)){
		_offset.y = texture(offsetYTex, gl_FragCoord.xy/size, 0).r;
	}

	float _f = f;
	if(textureSize(fTex, 0).xy != vec2(1.0f, 1.0f)){
		_f = f * texture(fTex, gl_FragCoord.xy/size, 0).r;
	}

    vec2 _pos = gl_FragCoord.xy - pos;

    switch (type) {
        case 0://Perlin
            //Domain Warping (modified) from: https://www.iquilezles.org/www/articles/warp/warp.htm
            for(int i = 0; i < ceil(warping) ; i++){
               if(i == floor(warping)){
                    result = ((1-fract(warping))*result) + (fract(warping)*cnoise(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y))));
                }else{
                    //result = cnoise(vec3(result) + vec3(((gl_FragCoord.xy)-(size/2)) * _scale, _f + (gl_FragCoord.x * _offset.x) + (gl_FragCoord.y * _offset.y)));
					result = cnoise(vec3(result) + vec3((((_pos)-(size/2)) * _scale), _f + (size.x * _offset.x) + (size.y * _offset.y)));

                }
            }
            result = (result + 1)/2;
            break;
        case 1://Voronoi
            for(int i = 0; i < ceil(warping) ; i++){
               if(i == floor(warping)){
                    result = ((1-fract(warping))*result) + (fract(warping)*voronoi3d(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y))).x);
                }else{
                    result = voronoi3d(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y))).x;
                }
            }
            break;
        case 2://Gradient
            for(int i = 0; i < ceil(warping) ; i++){
               if(i == floor(warping)){
                    result = ((1-fract(warping))*result) + (fract(warping)*gradientNoise(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y))));
                }else{
                    result = gradientNoise(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y)));
                }
                result *= 2;
            }
            break;
        case 3://Value
            for(int i = 0; i < ceil(warping) ; i++){
               if(i == floor(warping)){
                    result = ((1-fract(warping))*result) + (fract(warping)*valueNoise(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y))).x);
                }else{
                    result = valueNoise(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y))).x;
                }
            }
            break;
        case 4://Callular
            for(int i = 0; i < ceil(warping) ; i++){
               if(i == floor(warping)){
                    result = ((1-fract(warping))*result) + (fract(warping)*smoothVoronoi(vec3(result) + vec3(((_pos)-(size/2)) * scale, f)));
                }else{
                    result = smoothVoronoi(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y)));
                }
            }
            break;
        case 5://MetaBalls
            for(int i = 0; i < ceil(warping) ; i++){
               if(i == floor(warping)){
                    result = ((1-fract(warping))*result) + (fract(warping)*metaballs(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y))));
                }else{
                    result = metaballs(vec3(result) + vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y)));
                }
            }
            break;
        case 6://Fbm
            result = fbm(vec3(((_pos)-(size/2)) * _scale, _f + (size.x * _offset.x) + (size.y * _offset.y)), warping, modulator);
            break;
        default:
            break;
    }
    
    out_color = vec4(vec3(result), 1.0);
}
)"
