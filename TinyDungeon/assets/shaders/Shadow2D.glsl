#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;

uniform vec2 u_CameraPos;
uniform vec2 u_ViewportSize;

void main()
{
    // Convert world position to NDC
    vec2 screenPos = (a_Position - u_CameraPos) / (u_ViewportSize * 0.5);
    gl_Position = vec4(screenPos, 0.0, 1.0);
}

#type fragment
#version 450 core

out vec4 o_Color;

void main()
{
    // White = visible, will be used as shadow mask
    o_Color = vec4(1.0, 1.0, 1.0, 1.0);
}
