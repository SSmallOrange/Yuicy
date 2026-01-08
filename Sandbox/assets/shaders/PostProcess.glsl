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

void main()
{
    vec4 sceneColor = texture(u_ScreenTexture, v_TexCoord);
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