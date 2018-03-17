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
	
	// conservative frustum culling
	
	vec3 patch_center=vec3(origin[0].x+size[0].x*0.5,texture(g_HeightfieldTexture, texcoord0to1,0).w,origin[0].y+size[0].y*0.5);
	vec3 camera_to_patch_vector =  patch_center-g_CameraPosition;
	vec3 patch_to_camera_direction_vector = g_CameraDirection*dot(camera_to_patch_vector,g_CameraDirection) - camera_to_patch_vector;
	vec3 patch_center_realigned=patch_center+normalize(patch_to_camera_direction_vector)*min(2.0*size[0].x,length(patch_to_camera_direction_vector));
	vec4 patch_screenspace_center = g_ModelViewProjectionMatrix * vec4(patch_center_realigned, 1.0);

	if(((patch_screenspace_center.x/patch_screenspace_center.w>-1.0) && (patch_screenspace_center.x/patch_screenspace_center.w<1.0) 
		&& (patch_screenspace_center.y/patch_screenspace_center.w>-1.0) && (patch_screenspace_center.y/patch_screenspace_center.w<1.0)
		&& (patch_screenspace_center.w>0)) || (length(patch_center-g_CameraPosition)<2.0*size[0].x))
	{
		in_frustum=1.0;
	}
	
	
	//in_frustum=1.0;
	
	if((in_frustum==1.0) || (g_FrustumCullInHS ==0))
	{
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
	else
	{
		gl_TessLevelOuter[0]=-1;
		gl_TessLevelOuter[1]=-1;
		gl_TessLevelOuter[2]=-1;
		gl_TessLevelOuter[3]=-1;
		gl_TessLevelInner[0]=-1;
		gl_TessLevelInner[1]=-1;
	}
}

<<<TETEXT>>>
#version 420 core

layout (quads, fractional_odd_spacing, cw) in;

in			 vec2  tc_origin[];
in			 vec2  tc_size[];

out			 vec4  position;
out centroid vec2  texcoord;
out centroid vec3  normal;
out centroid vec3  positionWS;
out centroid vec4  layerdef;
out centroid vec4  depthmap_scaler;

uniform		float  g_StaticTessFactor;
uniform		float  g_DynamicTessFactor;
uniform		float  g_UseDynamicLOD;
uniform		float  g_SkipCausticsCalculation;
uniform		float  g_HeightFieldSize;
uniform		vec3   g_CameraPosition;
uniform		vec3   g_CameraDirection;
uniform		mat4x4 g_ModelViewProjectionMatrix;
uniform		vec3   g_LightPosition;

uniform sampler2D  g_HeightfieldTexture;
uniform sampler2D  g_LayerdefTexture;
uniform sampler2D  g_SandBumpTexture;
uniform sampler2D  g_RockBumpTexture;
uniform sampler2D  g_WaterNormalTexture;

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

// calculating water refraction caustics intensity
float CalculateWaterCausticIntensity(vec3 worldpos)
{

	float distance_to_camera=length(g_CameraPosition-worldpos);

	vec2 refraction_disturbance;
	vec3 n;
	float m1=0.2;
	float cc1=0;
	float cc2=0;
	float k1=0.35;
	float m2=0.2;
	float k2=0.35;	
	float water_depth=1.5-worldpos.y;

	vec3 pixel_to_light_vector=normalize(g_LightPosition-worldpos);

	worldpos.xz-=worldpos.y*pixel_to_light_vector.xz;
	vec3 pixel_to_water_surface_vector=pixel_to_light_vector*water_depth;
	vec3 refracted_pixel_to_light_vector;

	// tracing approximately refracted rays back to light
	for(float i=-2; i<=2;i+=1)
		for(float j=-2; j<=2;j+=1)
		{
			n=texture(g_WaterNormalTexture,(worldpos.xz*1.0+vec2(i*k1,j*k1)*m1*water_depth)*6.0/512.0,0).rgb;
			refracted_pixel_to_light_vector=m1*(pixel_to_water_surface_vector+vec3(i*k1,0,j*k1))-1.5*vec3(n.x,0,n.z);
			cc1+=0.15*max(0,pow(max(0,dot(normalize(refracted_pixel_to_light_vector),normalize(pixel_to_light_vector))),500.0));
		}		
	return cc1;
}


