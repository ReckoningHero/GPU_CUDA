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

#ifndef PI
#define PI 3.14159265358979323846f
#endif
 
extern bool g_RenderWireframe;
extern bool g_RenderCaustics;

int gp_wrap( int a)
{
    if(a<0) return (a+terrain_gridpoints);
    if(a>=terrain_gridpoints) return (a-terrain_gridpoints);
    return a;
}

int CreateFBO(GLuint width, GLuint height, GLuint internalformat, GLuint format, GLuint type, GLuint mipmapped, GLuint depthformat, fbo_def_type* fbo_ptr) 
            // width, height, format, internalformat, type, mipmapped, depthformat
{
    GLuint status;
    int fnresult;

    glGenFramebuffersEXT(1,&(fbo_ptr->fbo));

    if(internalformat)
    {
        glGenTextures(1,&(fbo_ptr->color_texture));
        glBindTexture(GL_TEXTURE_2D,fbo_ptr->color_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);
        if(mipmapped) 
        {
            glGenerateMipmapEXT(GL_TEXTURE_2D);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }
        else
        {
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_ptr->fbo);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbo_ptr->color_texture, 0);
    }

    if(depthformat)
    {
        glGenTextures(1,&(fbo_ptr->depth_texture));
        glBindTexture(GL_TEXTURE_2D,fbo_ptr->depth_texture);
        checkError ("depthformat glBindTexture");
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glTexImage2D(GL_TEXTURE_2D, 0, depthformat, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_ptr->fbo);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, fbo_ptr->depth_texture, 0);
        //GLint a ;
        //glGetIntegerv(GL_READ_BUFFER, &a);
        //fprintf(stderr,"\n0x%08x",a);
        if(!internalformat)
        {
            glReadBuffer(GL_NONE);
            glDrawBuffer(GL_NONE);
        }
    }
    status=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    fnresult=0;
    switch(status) 
    {                                          
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            fbo_ptr->width=width;
            fbo_ptr->height=height;
            if(internalformat)
            {
                fprintf(stderr,"\nFBO created: %4ix%4i [%s fmt=0x%4x ifmt=0x%4x] %s",width,height,mipmapped?"mipmapped":"nomipmaps", format,internalformat,depthformat?"+ depth":"");
            }
            else
            {
                fprintf(stderr,"\nFBO created: %4ix%4i [%s fmt=0x0000 ifmt=0x0000] %s",width,height,"nocolor  ", depthformat?"+ depth":"");
            }
            fnresult=1;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            fprintf(stderr,"\ncan't create FBO: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT error occurs");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            fprintf(stderr,"\ncan't create FBO: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT error occurs");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            fprintf(stderr,"\ncan't create FBO: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT error occurs");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            fprintf(stderr,"\ncan't create FBO: GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT error occurs");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            fprintf(stderr,"\ncan't create FBO: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT error occurs");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            fprintf(stderr,"\ncan't create FBO: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT error occurs");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            fprintf(stderr,"\ncan't create FBO: GL_FRAMEBUFFER_UNSUPPORTED_EXT error occurs");
            break;
        default:
            fprintf(stderr,"\ncan't create FBO: unknown error occurs");
            break;
    }
        
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    return(fnresult);     
}

void CTerrain::Initialize()
{
    LoadShaders();
    CreateTerrain();
    CreateSky();
    CreateQuad();
    CreateFBOs();
    LoadTextures();
    CameraPosition[0] = 365.0f;
    CameraPosition[1] = 6.0f;
    CameraPosition[2] = 166.0f;
    LookAtPosition[0] = 330.0f;
    LookAtPosition[1] = -11.0f;
    LookAtPosition[2] = 259.0f;
    LightPosition[0] = -10000.0f;
    LightPosition[1] = 6500.0f;
    LightPosition[2] = 10000.0f;

    StaticTesselationFactor = 5.0f;
    DynamicTesselationFactor = 25.0f;
    UseDynamicTessellation = 1.0f;

    ViewPointIndex = 0;
}

