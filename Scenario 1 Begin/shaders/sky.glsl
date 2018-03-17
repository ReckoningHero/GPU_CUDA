<<<VSTEXT>>>
#version 420 core

in vec4 a_Position;
in vec2 a_Texcoord;

uniform mat4x4 u_ModelViewProjectionMatrix;

out vec3 positionWS;
out vec2 texcoord;

void main()
{
    gl_Position = u_ModelViewProjectionMatrix * vec4(a_Position.xyz,1.0);
    positionWS = a_Position.xyz;
    texcoord = a_Texcoord.xy;
}

<<<FSTEXT>>>
#version 420 core

uniform vec3 u_LightPosition;
uniform vec3 u_CameraPosition;

uniform sampler2D g_SkyTexture;

in vec3 positionWS;
in vec2 texcoord;

out vec4 color;

vec3		g_AtmosphereBrightColor=vec3(1.0,1.1,1.4);
vec3		g_AtmosphereDarkColor=vec3(0.6,0.6,0.7);
float		g_FogDensity = 1.0/700.0;

// primitive simulation of non-uniform atmospheric fog
vec3 CalculateFogColor(vec3 pixel_to_light_vector, vec3 pixel_to_eye_vector)
{
	return mix(g_AtmosphereDarkColor,g_AtmosphereBrightColor,0.5*dot(pixel_to_light_vector,-pixel_to_eye_vector)+0.5);
}

void main()
{
	vec3 acolor;
	vec3 pixel_to_light_vector = normalize(u_LightPosition-positionWS);
	vec3 pixel_to_eye_vector = normalize(u_CameraPosition-positionWS);


	color=texture(g_SkyTexture,vec2(texcoord.x,pow(texcoord.y,2.0)));
	acolor = CalculateFogColor(pixel_to_light_vector,pixel_to_eye_vector);
	color.rgb = mix(color.rgb,acolor,pow(clamp(texcoord.y,0.0,1.0),10.0));
	color.a = 1;
}
