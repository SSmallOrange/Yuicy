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
uniform sampler2D u_LightMap;

void main()
{
    vec4 sceneColor = texture(u_InputTexture, v_TexCoord);
    vec3 lightColor = texture(u_LightMap, v_TexCoord).rgb;
    
    o_Color = vec4(sceneColor.rgb * lightColor, sceneColor.a);
}