void CTerrain::DeInitialize()
{
    // All the GL stuff has to be freed here
}

void CTerrain::CreateFBOs()
{
    CreateFBO(ScreenWidth,ScreenHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0, GL_DEPTH_COMPONENT24, &reflection_fbo);
    CreateFBO(ScreenWidth,ScreenHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0, GL_DEPTH_COMPONENT24, &refraction_fbo);
    CreateFBO(shadowmap_size,shadowmap_size, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0, GL_DEPTH_COMPONENT32F, &shadowmap_fbo);
    checkError ("CreateBuffers");
}

void CTerrain::CreateTerrain()
{
    int i,j,k,l;
    float x,z;
    int ix,iz;
    float * backterrain;
    float vec1[3],vec2[3],vec3[3];
    int currentstep=terrain_gridpoints;
    float mv,rm;
    float offset=0,yscale=0,maxheight=0,minheight=0;

    float *height_linear_array;
    float *patches_rawdata;

    backterrain = (float *) malloc((terrain_gridpoints+1)*(terrain_gridpoints+1)*sizeof(float));
    rm=terrain_fractalinitialvalue;
    backterrain[0]=0;
    backterrain[0+terrain_gridpoints*terrain_gridpoints]=0;
    backterrain[terrain_gridpoints]=0;
    backterrain[terrain_gridpoints+terrain_gridpoints*terrain_gridpoints]=0;
    currentstep=terrain_gridpoints;
    srand(12);
    
    // generating fractal terrain using square-diamond method
    while (currentstep>1)
    {
        //square step;
        i=0;
        j=0;


        while (i<terrain_gridpoints)
        {
            j=0;
            while (j<terrain_gridpoints)
            {
                
                mv=backterrain[i+terrain_gridpoints*j];
                mv+=backterrain[(i+currentstep)+terrain_gridpoints*j];
                mv+=backterrain[(i+currentstep)+terrain_gridpoints*(j+currentstep)];
                mv+=backterrain[i+terrain_gridpoints*(j+currentstep)];
                mv/=4.0;
                backterrain[i+currentstep/2+terrain_gridpoints*(j+currentstep/2)]=(float)(mv+rm*((rand()%1000)/1000.0f-0.5f));
                j+=currentstep;
            }
        i+=currentstep;
        }

        //diamond step;
        i=0;
        j=0;

        while (i<terrain_gridpoints)
        {
            j=0;
            while (j<terrain_gridpoints)
            {

                mv=0;
                mv=backterrain[i+terrain_gridpoints*j];
                mv+=backterrain[(i+currentstep)+terrain_gridpoints*j];
                mv+=backterrain[(i+currentstep/2)+terrain_gridpoints*(j+currentstep/2)];
                mv+=backterrain[i+currentstep/2+terrain_gridpoints*gp_wrap(j-currentstep/2)];
                mv/=4;
                backterrain[i+currentstep/2+terrain_gridpoints*j]=(float)(mv+rm*((rand()%1000)/1000.0f-0.5f));

                mv=0;
                mv=backterrain[i+terrain_gridpoints*j];
                mv+=backterrain[i+terrain_gridpoints*(j+currentstep)];
                mv+=backterrain[(i+currentstep/2)+terrain_gridpoints*(j+currentstep/2)];
                mv+=backterrain[gp_wrap(i-currentstep/2)+terrain_gridpoints*(j+currentstep/2)];
                mv/=4;
                backterrain[i+terrain_gridpoints*(j+currentstep/2)]=(float)(mv+rm*((rand()%1000)/1000.0f-0.5f));

                mv=0;
                mv=backterrain[i+currentstep+terrain_gridpoints*j];
                mv+=backterrain[i+currentstep+terrain_gridpoints*(j+currentstep)];
                mv+=backterrain[(i+currentstep/2)+terrain_gridpoints*(j+currentstep/2)];
                mv+=backterrain[gp_wrap(i+currentstep/2+currentstep)+terrain_gridpoints*(j+currentstep/2)];
                mv/=4;
                backterrain[i+currentstep+terrain_gridpoints*(j+currentstep/2)]=(float)(mv+rm*((rand()%1000)/1000.0f-0.5f));

                mv=0;
                mv=backterrain[i+currentstep+terrain_gridpoints*(j+currentstep)];
                mv+=backterrain[i+terrain_gridpoints*(j+currentstep)];
                mv+=backterrain[(i+currentstep/2)+terrain_gridpoints*(j+currentstep/2)];
                mv+=backterrain[i+currentstep/2+terrain_gridpoints*gp_wrap(j+currentstep/2+currentstep)];
                mv/=4;
                backterrain[i+currentstep/2+terrain_gridpoints*(j+currentstep)]=(float)(mv+rm*((rand()%1000)/1000.0f-0.5f));
                j+=currentstep;
            }
            i+=currentstep;
        }
        //changing current step;
        currentstep/=2;
        rm*=terrain_fractalfactor;
    }   
    fprintf(stderr,"\nTerrain fractal data generated");

    // scaling to minheight..maxheight range
    for (i=0;i<terrain_gridpoints+1;i++)
        for (j=0;j<terrain_gridpoints+1;j++)
        {
            height[i][j]=backterrain[i+terrain_gridpoints*j];
        }
    maxheight=height[0][0];
    minheight=height[0][0];
    for(i=0;i<terrain_gridpoints+1;i++)
        for(j=0;j<terrain_gridpoints+1;j++)
        {
            if(height[i][j]>maxheight) maxheight=height[i][j];
            if(height[i][j]<minheight) minheight=height[i][j];
        }
    offset=minheight-terrain_minheight;
    yscale=(terrain_maxheight-terrain_minheight)/(maxheight-minheight);

    for(i=0;i<terrain_gridpoints+1;i++)
        for(j=0;j<terrain_gridpoints+1;j++)
        {
            height[i][j]-=minheight;
            height[i][j]*=yscale;
            height[i][j]+=terrain_minheight;
        }

    // moving down edges of heightmap   
    for (i=0;i<terrain_gridpoints+1;i++)
        for (j=0;j<terrain_gridpoints+1;j++)
        {
            mv=(float)((i-terrain_gridpoints/2.0f)*(i-terrain_gridpoints/2.0f)+(j-terrain_gridpoints/2.0f)*(j-terrain_gridpoints/2.0f));
            rm=(float)((terrain_gridpoints*0.8f)*(terrain_gridpoints*0.8f)/4.0f);
            if(mv>rm)
            {
                height[i][j]-=((mv-rm)/1000.0f)*terrain_geometry_scale;
            }
            if(height[i][j]<terrain_minheight)
            {
                height[i][j]=terrain_minheight;
            }
        }   


    // terrain banks
    for(k=0;k<10;k++)
    {   
        for(i=0;i<terrain_gridpoints+1;i++)
            for(j=0;j<terrain_gridpoints+1;j++)
            {
                mv=height[i][j];
                if((mv)>0.02f) 
                {
                    mv-=0.02f;
                }
                if(mv<-0.02f) 
                {
                    mv+=0.02f;
                }
                height[i][j]=mv;
            }
    }

    // smoothing 
    for(k=0;k<terrain_smoothsteps;k++)
    {   
        for(i=0;i<terrain_gridpoints+1;i++)
            for(j=0;j<terrain_gridpoints+1;j++)
            {

                vec1[0]=2*terrain_geometry_scale;
                vec1[1]=terrain_geometry_scale*(height[gp_wrap(i+1)][j]-height[gp_wrap(i-1)][j]);
                vec1[2]=0;
                vec2[0]=0;
                vec2[1]=-terrain_geometry_scale*(height[i][gp_wrap(j+1)]-height[i][gp_wrap(j-1)]);
                vec2[2]=-2*terrain_geometry_scale;
                vec3CrossProductNormalized(vec3,vec1,vec2);
                
                if(((vec3[1]>terrain_rockfactor)||(height[i][j]<1.2f))) 
                {
                    rm=terrain_smoothfactor1;
                    mv=height[i][j]*(1.0f-rm) +rm*0.25f*(height[gp_wrap(i-1)][j]+height[i][gp_wrap(j-1)]+height[gp_wrap(i+1)][j]+height[i][gp_wrap(j+1)]);
                    backterrain[i+terrain_gridpoints*j]=mv;
                }
                else
                {
                    rm=terrain_smoothfactor2;
                    mv=height[i][j]*(1.0f-rm) +rm*0.25f*(height[gp_wrap(i-1)][j]+height[i][gp_wrap(j-1)]+height[gp_wrap(i+1)][j]+height[i][gp_wrap(j+1)]);
                    backterrain[i+terrain_gridpoints*j]=mv;
                }

            }
            for (i=0;i<terrain_gridpoints+1;i++)
                for (j=0;j<terrain_gridpoints+1;j++)
                {
                    height[i][j]=(backterrain[i+terrain_gridpoints*j]);
                }
    }
    for(i=0;i<terrain_gridpoints+1;i++)
        for(j=0;j<terrain_gridpoints+1;j++)
        {
            rm=0.5f;
            mv=height[i][j]*(1.0f-rm) +rm*0.25f*(height[gp_wrap(i-1)][j]+height[i][gp_wrap(j-1)]+height[gp_wrap(i+1)][j]+height[i][gp_wrap(j+1)]);
            backterrain[i+terrain_gridpoints*j]=mv;
        }
    for (i=0;i<terrain_gridpoints+1;i++)
        for (j=0;j<terrain_gridpoints+1;j++)
        {
            height[i][j]=(backterrain[i+terrain_gridpoints*j]);
        }


    free(backterrain);

    //calculating normals
    for (i=0;i<terrain_gridpoints+1;i++)
        for (j=0;j<terrain_gridpoints+1;j++)
        {
            vec1[0]=2*terrain_geometry_scale;
            vec1[1]=terrain_geometry_scale*(height[gp_wrap(i+1)][j]-height[gp_wrap(i-1)][j]);
            vec1[2]=0;
            vec2[0]=0;
            vec2[1]=-terrain_geometry_scale*(height[i][gp_wrap(j+1)]-height[i][gp_wrap(j-1)]);
            vec2[2]=-2*terrain_geometry_scale;
            vec3CrossProductNormalized(normal[i][j],vec1,vec2);

        }
    fprintf(stderr,"\nTerrain postprocessed");

    // buiding layerdef 
    GLubyte* temp_layerdef_map_texture_pixels=(GLubyte *)malloc(terrain_layerdef_map_texture_size*terrain_layerdef_map_texture_size*4);
    GLubyte* layerdef_map_texture_pixels=(GLubyte *)malloc(terrain_layerdef_map_texture_size*terrain_layerdef_map_texture_size*4);
    for(i=0;i<terrain_layerdef_map_texture_size;i++)
        for(j=0;j<terrain_layerdef_map_texture_size;j++)
        {
            x=(float)(terrain_gridpoints)*((float)i/(float)terrain_layerdef_map_texture_size);
            z=(float)(terrain_gridpoints)*((float)j/(float)terrain_layerdef_map_texture_size);
            ix=(int)floor(x);
            iz=(int)floor(z);
            rm=bilinear_interpolation(x-ix,z-iz,height[ix][iz],height[ix+1][iz],height[ix+1][iz+1],height[ix][iz+1])*terrain_geometry_scale;
            
            temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=0;
            temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=0;
            temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=0;
            temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=0;

            if((rm>terrain_height_underwater_start)&&(rm<=terrain_height_underwater_end))
            {
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=255;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=0;
            }

            if((rm>terrain_height_sand_start)&&(rm<=terrain_height_sand_end))
            {
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=255;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=0;
            }

            if((rm>terrain_height_grass_start)&&(rm<=terrain_height_grass_end))
            {
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=255;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=0;
            }

            mv=bilinear_interpolation(x-ix,z-iz,normal[ix][iz][1],normal[ix+1][iz][1],normal[ix+1][iz+1][1],normal[ix][iz+1][1]);

            if((mv<terrain_slope_grass_start)&&(rm>terrain_height_sand_end))
            {
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=0;
            }

            if((mv<terrain_slope_rocks_start)&&(rm>terrain_height_rocks_start))
            {
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=0;
                temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=255;
            }

        }
    for(i=0;i<terrain_layerdef_map_texture_size;i++)
        for(j=0;j<terrain_layerdef_map_texture_size;j++)
        {
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0];
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1];
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2];
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3];
        }


    for(i=2;i<terrain_layerdef_map_texture_size-2;i++)
        for(j=2;j<terrain_layerdef_map_texture_size-2;j++)
        {
            int n1=0;
            int n2=0;
            int n3=0;
            int n4=0;
            for(k=-2;k<3;k++)
                for(l=-2;l<3;l++)
                    {
                            n1+=temp_layerdef_map_texture_pixels[((j+k)*terrain_layerdef_map_texture_size+i+l)*4+0];
                            n2+=temp_layerdef_map_texture_pixels[((j+k)*terrain_layerdef_map_texture_size+i+l)*4+1];
                            n3+=temp_layerdef_map_texture_pixels[((j+k)*terrain_layerdef_map_texture_size+i+l)*4+2];
                            n4+=temp_layerdef_map_texture_pixels[((j+k)*terrain_layerdef_map_texture_size+i+l)*4+3];
                    }
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+0]=(GLubyte)(n1/25);
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+1]=(GLubyte)(n2/25);
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+2]=(GLubyte)(n3/25);
            layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size+i)*4+3]=(GLubyte)(n4/25);
        }
    
    // putting the generated layerdef data to texture
    glGenTextures(1,&layerdef_texture);
    glBindTexture(GL_TEXTURE_2D, layerdef_texture);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, terrain_layerdef_map_texture_size,terrain_layerdef_map_texture_size,GL_RGBA, GL_UNSIGNED_BYTE, layerdef_map_texture_pixels);     
    free(temp_layerdef_map_texture_pixels);
    free(layerdef_map_texture_pixels);

    // building heightmap
    height_linear_array = new GLfloat [terrain_gridpoints*terrain_gridpoints*4];
    for(int i=0;i<terrain_gridpoints;i++)
        for(int j=0; j<terrain_gridpoints;j++)
        {
            height_linear_array[(i+j*terrain_gridpoints)*4+0]=normal[i][j][0];
            height_linear_array[(i+j*terrain_gridpoints)*4+1]=normal[i][j][1];
            height_linear_array[(i+j*terrain_gridpoints)*4+2]=normal[i][j][2];
            height_linear_array[(i+j*terrain_gridpoints)*4+3]=height[i][j];
        }
    glGenTextures(1,&heightmap_texture);
    glBindTexture(GL_TEXTURE_2D, heightmap_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, terrain_gridpoints, terrain_gridpoints,0, GL_RGBA, GL_FLOAT,height_linear_array);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0);

    free(height_linear_array);

    //building depthmap
    GLubyte* depthmap_pixels=(GLubyte *)malloc(terrain_depthmap_texture_size*terrain_depthmap_texture_size*4);
    for(i=0;i<terrain_depthmap_texture_size;i++)
        for(j=0;j<terrain_depthmap_texture_size;j++)
        {
            x=(float)(terrain_gridpoints)*((float)i/(float)terrain_depthmap_texture_size);
            z=(float)(terrain_gridpoints)*((float)j/(float)terrain_depthmap_texture_size);
            ix=(int)floor(x);
            iz=(int)floor(z);
            rm=bilinear_interpolation(x-ix,z-iz,height[ix][iz],height[ix+1][iz],height[ix+1][iz+1],height[ix][iz+1])*terrain_geometry_scale;
            
            if(rm>0)
            {
                depthmap_pixels[(j*terrain_depthmap_texture_size+i)*4+0]=0;
                depthmap_pixels[(j*terrain_depthmap_texture_size+i)*4+1]=0;
                depthmap_pixels[(j*terrain_depthmap_texture_size+i)*4+2]=0;
            }
            else
            {
                float no=(1.0f*255.0f*(rm/(terrain_minheight*terrain_geometry_scale)))-1.0f;
                if(no>255) no=255;
                if(no<0) no=0;
                depthmap_pixels[(j*terrain_depthmap_texture_size+i)*4+0]=(GLubyte)no;
                
                no=(6.0f*255.0f*(rm/(terrain_minheight*terrain_geometry_scale)))-40.0f;
                if(no>255) no=255;
                if(no<0) no=0;
                depthmap_pixels[(j*terrain_depthmap_texture_size+i)*4+1]=(GLubyte)no;

                no=(100.0f*255.0f*(rm/(terrain_minheight*terrain_geometry_scale)))-300.0f;
                if(no>255) no=255;
                if(no<0) no=0;
                depthmap_pixels[(j*terrain_depthmap_texture_size+i)*4+2]=(GLubyte)no;
            }
            depthmap_pixels[(j*terrain_depthmap_texture_size+i)*4+3]=0;
        }
    glGenTextures(1,&depthmap_texture);
    glBindTexture(GL_TEXTURE_2D, depthmap_texture);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, terrain_depthmap_texture_size,terrain_depthmap_texture_size,GL_RGBA, GL_UNSIGNED_BYTE, depthmap_pixels);     
    free(depthmap_pixels);
    fprintf(stderr,"\nTerrain related textures created");

    // creating terrain vertex buffer
    patches_rawdata = new float [terrain_numpatches_1d*terrain_numpatches_1d*4];
    for(int i=0;i<terrain_numpatches_1d;i++)
        for(int j=0;j<terrain_numpatches_1d;j++)
        {
            patches_rawdata[(i+j*terrain_numpatches_1d)*4+0]=i*terrain_geometry_scale*terrain_gridpoints/terrain_numpatches_1d;
            patches_rawdata[(i+j*terrain_numpatches_1d)*4+1]=j*terrain_geometry_scale*terrain_gridpoints/terrain_numpatches_1d;
            patches_rawdata[(i+j*terrain_numpatches_1d)*4+2]=terrain_geometry_scale*terrain_gridpoints/terrain_numpatches_1d;
            patches_rawdata[(i+j*terrain_numpatches_1d)*4+3]=terrain_geometry_scale*terrain_gridpoints/terrain_numpatches_1d;
        }
    glGenBuffers(1,&grid_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, 4*terrain_numpatches_1d*terrain_numpatches_1d*sizeof(float),patches_rawdata, GL_STATIC_DRAW) ;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free (patches_rawdata);
    fprintf(stderr,"\nGrid VBO created");

    GLuint a_PatchParams = glGetAttribLocation(terrain_program, "a_PatchParams");
    glGenVertexArrays(1, &grid_vao);
    glBindVertexArray(grid_vao);
    glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);

    glEnableVertexAttribArray(a_PatchParams);
    glVertexAttribPointer(a_PatchParams, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);

    glBindVertexArray(0);
    //fprintf(stderr,"\n %i %i %i",a_PatchParams,grid_vao,grid_vbo );
    fprintf(stderr,"\nGrid VAO created");
    checkError ("VAO");
}

