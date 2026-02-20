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
uniform float u_VignetteIntensity;
uniform float u_VignetteRadius;

void main()
{
    vec4 sceneColor = texture(u_InputTexture, v_TexCoord);
    
    vec2 center = v_TexCoord - 0.5;
    float dist = length(center);
    float vignette = smoothstep(u_VignetteRadius, u_VignetteRadius - 0.5, dist);
    vignette = mix(1.0, vignette, u_VignetteIntensity);
    
    o_Color = vec4(sceneColor.rgb * vignette, sceneColor.a);
}
