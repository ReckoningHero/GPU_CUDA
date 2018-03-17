// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

#include "stdafx.h"
#include "terrain.h"
#include "math_code.h"
#include "image.h"
#include "shaders.h"
#include "nvToolsExt.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif
 
extern bool g_RenderWireframe;
extern bool g_RenderCaustics;

void SetBilinearNoMipmapWrap()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

void SetBilinearNoMipmapClamp()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}

void SetTrilinearWrap()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

void SetTrilinearClamp()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}

void SetAnisotropicWrap()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
}

void CTerrain::RenderSky()
{
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,sky_texture);
    SetBilinearNoMipmapWrap();
    glUniform1i(glGetUniformLocation(sky_program, "g_SkyTexture"), 0);

    glBindVertexArray(sky_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, sky_numvertices);
}

void CTerrain::RenderTerrainShadow()
{
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,heightmap_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_shadow_program, "g_HeightfieldTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,rock_bump_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_shadow_program, "g_RockBumpTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,sand_bump_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_shadow_program, "g_SandBumpTexture"), 2);

    glActiveTexture(GL_TEXTURE3);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,layerdef_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_shadow_program, "g_LayerdefTexture"), 3);

    glBindVertexArray(grid_vao);
    glPatchParameteri ( GL_PATCH_VERTICES, 1);
    glDrawArrays(GL_PATCHES, 0, terrain_numpatches_1d*terrain_numpatches_1d);
}

void CTerrain::RenderTerrain()
{
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,heightmap_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_HeightfieldTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,rock_bump_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_RockBumpTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,sand_bump_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_SandBumpTexture"), 2);

    glActiveTexture(GL_TEXTURE3);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,layerdef_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_LayerdefTexture"), 3);

    glActiveTexture(GL_TEXTURE4);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,slope_diffuse_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_SlopeDiffuseTexture"), 4);

    glActiveTexture(GL_TEXTURE5);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,sand_diffuse_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_SandDiffuseTexture"), 5);

    glActiveTexture(GL_TEXTURE6);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,rock_diffuse_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_RockDiffuseTexture"), 6);

    glActiveTexture(GL_TEXTURE7);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,grass_diffuse_texture);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_GrassDiffuseTexture"), 7);

    glActiveTexture(GL_TEXTURE8);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,rock_microbump_texture);
    SetAnisotropicWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_RockMicroBumpTexture"), 8);

    glActiveTexture(GL_TEXTURE9);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,sand_microbump_texture);
    SetAnisotropicWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_SandMicroBumpTexture"), 9);

    glActiveTexture(GL_TEXTURE10);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,shadowmap_fbo.depth_texture);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glUniform1i(glGetUniformLocation(terrain_program, "g_DepthTexture"), 10);

    glActiveTexture(GL_TEXTURE11);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,water_normal_textures[frame_number%2]);
    SetBilinearNoMipmapWrap();
    glUniform1i(glGetUniformLocation(terrain_program, "g_WaterNormalTexture"), 11);

    glBindVertexArray(grid_vao);
    glPatchParameteri ( GL_PATCH_VERTICES, 1);
    glDrawArrays(GL_PATCHES, 0, terrain_numpatches_1d*terrain_numpatches_1d);
}