void CTerrain::CreateSky()
{
    float *sky_vertexdata;
    int floatnum;
    int i;
    sky_vertexdata = new float [sky_gridpoints*(sky_gridpoints+2)*2*6];
    
    float sky_radius = 4000.0f;
    for(int j=0;j<sky_gridpoints;j++)
    {
        
        i=0;
        floatnum=(j*(sky_gridpoints+2)*2)*6;
        sky_vertexdata[floatnum+0]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*cos(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)j/(float)sky_gridpoints);
        sky_vertexdata[floatnum+1]=sky_radius*sin(-0.5f*PI+PI*(float)(j)/(float)sky_gridpoints);
        sky_vertexdata[floatnum+2]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*sin(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)j/(float)sky_gridpoints);
        sky_vertexdata[floatnum+3]=1;
        sky_vertexdata[floatnum+4]=(sky_texture_angle+(float)i/(float)sky_gridpoints);
        sky_vertexdata[floatnum+5]=2.0f-2.0f*(float)j/(float)sky_gridpoints;
        floatnum+=6;
        for(i=0;i<sky_gridpoints+1;i++)
        {
            sky_vertexdata[floatnum+0]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*cos(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)j/(float)sky_gridpoints);
            sky_vertexdata[floatnum+1]=sky_radius*sin(-0.5f*PI+PI*(float)(j)/(float)sky_gridpoints);
            sky_vertexdata[floatnum+2]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*sin(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)j/(float)sky_gridpoints);
            sky_vertexdata[floatnum+3]=1;
            sky_vertexdata[floatnum+4]=(sky_texture_angle+(float)i/(float)sky_gridpoints);
            sky_vertexdata[floatnum+5]=2.0f-2.0f*(float)j/(float)sky_gridpoints;
            floatnum+=6;
            sky_vertexdata[floatnum+0]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*cos(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)(j+1)/(float)sky_gridpoints);
            sky_vertexdata[floatnum+1]=sky_radius*sin(-0.5f*PI+PI*(float)(j+1)/(float)sky_gridpoints);
            sky_vertexdata[floatnum+2]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*sin(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)(j+1)/(float)sky_gridpoints);
            sky_vertexdata[floatnum+3]=1;
            sky_vertexdata[floatnum+4]=(sky_texture_angle+(float)i/(float)sky_gridpoints);
            sky_vertexdata[floatnum+5]=2.0f-2.0f*(float)(j+1)/(float)sky_gridpoints;
            floatnum+=6;
        }
        i=sky_gridpoints;
        sky_vertexdata[floatnum+0]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*cos(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)(j+1)/(float)sky_gridpoints);
        sky_vertexdata[floatnum+1]=sky_radius*sin(-0.5f*PI+PI*(float)(j+1)/(float)sky_gridpoints);
        sky_vertexdata[floatnum+2]=terrain_gridpoints*terrain_geometry_scale*0.5f+sky_radius*sin(2.0f*PI*(float)i/(float)sky_gridpoints)*cos(-0.5f*PI+PI*(float)(j+1)/(float)sky_gridpoints);
        sky_vertexdata[floatnum+3]=1;
        sky_vertexdata[floatnum+4]=(sky_texture_angle+(float)i/(float)sky_gridpoints);
        sky_vertexdata[floatnum+5]=2.0f-2.0f*(float)(j+1)/(float)sky_gridpoints;
        floatnum+=6;
    }
    
    glGenBuffers(1,&sky_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sky_vbo);
    glBufferData(GL_ARRAY_BUFFER, sky_gridpoints*(sky_gridpoints+2)*2*6*sizeof(float),sky_vertexdata, GL_STATIC_DRAW) ;
    glBindBuffer(GL_ARRAY_BUFFER, 0);   
    free (sky_vertexdata);
    sky_numvertices = sky_gridpoints*(sky_gridpoints+2)*2;
    fprintf(stderr,"\nSky VBO created");

    GLuint a_Position = glGetAttribLocation(sky_program, "a_Position");
    GLuint a_Texcoord = glGetAttribLocation(sky_program, "a_Texcoord");
    glGenVertexArrays(1, &sky_vao);
    glBindVertexArray(sky_vao);
    glBindBuffer(GL_ARRAY_BUFFER, sky_vbo);

    glEnableVertexAttribArray(a_Position);
    glVertexAttribPointer(a_Position, 4, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);

    glEnableVertexAttribArray(a_Texcoord);
    glVertexAttribPointer(a_Texcoord, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)(4*sizeof(float)));

    glBindVertexArray(0);
    //fprintf(stderr,"\n %i %i %i %i",a_Position,a_Texcoord,sky_vao,sky_vbo );
    fprintf(stderr,"\nSky VAO created");
    checkError ("VAO");
}