void main()
{
	vec3 vertexPosition;
	vec4 base_texvalue;
	vec2 texcoord0to1 = (tc_origin[0] + gl_TessCoord.xy * tc_size[0])/g_HeightFieldSize;
	vec3 base_normal;
	vec3 detail_normal;
	vec3 detail_normal_rotated;
	vec4 detail_texvalue;
	float detail_height;
	mat3x3 normal_rotation_matrix;
	float distance_to_camera;
	float detailmap_miplevel;
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
	detail_normal=normalize(2*detail_texvalue.xyz-vec3(1,0,1));
	detail_height=(detail_texvalue.w-0.5)*g_SandBumpHeightScale;

	// rock detail texture
	detail_texvalue=textureLod(g_RockBumpTexture, texcoord0to1*g_RockBumpTexcoordScale,detailmap_miplevel).rbga;
	detail_normal=mix(detail_normal,normalize(2*detail_texvalue.xyz-vec3(1,1.4,1)),layerdef.w);
	detail_height=mix(detail_height,(detail_texvalue.w-0.5)*g_RockBumpHeightScale,layerdef.w);

	// moving vertices by detail height along base normal
	vertexPosition+=base_normal*detail_height;

	//calculating base normal rotation matrix
	normal_rotation_matrix[1]=base_normal;
	normal_rotation_matrix[2]=normalize(cross(vec3(-1.0,0.0,0.0),normal_rotation_matrix[1]));
	normal_rotation_matrix[0]=normalize(cross(normal_rotation_matrix[2],normal_rotation_matrix[1]));

	//applying base rotation matrix to detail normal
	detail_normal_rotated=  normal_rotation_matrix * detail_normal;

	//adding refraction caustics
	float cc=0;
	
	if((g_SkipCausticsCalculation==0) && (vertexPosition.y<0))
	{
		cc=CalculateWaterCausticIntensity(vertexPosition.xyz);
	}
	
	// fading caustics out at distance
	cc*=(200.0/(200.0+distance_to_camera));

	// fading caustics out as we're getting closer to water surface
	cc*=min(1.0,max(0.0,-vertexPosition.y));

	
	// writing output params
    position = g_ModelViewProjectionMatrix * vec4(vertexPosition, 1.0);
    gl_Position = g_ModelViewProjectionMatrix * vec4(vertexPosition, 1.0);
    texcoord = texcoord0to1*g_DiffuseTexcoordScale;
	normal=detail_normal_rotated;
	positionWS = vertexPosition;
	depthmap_scaler=vec4(1.0,1.0,detail_height,cc);
}

<<<FSTEXT>>>
#version 420 core

in			vec4  position;
in centroid vec2  texcoord;
in centroid vec3  normal;
in centroid vec3  positionWS;
in centroid vec4  layerdef;
in centroid vec4  depthmap_scaler;

out			vec4  color;

uniform		vec3   g_CameraPosition;
uniform		vec3   g_LightPosition;
uniform		mat4x4 g_ModelViewProjectionMatrix;
uniform		mat4x4 g_LightModelViewProjectionMatrix;
uniform		float  g_HalfSpaceCullSign;
uniform		float  g_HalfSpaceCullPosition;


uniform sampler2D  g_HeightfieldTexture;
uniform sampler2D  g_LayerdefTexture;
uniform sampler2D  g_SandMicroBumpTexture;
uniform sampler2D  g_RockMicroBumpTexture;
uniform sampler2D  g_SlopeDiffuseTexture;
uniform sampler2D  g_SandDiffuseTexture;
uniform sampler2D  g_RockDiffuseTexture;
uniform sampler2D  g_GrassDiffuseTexture;
uniform sampler2DShadow  g_DepthTexture;


