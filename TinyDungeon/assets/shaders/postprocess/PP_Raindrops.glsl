#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}

#type fragment
#version 450 core

in vec2 v_TexCoord;
out vec4 o_Color;

uniform sampler2D u_InputTexture;
uniform float u_RaindropsIntensity;
uniform float u_RaindropsTime;

#define S(a, b, t) smoothstep(a, b, t)

float N21(vec2 p)
{
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

float saw(float x)
{
    return (cos(x) + cos(x * 3.0) * 0.5 + sin(x * 5.0) * 0.1) * 0.4 + 0.5;
}

vec2 GetDrops(vec2 uv, float seed, float m)
{
    float t = u_RaindropsTime + m * 30.0;
    vec2 o = vec2(0.0);
    
    uv.y += t * 0.05;
    
    uv *= vec2(10.0, 2.5) * seed;
    vec2 id = floor(uv);
    vec2 st = fract(uv) - 0.5;
    
    float n = N21(id + seed);
    float x = n - 0.5;
    float ti = fract(t + n * 6.28);
    float y = (saw(ti) - 0.5) * 0.9;
    
    vec2 p = vec2(x, y);
    float d = length(st - p);
    
    float mainDrop = S(0.2, 0.0, d);
    
    float trailMask = S(0.0, 0.2, st.y - p.y);
    trailMask *= S(0.5, 0.0, st.y - p.y);
    trailMask *= S(0.05, 0.03, abs(st.x - p.x));
    
    float td = length(st - vec2(p.x, st.y));
    float dropTrail = S(0.1, 0.02, td);
    dropTrail *= trailMask;
    
    o = (mainDrop + dropTrail * 0.5) * (st - p);
    
    return o;
}

vec2 CalculateRaindropsOffset(vec2 uv, float intensity)
{
    vec2 offs = vec2(0.0);
    
    offs += GetDrops(uv, 1.0, intensity);
    offs += GetDrops(uv * 1.4 + 7.23, 1.25, intensity);
    offs += GetDrops(uv * 2.1 + 1.17, 1.5, intensity) * 0.5;
    
    return offs * 0.03 * intensity;
}

void main()
{
    vec2 uv = v_TexCoord;
    
    vec2 offs = CalculateRaindropsOffset(uv, u_RaindropsIntensity);
    uv = clamp(uv + offs, vec2(0.0), vec2(1.0));
    
    o_Color = texture(u_InputTexture, uv);
}