void CTerrain::CreateQuad()
{
    float quad_vertexdata[4*6];

    quad_vertexdata[0] = 0;
    quad_vertexdata[1] = 0;
    quad_vertexdata[2] = 0;
    quad_vertexdata[3] = 1.0f;
    quad_vertexdata[4] = 0;
    quad_vertexdata[5] = 0;

    quad_vertexdata[6] = 1.0f;
    quad_vertexdata[7] = 0;
    quad_vertexdata[8] = 0;
    quad_vertexdata[9] = 1.0f;
    quad_vertexdata[10] = 1.0f;
    quad_vertexdata[11] = 0;

    quad_vertexdata[12] = 0;
    quad_vertexdata[13] = 1.0f;
    quad_vertexdata[14] = 0;
    quad_vertexdata[15] = 1.0f;
    quad_vertexdata[16] = 0;
    quad_vertexdata[17] = 1.0f;

    quad_vertexdata[18] = 1.0f;
    quad_vertexdata[19] = 1.0f;
    quad_vertexdata[20] = 0;
    quad_vertexdata[21] = 1.0f;
    quad_vertexdata[22] = 1.0f;
    quad_vertexdata[23] = 1.0f;

    glGenBuffers(1,&quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, 24*sizeof(float),quad_vertexdata, GL_STATIC_DRAW) ;
    glBindBuffer(GL_ARRAY_BUFFER, 0);   

    fprintf(stderr,"\nQuad VBO created");

    GLuint a_Position = glGetAttribLocation(sky_program, "a_Position");
    GLuint a_Texcoord = glGetAttribLocation(sky_program, "a_Texcoord");
    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

    glEnableVertexAttribArray(a_Position);
    glVertexAttribPointer(a_Position, 4, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0);

    glEnableVertexAttribArray(a_Texcoord);
    glVertexAttribPointer(a_Texcoord, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)(4*sizeof(float)));

    glBindVertexArray(0);
    //fprintf(stderr,"\n %i %i %i %i",a_Position,a_Texcoord,quad_vao,quad_vbo );
    fprintf(stderr,"\nQuad VAO created");
    checkError ("VAO");
}