void CTerrain::RenderWater()
{
    float m[2];
    //glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,water_displacement_textures[frame_number%2]);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(water_program, "g_WaterDisplacementTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,water_normal_textures[frame_number%2]);
    SetTrilinearWrap();
    glUniform1i(glGetUniformLocation(water_program, "g_WaterNormalTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,reflection_fbo.color_texture);
    SetBilinearNoMipmapClamp();
    glUniform1i(glGetUniformLocation(water_program, "g_ReflectionTexture"), 2);

    glActiveTexture(GL_TEXTURE3);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,refraction_fbo.color_texture);
    SetBilinearNoMipmapClamp();
    glUniform1i(glGetUniformLocation(water_program, "g_RefractionTexture"), 3);

    glActiveTexture(GL_TEXTURE4);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,depthmap_texture);
    SetTrilinearClamp();
    glUniform1i(glGetUniformLocation(water_program, "g_WaterDepthMapTexture"), 4);

    glActiveTexture(GL_TEXTURE5);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,shadowmap_fbo.depth_texture);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glUniform1i(glGetUniformLocation(water_program, "g_DepthTexture"), 5);

    glActiveTexture(GL_TEXTURE6);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,refraction_fbo.depth_texture);
    SetBilinearNoMipmapClamp();
    glUniform1i(glGetUniformLocation(water_program, "g_RefractionDepthTexture"), 6);

    glActiveTexture(GL_TEXTURE7);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,water_bump_texture);
    SetAnisotropicWrap();
    glUniform1i(glGetUniformLocation(water_program, "g_WaterHighFreqNormalTexture"), 7);

    glBindVertexArray(grid_vao);
    glPatchParameteri ( GL_PATCH_VERTICES, 1);
    for(int i=-2;i<=2;i++)
    {
        for(int j=-2;j<=2;j++)
        {
            m[0]=i*terrain_far_range;
            m[1]=j*terrain_far_range;
            glUniform2fv(glGetUniformLocation(water_program, "g_HeightFieldOffset"), 1, (const GLfloat *) m);
            glDrawArrays(GL_PATCHES, 0, terrain_numpatches_1d*terrain_numpatches_1d);

        }
    }
    //glDrawArrays(GL_PATCHES, 0, terrain_numpatches_1d*terrain_numpatches_1d);
    //glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
}

