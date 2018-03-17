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

uniform vec3 g_CameraPosition;
uniform vec3 g_CameraDirection;
uniform mat4x4 g_ModelViewProjectionMatrix;
uniform	float g_HeightFieldSize;
uniform	vec2 g_HeightFieldOffset;

uniform float g_StaticTessFactor;
uniform float g_DynamicTessFactor;
uniform float g_UseDynamicLOD;
uniform float g_FrustumCullInHS;


float CalculateTessellationFactor(float distance)
{
	return mix(g_StaticTessFactor,min(20.0,g_DynamicTessFactor*(1.0/(0.0002*distance*distance))),g_UseDynamicLOD);
}

void main()
{    
	float distance_to_camera;
	float tesselation_factor;
	float inside_tessellation_factor=0;
	float in_frustum=0.0;
	vec2 texcoord0to1 = (origin[0] + g_HeightFieldOffset + size[0]/2.0)/g_HeightFieldSize;
	tc_origin[gl_InvocationID] = origin[0] + g_HeightFieldOffset;
	tc_size[gl_InvocationID] = size[0];

	texcoord0to1.y=1-texcoord0to1.y;
	
	// conservative frustum culling
	
	vec3 patch_center=vec3(origin[0].x+size[0].x*0.5 + g_HeightFieldOffset.x,0,origin[0].y+size[0].y*0.5 + g_HeightFieldOffset.y);
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

	
	if((in_frustum==1.0) || (g_FrustumCullInHS ==0))
	{
		distance_to_camera=length(g_CameraPosition.xz-origin[0] - g_HeightFieldOffset - vec2(0,size[0].y*0.5));
		tesselation_factor=CalculateTessellationFactor(distance_to_camera);
		gl_TessLevelOuter[0] =  tesselation_factor;
		inside_tessellation_factor+=tesselation_factor;


		distance_to_camera=length(g_CameraPosition.xz-origin[0] - g_HeightFieldOffset-vec2(size[0].x*0.5,0));
		tesselation_factor=CalculateTessellationFactor(distance_to_camera);
		gl_TessLevelOuter[1] =  tesselation_factor;
		inside_tessellation_factor+=tesselation_factor;

		distance_to_camera=length(g_CameraPosition.xz-origin[0] - g_HeightFieldOffset-vec2(size[0].x,size[0].y*0.5));
		tesselation_factor=CalculateTessellationFactor(distance_to_camera);
		gl_TessLevelOuter[2] =  tesselation_factor;
		inside_tessellation_factor+=tesselation_factor;

		distance_to_camera=length(g_CameraPosition.xz-origin[0] - g_HeightFieldOffset-vec2(size[0].x*0.5,size[0].y));
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
out centroid vec4  depthmap_scaler;

uniform		float  g_StaticTessFactor;
uniform		float  g_DynamicTessFactor;
uniform		float  g_UseDynamicLOD;
uniform		float  g_HeightFieldSize;
uniform		vec3   g_CameraPosition;
uniform		vec3   g_CameraDirection;
uniform		mat4x4 g_ModelViewProjectionMatrix;

uniform sampler2D  g_WaterNormalTexture;
uniform sampler2D  g_WaterDisplacementTexture;
uniform sampler2D  g_WaterDepthMapTexture;


float			   g_WaterHeightBumpScale = 0.8;
float			   g_WaterChopScale = 0.8;
float			   g_WaterTextureCoordScale = 6.0;


float CalculateTessellationFactor(float distance)
{
	return mix(g_StaticTessFactor,g_DynamicTessFactor*(1/(0.015*distance)),g_UseDynamicLOD);
}

float CalculateMIPLevelForDisplacementTextures(float distance)
{
	return 0;//log2(128.0/CalculateTessellationFactor(distance));
}


void main()
{
	vec3 vertexPosition;
	vec4 base_texvalue;
	vec2 texcoord0to1 = (tc_origin[0] + gl_TessCoord.xy * tc_size[0])/g_HeightFieldSize;
	vec3 base_normal;
	float distance_to_camera;
	float watertextures_miplevel;
	vec4 depthmap;
	//texcoord0to1.y=1-texcoord0to1.y;

	// fething depthmap
	depthmap = textureLod(g_WaterDepthMapTexture, vec2(texcoord0to1.x,1.0-texcoord0to1.y),0);
	
	// vertex position before displacement
	vertexPosition.xz = tc_origin[0] + gl_TessCoord.xy * tc_size[0];
    vertexPosition.y = 0;
    

	
	// calculating MIP level for texture fetches
	distance_to_camera =length(g_CameraPosition-vertexPosition);
	watertextures_miplevel = CalculateMIPLevelForDisplacementTextures(distance_to_camera);
	
	// fetching & applying water displacement
	base_texvalue=textureLod(g_WaterDisplacementTexture, texcoord0to1*g_WaterTextureCoordScale,watertextures_miplevel);
	vertexPosition.y += base_texvalue.y*g_WaterHeightBumpScale*(0.2+depthmap.g*0.8);
	vertexPosition.xz -= base_texvalue.xz*g_WaterChopScale*depthmap.g*(0.9+depthmap.g*0.1);
    
	// fetching water normal
	base_texvalue=textureLod(g_WaterNormalTexture, texcoord0to1*g_WaterTextureCoordScale,watertextures_miplevel);
	base_normal = base_texvalue.rgb;
	base_normal = mix(vec3(0.0,1.0,0.0),base_normal,(0.2+depthmap.g*0.8));



	// writing output params
    position = g_ModelViewProjectionMatrix * vec4(vertexPosition, 1.0);
    position.xyz/=position.w;
    gl_Position = g_ModelViewProjectionMatrix * vec4(vertexPosition, 1.0);
    texcoord = texcoord0to1;
	normal=base_normal;
	positionWS = vertexPosition;
	depthmap_scaler=depthmap;
}

<<<FSTEXT>>>
#version 420 core

in			vec4  position;
in centroid vec2  texcoord;
in centroid vec3  normal;
in centroid vec3  positionWS;
in centroid vec4  depthmap_scaler;

out			vec4  color;

uniform		vec3   g_CameraPosition;
uniform		vec3   g_LightPosition;
uniform		mat4x4 g_ModelViewMatrix;
uniform		mat4x4 g_ModelViewProjectionMatrix;
uniform		mat4x4 g_LightModelViewProjectionMatrix;
uniform		vec2   g_ScreenSizeInv;
uniform		float  g_ZNear;
uniform		float  g_ZFar;
uniform		vec2   g_WaterTexCoordShift;

uniform		sampler2D  g_ReflectionTexture;
uniform		sampler2D  g_RefractionTexture;
uniform		sampler2D  g_RefractionDepthTexture;
uniform		sampler2D  g_WaterHighFreqNormalTexture;

uniform		sampler2DShadow  g_DepthTexture;


			vec3   g_AtmosphereBrightColor=vec3(1.0,1.1,1.4);
			vec3   g_AtmosphereDarkColor=vec3(0.6,0.6,0.7);
			float  g_FogDensity = 1.0/300.0;
			vec3   g_WaterDeepColor=vec3(0.1,0.1,0.2);
			vec3   g_WaterScatterColor=vec3(0.3,0.7,0.6);
			vec3   g_WaterSpecularColor=vec3(1.0,1.0,1.0);
			float  g_WaterSpecularIntensity=350.0;
			float  g_WaterSpecularPower=500.0;
			vec2   g_WaterColorIntensity=vec2(0.1,0.1);


// primitive simulation of non-uniform atmospheric fog
vec3 CalculateFogColor(vec3 pixel_to_light_vector, vec3 pixel_to_eye_vector)
{
	return mix(g_AtmosphereDarkColor,g_AtmosphereBrightColor,0.5*dot(pixel_to_light_vector,-pixel_to_eye_vector)+0.5);
}

float GetRefractionDepth(vec2 position)
{
	return textureLod(g_RefractionDepthTexture,position,0).r;
}

float GetConservativeRefractionDepth(vec2 position)
{
	float result =      textureLod(g_RefractionDepthTexture,position + vec2(-0.01,0.01),0).r;
	result = min(result,textureLod(g_RefractionDepthTexture,position + vec2(0.01,-0.01),0).r);
	result = min(result,textureLod(g_RefractionDepthTexture,position + vec2(-0.01,-0.01),0).r);
	result = min(result,textureLod(g_RefractionDepthTexture,position - vec2(0.01,0.01),0).r);
	return result;
}

void main()
{
	
	vec3 pixel_to_light_vector = normalize(g_LightPosition-positionWS);
	vec3 pixel_to_eye_vector = normalize(g_CameraPosition-positionWS);
	vec3 reflected_eye_to_pixel_vector;
	vec3 water_normal;
	mat3x3 normal_rotation_matrix;

	float fresnel_factor;
	float diffuse_factor;
	float specular_factor;
	float scatter_factor;
	vec4 refraction_color;
	vec4 reflection_color;
	vec4 disturbance_eyespace;
	vec2 reflection_disturbance = vec2(0);
	vec2 refraction_disturbance = vec2(0);
	vec4 water_color;
	float water_depth;

	color.rgb = vec3(0.5);

	// calculating pixel position in light view space
	vec4 positionLS = g_LightModelViewProjectionMatrix *vec4(positionWS,1.0);
	positionLS.xyz/=positionLS.w;
	positionLS.x=(positionLS.x+1.0)*0.5;
	positionLS.y=(positionLS.y+1.0)*0.5;
	positionLS.z=(positionLS.z+1.0)*0.5;

	// fetching shadowmap and shading
	float dsf=0.75/4096.0;
	float shadow_factor=0.2*texture(g_DepthTexture,vec3(positionLS.xy,positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(dsf,dsf),positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(-dsf,dsf),positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(dsf,-dsf),positionLS.z)).r;
	shadow_factor+=0.2*texture(g_DepthTexture,vec3(positionLS.xy+vec2(-dsf,-dsf),positionLS.z)).r;

	// need more high frequency bumps for plausible water surface, so creating normal defined by 2 instances of same bump texture
	  //water_normal = texture(g_WaterHighFreqNormalTexture,positionWS.xz*(5.0/128.0)).rgb*vec3(1.0,1.0,-1.0);
	  //water_normal += texture(g_WaterHighFreqNormalTexture,positionWS.xz*0.2).rgb*vec3(1.0,1.0,-1.0) + vec3(0.0,1.0,0.0);
	  water_normal = texture(g_WaterHighFreqNormalTexture,positionWS.xz*vec2(1.0,-1.0)*0.05+g_WaterTexCoordShift).rbg*vec3(2.0) - vec3(1.0,0.0,1.0);
	  water_normal += texture(g_WaterHighFreqNormalTexture,texcoord*vec2(1.0,-1.0)*40.0+g_WaterTexCoordShift.yx*3.0).rbg*vec3(2.0) - vec3(1.0,0.0,1.0);
	  water_normal += texture(g_WaterHighFreqNormalTexture,texcoord*vec2(1.0,-1.0)*120.0+g_WaterTexCoordShift*6.0).rbg*vec3(2.0) - vec3(1.0,0.0,1.0);
	  water_normal += texture(g_WaterHighFreqNormalTexture,texcoord*vec2(1.0,-1.0)*900.0+g_WaterTexCoordShift*9.0).rbg*vec3(2.0) - vec3(1.0,-2.0,1.0);
	  

	
	// calculating base normal rotation matrix
	normal_rotation_matrix[1]=normal.xyz;
	normal_rotation_matrix[2]=normalize(cross(vec3(-1.0,0.0,0.0),normal_rotation_matrix[1]));
	normal_rotation_matrix[0]=normalize(cross(normal_rotation_matrix[2],normal_rotation_matrix[1]));


	// applying base normal rotation matrix to high frequency bump normal
	water_normal=normal_rotation_matrix * water_normal;
	
	// fading normal to vert near shores
	water_normal = mix(vec3(0.0,1.0,0.0),water_normal,(0.2+depthmap_scaler.g*0.5));
	water_normal=normalize(water_normal);
	

	
	// simulating scattering/double refraction: light hits the side of wave, travels some distance in water, and leaves wave on the other side
	// it's difficult to do it physically correct without photon mapping/ray tracing, so using simple but plausible emulation below
	
	// only the crests of water waves generate double refracted light
	scatter_factor=0.25*max(0,positionWS.y*0.05+0.55);

	// the waves that lie between camera and light projection on water plane generate maximal amount of double refracted light 
	scatter_factor*=shadow_factor*pow(max(0.0,dot(normalize(vec3(pixel_to_light_vector.x,0.0,pixel_to_light_vector.z)),-pixel_to_eye_vector)),2.0);
	
	// the slopes of waves that are oriented back to light generate maximal amount of double refracted light 
	scatter_factor*=pow(max(0.0,1.0-dot(pixel_to_light_vector,water_normal)),5.0);
	
	// water crests gather more light than lobes, so more light is scattered under the crests
	scatter_factor+=shadow_factor*1.5*g_WaterColorIntensity.y*max(0,positionWS.y+1.0)*
		// the scattered light is best seen if observing direction is normal to slope surface
		max(0,dot(pixel_to_eye_vector,water_normal))*
		// fading scattered light out at distance and if viewing direction is vertical to avoid unnatural look
		max(0,1-pixel_to_eye_vector.y)*(300.0/(300+length(g_CameraPosition-positionWS)));

	// fading scatter out by 90% near shores so it looks better
	scatter_factor*=0.1+0.9*depthmap_scaler.g;

	// calculating fresnel factor 
	float r=(1.1-1.0)/(1.1+1.0);
	fresnel_factor = max(0.0,min(1.0,r+(1.0-r)*pow(1.0-dot(water_normal,pixel_to_eye_vector),4.3)));

	// calculating specular factor
	reflected_eye_to_pixel_vector=-pixel_to_eye_vector+2.0*dot(pixel_to_eye_vector,water_normal)*water_normal;
	specular_factor=shadow_factor*fresnel_factor*pow(max(0,dot(pixel_to_light_vector,reflected_eye_to_pixel_vector)),g_WaterSpecularPower);

	// calculating diffuse intensity of water surface itself
	diffuse_factor=g_WaterColorIntensity.x+g_WaterColorIntensity.y*max(0,dot(pixel_to_light_vector,water_normal));

	
	
	// calculating disturbance which has to be applied to planar reflections/refractions to give plausible results
	disturbance_eyespace=g_ModelViewMatrix * vec4(water_normal.x,0,water_normal.z,0);

	reflection_disturbance=vec2(disturbance_eyespace.x,disturbance_eyespace.z)*0.05;
	refraction_disturbance=vec2(disturbance_eyespace.x,disturbance_eyespace.y)*0.03*
		// fading out refraction disturbance at distance so reflection doesn't look noisy at distance
		(140.0/(140.0+length(g_CameraPosition-positionWS)));
	
	// calculating correction that shifts reflection up/down according to water wave Y position
	vec4 projected_waveheight = g_ModelViewProjectionMatrix * vec4(positionWS.x,positionWS.y,positionWS.z,1.0);
	float waveheight_correction=-0.5*projected_waveheight.y/projected_waveheight.w;
	projected_waveheight = g_ModelViewProjectionMatrix * vec4(positionWS.x,-0.8,positionWS.z,1.0);
	waveheight_correction+=0.5*projected_waveheight.y/projected_waveheight.w;
	reflection_disturbance.y=max(-0.15,-waveheight_correction+reflection_disturbance.y);

	
	// picking refraction depth at non-displaced point, need it to scale the refraction texture displacement amount according to water depth
	float refraction_depth=GetRefractionDepth(position.xy*0.5 + vec2(0.5));
	refraction_depth=g_ZFar*g_ZNear/(g_ZFar - refraction_depth*(g_ZFar-g_ZNear));
	
	vec4 vertex_in_viewspace = g_ModelViewMatrix * vec4(positionWS,1.0);
	water_depth = refraction_depth + vertex_in_viewspace.z;
	float nondisplaced_water_depth=water_depth;
	
	
	// scaling refraction texture displacement amount according to water depth, with some limit
	refraction_disturbance*=min(4.0,water_depth);

	// picking refraction depth again, now at displaced point, need it to calculate correct water depth
	refraction_depth=GetRefractionDepth(position.xy*0.5 + vec2(0.5)+refraction_disturbance );
	refraction_depth=g_ZFar*g_ZNear/(g_ZFar - refraction_depth*(g_ZFar-g_ZNear));
	vertex_in_viewspace = g_ModelViewMatrix * vec4(positionWS,1.0);
	water_depth = refraction_depth + vertex_in_viewspace.z;


	// zeroing displacement for points where displaced position points at geometry which is actually closer to the camera than the water surface
	float conservative_refraction_depth=GetConservativeRefractionDepth(position.xy*0.5+vec2(0.5)+refraction_disturbance);
	conservative_refraction_depth=g_ZFar*g_ZNear/(g_ZFar-conservative_refraction_depth*(g_ZFar-g_ZNear));
	vertex_in_viewspace=g_ModelViewMatrix * vec4(positionWS,1.0);
	float conservative_water_depth=conservative_refraction_depth+vertex_in_viewspace.z;


	if(conservative_water_depth<0)
	{
		refraction_disturbance=vec2(0.0);
		water_depth=nondisplaced_water_depth;
	}
	water_depth=max(0.0,water_depth);

	
	
	// getting reflection and refraction color at disturbed texture coordinates
	reflection_color=textureLod(g_ReflectionTexture,vec2(position.x*0.5 + 0.5,0.5-position.y*0.5)+reflection_disturbance,0);
	refraction_color=textureLod(g_RefractionTexture,position.xy*0.5 + vec2(0.5)+refraction_disturbance,0);
	
	// fading reflection color to half if reflected vector points below water surface
	//reflection_color.rgb *= 1.0 - 0.4*max(0.0,min(1.0,-reflected_eye_to_pixel_vector.y*4.0));

	// calculating water color at max depth
	water_color.rgb=vec3(0.0);
	water_color.rgb=mix(CalculateFogColor(pixel_to_light_vector,pixel_to_eye_vector).rgb,water_color.rgb,min(1,exp(-length(g_CameraPosition-positionWS)*g_FogDensity)));
	
	// fading fresnel factor to 0 to soften water surface edges
	fresnel_factor*=min(1,water_depth*5.0);

	// fading refraction color to water color according to distance that refracted ray travels in water 
	refraction_color=mix(water_color,refraction_color,min(1,1.0*exp(-water_depth/8.0)));
	
	// combining final water color
	color.rgb=mix(refraction_color.rgb,reflection_color.rgb,fresnel_factor);
	color.rgb+=g_WaterSpecularIntensity*specular_factor*g_WaterSpecularColor*fresnel_factor;
	color.rgb+=g_WaterScatterColor*scatter_factor;
	color.rgb+=0.3*diffuse_factor*g_WaterDeepColor;
	
	color.a=1.0;

}