void CTerrain::LoadTextures()
{
    // loading textures from files
    LoadTexture("textures/rock_bump6.dds",&rock_bump_texture);
    LoadTexture("textures/terrain_rock4.dds",&rock_diffuse_texture);
    LoadTexture("textures/sand_diffuse.dds",&sand_diffuse_texture);
    LoadTexture("textures/rock_bump4.dds",&sand_bump_texture);
    LoadTexture("textures/terrain_grass.dds",&grass_diffuse_texture);
    LoadTexture("textures/terrain_slope.dds",&slope_diffuse_texture);
    LoadTexture("textures/lichen1_normal.dds",&sand_microbump_texture);
    LoadTexture("textures/rock_bump4.dds",&rock_microbump_texture);
    LoadTexture("textures/water_bump.dds",&water_bump_texture);
    LoadTexture("textures/sky.dds",&sky_texture);

    // creating textures for dynamic water surface
    glGenTextures(2,water_normal_textures);
    glGenTextures(2,water_displacement_textures);
    fprintf(stderr,"\nTextures loaded");  
}

void CTerrain::LoadShaders()
{
    quad_program = LoadProgram("shaders/quad.glsl");
    sky_program = LoadProgram("shaders/sky.glsl");
    terrain_program = LoadProgram("shaders/terrain.glsl");
    terrain_shadow_program = LoadProgram("shaders/terrain_shadow.glsl");
    water_program = LoadProgram("shaders/water.glsl");
}

void checkError(const char *functionName)
{
   GLenum error;
   while (( error = glGetError() ) != GL_NO_ERROR) {
      fprintf (stderr, "\nGL error 0x%X detected in %s", error, functionName);
   }
}