void CTerrain::Render()
{
    float camera_direction[3];

    nvtxRangePushA("Rendering");
    nvtxRangePushA("ShadowMap");

    // rendering to shadowmap
    // setting up shadowmap RT
    glBindFramebuffer(GL_FRAMEBUFFER, shadowmap_fbo.fbo);
    glClearDepthf(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    //glDepthRangef(scene_z_near,scene_z_far);
    glViewport(0,0,shadowmap_size,shadowmap_size);
    SetupLightView();

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    // rendering terrain to shadowmap
    glUseProgram(terrain_shadow_program);

    //setting up uniforms
    glUniform1f(glGetUniformLocation(terrain_shadow_program, "g_StaticTessFactor"), StaticTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_shadow_program, "g_DynamicTessFactor"), DynamicTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_shadow_program, "g_UseDynamicLOD"), UseDynamicTessellation);
    glUniform1f(glGetUniformLocation(terrain_shadow_program, "g_FrustumCullInHS"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_shadow_program, "g_HeightFieldSize"), terrain_gridpoints*terrain_geometry_scale);
    glUniform1f(glGetUniformLocation(terrain_shadow_program, "g_HalfSpaceCullSign"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_shadow_program, "g_HalfSpaceCullPosition"), -100.0f);
    glUniformMatrix4fv(glGetUniformLocation(terrain_shadow_program, "g_ModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) ShadowViewProjMatrix);
    
    camera_direction[0] = -LightPosition[0];
    camera_direction[1] = -LightPosition[1];
    camera_direction[2] = -LightPosition[2];
    vec3Normalize(camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_shadow_program, "g_CameraDirection"), 1, (const GLfloat *) camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_shadow_program, "g_CameraPosition"), 1, (const GLfloat *) CameraPosition);
    glUniform3fv(glGetUniformLocation(terrain_shadow_program, "g_LightPosition"), 1, (const GLfloat *) LightPosition);
    
    RenderTerrainShadow();

	nvtxRangePop();
	nvtxRangePushA("Refraction");

    // rendering to refraction buffer
    SetupRefractionView();
    // setting up refraction RT
    glBindFramebuffer(GL_FRAMEBUFFER, refraction_fbo.fbo);
    glClearColor(0.5f,0.5f,0.5f,1.0f);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,ScreenWidth,ScreenHeight);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    // setting up program
    glUseProgram(terrain_program);
    glUniform1f(glGetUniformLocation(terrain_program, "g_StaticTessFactor"), StaticTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_program, "g_DynamicTessFactor"), DynamicTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_program, "g_UseDynamicLOD"), UseDynamicTessellation);
    glUniform1f(glGetUniformLocation(terrain_program, "g_FrustumCullInHS"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_SkipCausticsCalculation"), 0.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HeightFieldSize"), terrain_gridpoints*terrain_geometry_scale);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HalfSpaceCullSign"), -1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HalfSpaceCullPosition"), 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(terrain_program, "g_ModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) RefractionViewProjMatrix);
    glUniformMatrix4fv(glGetUniformLocation(terrain_program, "g_LightModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) ShadowViewProjMatrix);
    camera_direction[0] = LookAtPosition[0]-CameraPosition[0];
    camera_direction[1] = LookAtPosition[1]-CameraPosition[1];
    camera_direction[2] = LookAtPosition[2]-CameraPosition[2];
    vec3Normalize(camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_CameraDirection"), 1, (const GLfloat *) camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_CameraPosition"), 1, (const GLfloat *) CameraPosition);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_LightPosition"), 1, (const GLfloat *) LightPosition);
    RenderTerrain();

    nvtxRangePop();
    nvtxRangePushA("Reflection");

    // rendering to reflection buffer
    SetupReflectionView();
    // setting up reflection RT
    glBindFramebuffer(GL_FRAMEBUFFER, reflection_fbo.fbo);
    glClearColor(0.5f,0.5f,0.5f,1.0f);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,ScreenWidth,ScreenHeight);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    // setting up program
    glUseProgram(terrain_program);
    glUniform1f(glGetUniformLocation(terrain_program, "g_StaticTessFactor"), StaticTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_program, "g_DynamicTessFactor"), DynamicTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_program, "g_UseDynamicLOD"), UseDynamicTessellation);
    glUniform1f(glGetUniformLocation(terrain_program, "g_FrustumCullInHS"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_SkipCausticsCalculation"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HeightFieldSize"), terrain_gridpoints*terrain_geometry_scale);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HalfSpaceCullSign"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HalfSpaceCullPosition"), 0.8f);
    glUniformMatrix4fv(glGetUniformLocation(terrain_program, "g_ModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) ReflectionViewProjMatrix);
    glUniformMatrix4fv(glGetUniformLocation(terrain_program, "g_LightModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) ShadowViewProjMatrix);
    float campos[3] = {CameraPosition[0],-CameraPosition[1]+1.0f,CameraPosition[2]};
    float lookatpos[3] = {LookAtPosition[0],-LookAtPosition[1]+1.0f,LookAtPosition[2]};
    camera_direction[0] = lookatpos[0]-campos[0];
    camera_direction[1] = lookatpos[1]-campos[1];
    camera_direction[2] = lookatpos[2]-campos[2];
    vec3Normalize(camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_CameraDirection"), 1, (const GLfloat *) camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_CameraPosition"), 1, (const GLfloat *) campos);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_LightPosition"), 1, (const GLfloat *) LightPosition);
    RenderTerrain();

    nvtxRangePop();
    nvtxRangePushA("Sky");

    //setting up program
    glUseProgram(sky_program);
    glUniformMatrix4fv(glGetUniformLocation(sky_program, "u_ModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) ReflectionViewProjMatrix);
    glUniform3fv(glGetUniformLocation(sky_program, "u_LightPosition"), 1, (const GLfloat *) LightPosition);
    glUniform3fv(glGetUniformLocation(sky_program, "u_EyePosition"), 1, (const GLfloat *) CameraPosition);
    RenderSky();

    nvtxRangePop();
    nvtxRangePushA("Color Pass Terrain");

    // rendering to main buffer
    SetupNormalView();
    // setting up main buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f,0.1f,0.5f,1.0f);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,ScreenWidth,ScreenHeight);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    //setting up program
    glUseProgram(terrain_program);
    glUniform1f(glGetUniformLocation(terrain_program, "g_StaticTessFactor"), StaticTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_program, "g_DynamicTessFactor"), DynamicTesselationFactor);
    glUniform1f(glGetUniformLocation(terrain_program, "g_UseDynamicLOD"), UseDynamicTessellation);
    glUniform1f(glGetUniformLocation(terrain_program, "g_FrustumCullInHS"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_SkipCausticsCalculation"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HeightFieldSize"), terrain_gridpoints*terrain_geometry_scale);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HalfSpaceCullSign"), 1.0f);
    glUniform1f(glGetUniformLocation(terrain_program, "g_HalfSpaceCullPosition"), -2.0f);
    glUniformMatrix4fv(glGetUniformLocation(terrain_program, "g_ModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) NormalViewProjMatrix);
    glUniformMatrix4fv(glGetUniformLocation(terrain_program, "g_LightModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) ShadowViewProjMatrix);
    camera_direction[0] = LookAtPosition[0]-CameraPosition[0];
    camera_direction[1] = LookAtPosition[1]-CameraPosition[1];
    camera_direction[2] = LookAtPosition[2]-CameraPosition[2];
    vec3Normalize(camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_CameraDirection"), 1, (const GLfloat *) camera_direction);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_CameraPosition"), 1, (const GLfloat *) CameraPosition);
    glUniform3fv(glGetUniformLocation(terrain_program, "g_LightPosition"), 1, (const GLfloat *) LightPosition);
    RenderTerrain();

    nvtxRangePop();
    nvtxRangePushA("Color Pass Water");

    // rendering water to main buffer
    // setting up program
    //glPolygonMode(GL_FRONT, GL_LINE);
    glEnable(GL_CULL_FACE);
    glUseProgram(water_program);
    glUniform1f(glGetUniformLocation(water_program, "g_StaticTessFactor"), StaticTesselationFactor);
    glUniform1f(glGetUniformLocation(water_program, "g_DynamicTessFactor"), DynamicTesselationFactor/2.0f);
    glUniform1f(glGetUniformLocation(water_program, "g_UseDynamicLOD"), UseDynamicTessellation);
    glUniform1f(glGetUniformLocation(water_program, "g_FrustumCullInHS"), 1.0f);
    glUniform1f(glGetUniformLocation(water_program, "g_HeightFieldSize"), terrain_gridpoints*terrain_geometry_scale);
    glUniformMatrix4fv(glGetUniformLocation(water_program, "g_ModelViewMatrix"), 1, GL_FALSE, (const GLfloat *) NormalViewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(water_program, "g_ModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) NormalViewProjMatrix);
    glUniformMatrix4fv(glGetUniformLocation(water_program, "g_LightModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) ShadowViewProjMatrix);
    camera_direction[0] = LookAtPosition[0]-CameraPosition[0];
    camera_direction[1] = LookAtPosition[1]-CameraPosition[1];
    camera_direction[2] = LookAtPosition[2]-CameraPosition[2];
    vec3Normalize(camera_direction);
    glUniform3fv(glGetUniformLocation(water_program, "g_CameraDirection"), 1, (const GLfloat *) camera_direction);
    glUniform3fv(glGetUniformLocation(water_program, "g_CameraPosition"), 1, (const GLfloat *) CameraPosition);
    glUniform3fv(glGetUniformLocation(water_program, "g_LightPosition"), 1, (const GLfloat *) LightPosition);
    float m[4];
    m[0]=1.0f/ScreenWidth;
    m[1]=1.0f/ScreenHeight;
    glUniform2fv(glGetUniformLocation(water_program, "g_ScreenSizeInv"), 1, (const GLfloat *) m);
    m[0]=scene_z_near;
    glUniform1f(glGetUniformLocation(water_program, "g_ZNear"), m[0]);
    m[0]=scene_z_far;
    glUniform1f(glGetUniformLocation(water_program, "g_ZFar"), m[0]);
    m[0]=-total_time*0.05f;
    m[1]=-total_time*0.04f;
    glUniform2fv(glGetUniformLocation(water_program, "g_WaterTexCoordShift"), 1, (const GLfloat *) m);
    RenderWater();

    nvtxRangePop();
    nvtxRangePushA("Color Pass Sky");

    // rendering sky to main buffer
    // setting up program
    glUseProgram(sky_program);
    glUniformMatrix4fv(glGetUniformLocation(sky_program, "u_ModelViewProjectionMatrix"), 1, GL_FALSE, (const GLfloat *) NormalViewProjMatrix);
    glUniform3fv(glGetUniformLocation(sky_program, "u_LightPosition"), 1, (const GLfloat *) LightPosition);
    glUniform3fv(glGetUniformLocation(sky_program, "u_EyePosition"), 1, (const GLfloat *) CameraPosition);
    RenderSky();

    nvtxRangePop();

    /*
    // rendering quad to main buffer
    glDisable(GL_CULL_FACE);
    glUseProgram(quad_program);

    m[0]=0.7f;
    m[1]=0.7f;
    m[2]=0.3f;
    m[3]=0.3f;
    glUniform4fv(glGetUniformLocation(quad_program, "g_QuadParams"), 1, (const GLfloat *) m);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,refraction_fbo.color_texture);
    SetBilinearNoMipmapWrap();
    glUniform1i(glGetUniformLocation(quad_program, "g_QuadTexture"), 0);

    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkError("glDrawArrays quad");

    m[0]=0.3f;
    m[1]=0.7f;
    m[2]=0.3f;
    m[3]=0.3f;
    glUniform4fv(glGetUniformLocation(quad_program, "g_QuadParams"), 1, (const GLfloat *) m);
    glBindTexture(GL_TEXTURE_2D,reflection_fbo.color_texture);
    SetBilinearNoMipmapWrap();
    glUniform1i(glGetUniformLocation(quad_program, "g_QuadTexture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkError("glDrawArrays quad");

    */
    glEnable(GL_CULL_FACE);

    nvtxRangePop();

    // done rendering
    glutSwapBuffers();
}

float bilinear_interpolation(float fx, float fy, float a, float b, float c, float d)
{
    float s1,s2,s3,s4;
    s1=fx*fy;
    s2=(1-fx)*fy;
    s3=(1-fx)*(1-fy);
    s4=fx*(1-fy);
    return((a*s3+b*s4+c*s1+d*s2));
}

void CTerrain::SetupReflectionView()
{
    float campos[3] = {CameraPosition[0],-CameraPosition[1]+1.0f,CameraPosition[2]};
    float lookatpos[3] = {LookAtPosition[0],-LookAtPosition[1]+1.0f,LookAtPosition[2]};
    mat4CreateView(ReflectionViewMatrix, campos, lookatpos);
    mat4CreateProjection(ReflectionProjMatrix,scene_z_near,scene_z_near/tan(camera_fov*0.5f)*(float)ScreenHeight/(float)ScreenWidth,scene_z_near/tan(camera_fov*0.5f),scene_z_far);
    mat4Mat4Mul(ReflectionViewProjMatrix,ReflectionProjMatrix,ReflectionViewMatrix);
    mat4Inverse(ReflectionViewProjMatrixInv,ReflectionViewProjMatrix);

}

void CTerrain::SetupRefractionView()
{
    mat4CreateView(RefractionViewMatrix, CameraPosition, LookAtPosition);
    mat4CreateProjection(RefractionProjMatrix,scene_z_near,scene_z_near/tan(camera_fov*0.5f)*(float)ScreenHeight/(float)ScreenWidth,scene_z_near/tan(camera_fov*0.5f),scene_z_far);
    mat4Mat4Mul(RefractionViewProjMatrix,RefractionProjMatrix,RefractionViewMatrix);
}
void CTerrain::SetupLightView()
{
    float light_lookat_position[3]={terrain_far_range*0.5f,0,terrain_far_range*0.5f};
    mat4CreateView(ShadowViewMatrix, LightPosition, light_lookat_position);
    float nr;
    nr=sqrt(LightPosition[0]*LightPosition[0]+LightPosition[1]*LightPosition[1]+LightPosition[2]*LightPosition[2])-terrain_far_range;
    mat4CreateOrthoProjection(ShadowProjMatrix,-terrain_far_range*0.71f,terrain_far_range*0.71f,-terrain_far_range*0.71f,terrain_far_range*0.71f,nr,nr+terrain_far_range*2.0f);
    mat4Mat4Mul(ShadowViewProjMatrix,ShadowProjMatrix,ShadowViewMatrix);
    mat4Inverse(ShadowViewProjMatrixInv,ShadowViewProjMatrix);
}

void CTerrain::SetupNormalView()
{
    mat4CreateView(NormalViewMatrix, CameraPosition, LookAtPosition);
    mat4CreateProjection(NormalProjMatrix,scene_z_near,scene_z_near/tan(camera_fov*0.5f)*(float)ScreenHeight/(float)ScreenWidth,scene_z_near/tan(camera_fov*0.5f),scene_z_far);
    mat4Mat4Mul(NormalViewProjMatrix,NormalProjMatrix,NormalViewMatrix);
}


