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

uniform sampler2D u_ScreenTexture;

uniform vec4 u_AmbientTint;
uniform float u_Brightness;
uniform float u_Contrast;
uniform float u_Saturation;

uniform int u_FogEnabled;
uniform vec4 u_FogColor;
uniform float u_FogDensity;

uniform int u_VignetteEnabled;
uniform float u_VignetteIntensity;
uniform float u_VignetteRadius;

uniform int u_FlashEnabled;
uniform float u_FlashIntensity;
uniform vec3 u_FlashColor;

// 屏幕雨滴
uniform int u_RaindropsEnabled;
uniform float u_RaindropsIntensity;
uniform float u_RaindropsTime;

vec3 AdjustSaturation(vec3 color, float saturation)
{
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    return mix(vec3(gray), color, saturation);
}

vec3 AdjustContrast(vec3 color, float contrast)
{
    return (color - 0.5) * contrast + 0.5;
}

vec3 AdjustBrightness(vec3 color, float brightness)
{
    return color * brightness;
}

float CalculateVignette(vec2 uv, float intensity, float radius)
{
    vec2 center = uv - 0.5;
    float dist = length(center);
    
    float vignette = smoothstep(radius, radius - 0.5, dist);
    
    return mix(1.0, vignette, intensity);
}

// 屏幕雨滴函数

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
    
    // 尾迹
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
    
    // 应用屏幕雨滴偏移
    if (u_RaindropsEnabled != 0 && u_RaindropsIntensity > 0.0)
    {
        vec2 offs = CalculateRaindropsOffset(uv, u_RaindropsIntensity);
        uv = clamp(uv + offs, vec2(0.0), vec2(1.0));
    }
    
    vec4 sceneColor = texture(u_ScreenTexture, uv);
    vec3 color = sceneColor.rgb;

    color *= u_AmbientTint.rgb;

    color = AdjustBrightness(color, u_Brightness);
    color = AdjustContrast(color, u_Contrast);
    color = AdjustSaturation(color, u_Saturation);

    if (u_FogEnabled != 0 && u_FogDensity > 0.0)
    {
        color = mix(color, u_FogColor.rgb, u_FogDensity);
    }

    if (u_VignetteEnabled != 0 && u_VignetteIntensity > 0.0)
    {
        float vignette = CalculateVignette(v_TexCoord, u_VignetteIntensity, u_VignetteRadius);
        color *= vignette;
    }

    if (u_FlashEnabled != 0 && u_FlashIntensity > 0.0)
    {
        color = mix(color, u_FlashColor, u_FlashIntensity);
    }

    color = clamp(color, 0.0, 1.0);

    o_Color = vec4(color, sceneColor.a);
}