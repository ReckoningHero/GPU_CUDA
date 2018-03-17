<<<VSTEXT>>>
#version 420 core

in vec4 a_PatchParams;

out vec2 origin;
out vec2 size; 

void main()
{
    origin = a_PatchParams.xy;
    size = a_PatchParams.zw;
}


<<<TCTEXT>>>
#version 420 core

layout (vertices=1) out;
in vec2 origin[];
in vec2 size[];
out vec2 tc_origin[];
out vec2 tc_size[];

uniform sampler2D g_HeightfieldTexture;

uniform float g_HeightFieldSize;
uniform vec3 g_CameraPosition;
uniform vec3 g_CameraDirection;
uniform mat4x4 g_ModelViewProjectionMatrix;

uniform float g_StaticTessFactor;
uniform float g_DynamicTessFactor;
uniform float g_UseDynamicLOD;
uniform float g_FrustumCullInHS;

float CalculateTessellationFactor(float distance)
{
	return mix(g_StaticTessFactor,min(40.0,g_DynamicTessFactor*(1.0/(0.0005*distance*distance))),g_UseDynamicLOD);
}

void main()
{    
	float distance_to_camera;
	float tesselation_factor;
	float inside_tessellation_factor=0;
	float in_frustum=0.0;
	vec2 texcoord0to1 = (origin[0] + size[0]/2.0)/g_HeightFieldSize;
	tc_origin[gl_InvocationID] = origin[0];
	tc_size[gl_InvocationID] = size[0];

	texcoord0to1.y=1-texcoord0to1.y;
	
	distance_to_camera=length(g_CameraPosition.xz-origin[0]-vec2(0,size[0].y*0.5));
	tesselation_factor=CalculateTessellationFactor(distance_to_camera);
	gl_TessLevelOuter[0] =  tesselation_factor;
	inside_tessellation_factor+=tesselation_factor;


	distance_to_camera=length(g_CameraPosition.xz-origin[0]-vec2(size[0].x*0.5,0));
	tesselation_factor=CalculateTessellationFactor(distance_to_camera);
	gl_TessLevelOuter[1] =  tesselation_factor;
	inside_tessellation_factor+=tesselation_factor;

	distance_to_camera=length(g_CameraPosition.xz-origin[0]-vec2(size[0].x,size[0].y*0.5));
	tesselation_factor=CalculateTessellationFactor(distance_to_camera);
	gl_TessLevelOuter[2] =  tesselation_factor;
	inside_tessellation_factor+=tesselation_factor;

	distance_to_camera=length(g_CameraPosition.xz-origin[0]-vec2(size[0].x*0.5,size[0].y));
	tesselation_factor=CalculateTessellationFactor(distance_to_camera);
	gl_TessLevelOuter[3] =  tesselation_factor;
	inside_tessellation_factor+=tesselation_factor;
	gl_TessLevelInner[0] = inside_tessellation_factor*0.25;
	gl_TessLevelInner[1] = inside_tessellation_factor*0.25;
}

<<<TETEXT>>>
#version 420 core

layout (quads, fractional_odd_spacing, cw) in;

in			 vec2  tc_origin[];
in			 vec2  tc_size[];

out			float  height;

uniform		float  g_StaticTessFactor;
uniform		float  g_DynamicTessFactor;
uniform		float  g_UseDynamicLOD;
uniform		float  g_HeightFieldSize;
uniform		vec3   g_LightPosition;
uniform		vec3   g_CameraPosition;
uniform		vec3   g_CameraDirection;
uniform		mat4x4 g_ModelViewProjectionMatrix;

uniform sampler2D  g_HeightfieldTexture;
uniform sampler2D  g_LayerdefTexture;
uniform sampler2D  g_SandBumpTexture;
uniform sampler2D  g_RockBumpTexture;

vec2			   g_DiffuseTexcoordScale = vec2(130.0,130.0);
vec2			   g_RockBumpTexcoordScale = vec2(10.0,10.0);
float			   g_RockBumpHeightScale = 3.0;
vec2			   g_SandBumpTexcoordScale = vec2(3.5,3.5);
float			   g_SandBumpHeightScale = 0.5;
float			   g_WaterHeightBumpScale = 1.0;


float CalculateTessellationFactor(float distance)
{
	return mix(g_StaticTessFactor,min(40.0,g_DynamicTessFactor*(1.0/(0.0005*distance*distance))),g_UseDynamicLOD);
}

float CalculateMIPLevelForDisplacementTextures(float distance)
{
	return log2(128.0/CalculateTessellationFactor(distance));
}


void main()
{
	vec3 vertexPosition;
	vec4 base_texvalue;
	vec2 texcoord0to1 = (tc_origin[0] + gl_TessCoord.xy * tc_size[0])/g_HeightFieldSize;
	vec3 base_normal;
	vec4 detail_texvalue;
	float detail_height;
	mat3x3 normal_rotation_matrix;
	float distance_to_camera;
	float detailmap_miplevel;
	vec4 layerdef;
	
	texcoord0to1.y=1-texcoord0to1.y;
		
	// fetching base heightmap,normal and moving vertices along y axis
	base_texvalue=textureLod(g_HeightfieldTexture, texcoord0to1,0);
    base_normal=base_texvalue.xyz;
	base_normal.z=-base_normal.z;
	vertexPosition.xz = tc_origin[0] + gl_TessCoord.xy * tc_size[0];
    vertexPosition.y = base_texvalue.w;

	
	// calculating MIP level for detail texture fetches
	distance_to_camera=length(g_CameraPosition-vertexPosition);
	detailmap_miplevel= CalculateMIPLevelForDisplacementTextures(distance_to_camera);
	
	// fetching layer definition texture
	layerdef=textureLod(g_LayerdefTexture, texcoord0to1,0);
	
	// default detail texture
	detail_texvalue=textureLod(g_SandBumpTexture, texcoord0to1*g_SandBumpTexcoordScale,detailmap_miplevel).rbga;
	detail_height=(detail_texvalue.w-0.5)*g_SandBumpHeightScale;

	// rock detail texture
	detail_texvalue=textureLod(g_RockBumpTexture, texcoord0to1*g_RockBumpTexcoordScale,detailmap_miplevel).rbga;
	detail_height=mix(detail_height,(detail_texvalue.w-0.5)*g_RockBumpHeightScale,layerdef.w);

	// moving vertices by detail height along base normal
	vertexPosition+=base_normal*detail_height;
	
	// moving vertices a bit away from the light
	vertexPosition -=2.5*normalize(g_LightPosition-vertexPosition);

	// writing output params
    gl_Position = g_ModelViewProjectionMatrix * vec4(vertexPosition, 1.0);
    height = (vertexPosition.y)/5.0;
}

<<<FSTEXT>>>
#version 420 core

in			float height;

out			vec4  color;


void main()
{
	color = vec4(height);	
}

