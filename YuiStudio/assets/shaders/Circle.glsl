#type vertex
#version 330 core

layout(location = 0) in vec3 a_WorldPosition;
layout(location = 1) in vec2 a_LocalPosition;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_Thickness;
layout(location = 4) in float a_Fade;
layout(location = 5) in int a_EntityID;

uniform mat4 u_ViewProjection;

out vec2 v_LocalPosition;
out vec4 v_Color;
out float v_Thickness;
out float v_Fade;
flat out int v_EntityID;

void main()
{
	v_LocalPosition = a_LocalPosition;
	v_Color = a_Color;
	v_Thickness = a_Thickness;
	v_Fade = a_Fade;
	v_EntityID = a_EntityID;
	gl_Position = u_ViewProjection * vec4(a_WorldPosition, 1.0);
}

#type fragment
#version 330 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec2 v_LocalPosition;
in vec4 v_Color;
in float v_Thickness;
in float v_Fade;
flat in int v_EntityID;

void main()
{
	float dist = length(v_LocalPosition);
	if (dist > 1.0 || dist < 1.0 - v_Thickness - v_Fade)
		discard;

	float alpha = 1.0 - smoothstep(1.0 - v_Fade, 1.0, dist);
	alpha *= smoothstep(1.0 - v_Thickness - v_Fade, 1.0 - v_Thickness, dist);

	if (alpha <= 0.0)
		discard;

	o_Color = v_Color;
	o_Color.a *= alpha;
	o_EntityID = v_EntityID;
}