vec3			   g_AtmosphereBrightColor=vec3(1.0,1.1,1.4);
vec3			   g_AtmosphereDarkColor=vec3(0.6,0.6,0.7);
float			   g_FogDensity = 1.0/300.0;

// primitive simulation of non-uniform atmospheric fog
vec3 CalculateFogColor(vec3 pixel_to_light_vector, vec3 pixel_to_eye_vector)
{
	return mix(g_AtmosphereDarkColor,g_AtmosphereBrightColor,0.5*dot(pixel_to_light_vector,-pixel_to_eye_vector)+0.5);
}

void main()
{
	
	vec3 pixel_to_light_vector = normalize(g_LightPosition-positionWS);
	vec3 pixel_to_eye_vector = normalize(g_CameraPosition-positionWS);
	vec3 microbump_normal; 

	mat3x3 normal_rotation_matrix;

	// culling halfspace if needed
	if(g_HalfSpaceCullSign*(positionWS.y-g_HalfSpaceCullPosition)<=0.0)
	{
		discard;
	}
	
	// fetching default microbump normal
	microbump_normal = normalize(2.0*texture(g_SandMicroBumpTexture,texcoord).rbg - vec3 (1.0,1.0,1.0));
	microbump_normal = normalize(mix(microbump_normal,2.0*texture(g_RockMicroBumpTexture,texcoord).rbg - vec3 (1.0,1.0,1.0),layerdef.w));

	//calculating base normal rotation matrix
	normal_rotation_matrix[1]=normal;
	normal_rotation_matrix[2]=normalize(cross(vec3(-1.0,0.0,0.0),normal_rotation_matrix[1]));
	normal_rotation_matrix[0]=normalize(cross(normal_rotation_matrix[2],normal_rotation_matrix[1]));
	microbump_normal=normal_rotation_matrix * microbump_normal ;

	// getting diffuse color
	color=texture(g_SlopeDiffuseTexture,texcoord);
	color=mix(color,texture(g_SandDiffuseTexture,texcoord),layerdef.g*layerdef.g);
	color=mix(color,texture(g_RockDiffuseTexture,texcoord),layerdef.w*layerdef.w);
	color=mix(color,texture(g_GrassDiffuseTexture,texcoord),layerdef.b);

	// adding per-vertex lighting defined by displacement of vertex 
	color*=0.5+0.5*min(1.0,max(0.0,depthmap_scaler.b/3.0f+0.5));

	// calculating pixel position in light view space
	vec4 positionLS = g_LightModelViewProjectionMatrix *vec4(positionWS,1.0);
	positionLS.xyz/=positionLS.w;
	positionLS.x=(positionLS.x+1.0)*0.5;
	positionLS.y=(positionLS.y+1.0)*0.5;
	positionLS.z=(positionLS.z+1.0)*0.5;

	// fetching shadowmap and shading
	float dsf=0.75/1024.0;
	float shadow_factor=0.2*texture(g_DepthTexture,vec3(positionLS.xy,positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(dsf,dsf),positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(-dsf,dsf),positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(dsf,-dsf),positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(-dsf,-dsf),positionLS.z)).r;
	color.rgb*=max(0,dot(pixel_to_light_vector,microbump_normal))*shadow_factor+0.2;


	// adding light from the sky
	color.rgb+=(0.0+0.2*max(0,(dot(vec3(0,1.0,0),microbump_normal))))*vec3(0.2,0.2,0.3);

	// making all a bit brighter, simultaneously pretending the wet surface is darker than normal;
	color.rgb*=0.5+0.8*max(0,min(1,positionWS.y*0.5+0.5));

	// applying refraction caustics
	color.rgb*=(1.0+max(0,0.4+0.6*dot(pixel_to_light_vector,microbump_normal))*depthmap_scaler.a*(0.4+0.6*shadow_factor));

	// applying fog
	color.rgb=mix(CalculateFogColor(pixel_to_light_vector,pixel_to_eye_vector).rgb,color.rgb,min(1,exp(-length(g_CameraPosition-positionWS)*g_FogDensity)));
	color.a=length(g_CameraPosition-positionWS);	
}

