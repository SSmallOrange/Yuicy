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

uniform vec2 u_LightPosition;   // Screen space [0, 1]
uniform vec3 u_LightColor;
uniform float u_LightRadius;    // Screen space radius
uniform float u_LightIntensity;
uniform float u_LightFalloff;
uniform float u_AspectRatio;

// Spot light
uniform int u_IsSpotLight;
uniform float u_LightDirection; // Radians
uniform float u_InnerAngle;
uniform float u_OuterAngle;

// Shadow
uniform int u_UseShadowMap;
uniform sampler2D u_ShadowMap;

const float PI = 3.14159265359;

void main()
{
    vec2 uv = v_TexCoord;
    
    // Correct for aspect ratio
    vec2 correctedUV = uv;
    correctedUV.x *= u_AspectRatio;
    vec2 correctedLightPos = u_LightPosition;
    correctedLightPos.x *= u_AspectRatio;
    
    vec2 toLight = correctedUV - correctedLightPos;
    float dist = length(toLight);
    
    // Distance attenuation
    float normalizedDist = dist / (u_LightRadius * u_AspectRatio);
    float attenuation = 1.0 - pow(clamp(normalizedDist, 0.0, 1.0), u_LightFalloff);
    attenuation = clamp(attenuation, 0.0, 1.0);
    
    // Spot light angle attenuation
    if (u_IsSpotLight != 0)
    {
        float angle = atan(toLight.y, toLight.x);
        float angleDiff = abs(angle - u_LightDirection);
        
        // Handle angle wrap around
        if (angleDiff > PI)
            angleDiff = 2.0 * PI - angleDiff;
        
        float spotFactor = 1.0 - smoothstep(u_InnerAngle, u_OuterAngle, angleDiff);
        attenuation *= spotFactor;
    }
    
    // Shadow sampling
    if (u_UseShadowMap != 0)
    {
        float shadow = texture(u_ShadowMap, uv).r;
        attenuation *= shadow;
    }
    
    vec3 finalColor = u_LightColor * u_LightIntensity * attenuation;
    o_Color = vec4(finalColor, 1.0);
}
