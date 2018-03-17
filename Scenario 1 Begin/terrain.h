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
 
#define terrain_gridpoints					512
#define terrain_numpatches_1d				64
#define terrain_geometry_scale				1.0f
#define terrain_maxheight					30.0f 
#define terrain_minheight					-30.0f 
#define terrain_fractalfactor				0.68f;
#define terrain_fractalinitialvalue			100.0f
#define terrain_smoothfactor1				0.99f
#define terrain_smoothfactor2				0.10f
#define terrain_rockfactor					0.95f
#define terrain_smoothsteps					40

#define terrain_height_underwater_start		-100.0f
#define terrain_height_underwater_end		-8.0f
#define terrain_height_sand_start			-30.0f
#define terrain_height_sand_end				1.7f
#define terrain_height_grass_start			1.7f
#define terrain_height_grass_end			30.0f
#define terrain_height_rocks_start			-2.0f
#define terrain_height_trees_start			4.0f
#define terrain_height_trees_end			30.0f
#define terrain_slope_grass_start			0.96f
#define terrain_slope_rocks_start			0.85f

#define terrain_far_range terrain_gridpoints*terrain_geometry_scale

#define shadowmap_size									4096
#define water_normalmap_size							2048
#define terrain_layerdef_map_texture_size				1024
#define terrain_depthmap_texture_size			512

#define sky_gridpoints						10
#define sky_texture_angle					0.425f

#define main_buffer_size_multiplier			1.1f
#define reflection_buffer_size_multiplier   1.1f
#define refraction_buffer_size_multiplier   1.1f

#define scene_z_near						0.1f
#define scene_z_far							25000.0f
#define camera_fov							3.14f*0.5f

#ifdef BIG
#define fft_N 512
#define fft_Nm1 511
#else
#define fft_N 128
#define fft_Nm1 127
#endif
#define fft_water_tilesize 300.0f
#define fft_gravity 9.8f

#define fft_waves_amplitude 0.0000025f
#define fft_wind_direction 3.0f //(in radians)
#define fft_wind_speed 14.0f
#define fft_small_waves_damper 0.0f//0.001f


typedef struct
{
    GLuint		 fbo;					// name of an fbo.
    GLuint		 color_texture;			// name of a texture attached.
	GLuint		 depth_texture;			// name of a depth texture attached.
    GLuint		 depth_renderbuffer;    // name of depthtexture attached
	int          width;					// Width of the buffer
    int          height;				// Height of the buffer
} fbo_def_type;

// FFT related structs

typedef struct
{
    float		 re;
    float 		 im;
} complex_type;

typedef struct
{
	complex_type 	htilda0[fft_N][fft_N];
	complex_type 	htilda1[fft_N][fft_N];
	complex_type 	htilda[fft_N][fft_N];
	complex_type	h[fft_N][fft_N];

	float 			w[fft_N][fft_N];

	float 	    	tmp_re[fft_N];
	float 	    	tmp_im[fft_N];
	float 			normal[fft_N][fft_N][3];
	complex_type	wave_dx[fft_N][fft_N];
	complex_type	wave_dy[fft_N][fft_N];
} fft_type;


class CTerrain
{
	public:

		// creaion related methods
		void Initialize(void);
		void CreateTerrain(void);
		void CreateSky(void);
		void CreateQuad(void);
		void CreateFBOs(void);
		void LoadTextures(void);
		void LoadShaders(void);
		void DeInitialize(void);

		// rendering related methods
		void SetupNormalView(void);
		void SetupReflectionView(void);
		void SetupRefractionView(void);
		void SetupLightView(void);
		void RenderSky(void);
		void RenderTerrain(void);
		void RenderTerrainShadow(void);
		void RenderWater(void);
		void Render(void);

		// FFT related methods
		void SeedInitialFFTData(void);
		void UpdateFFTPhases(void);
		int  FFT2D(complex_type[fft_N][fft_N],int,int,int);
		void PerformFFTforHeights(void);
		void PerformFFTforChop(void);
		void ExtractNormals(void);
		void UpdateFFTTextures(void);

		// constants
		UINT ScreenWidth;
		UINT ScreenHeight;
		UINT MultiSampleCount;
		UINT MultiSampleQuality;
		float DynamicTesselationFactor;
		float StaticTesselationFactor;
		float UseDynamicTessellation;
		float BackbufferWidth;
		float BackbufferHeight;


		// textures
		GLuint					rock_bump_texture;
		GLuint					rock_microbump_texture;
		GLuint					rock_diffuse_texture;
		GLuint					sand_bump_texture;
		GLuint					sand_microbump_texture;
		GLuint					sand_diffuse_texture;
		GLuint					grass_diffuse_texture;
		GLuint					slope_diffuse_texture;
		GLuint					water_bump_texture;
		GLuint					sky_texture;
		GLuint					heightmap_texture;
		GLuint					layerdef_texture;
		GLuint					depthmap_texture;
		GLuint					water_normal_textures[2];
		GLuint					water_displacement_textures[2];

		// frame buffer objects
		fbo_def_type			 reflection_fbo;
		fbo_def_type			 refraction_fbo;
		fbo_def_type			 shadowmap_fbo;
		fbo_def_type			 water_normalmap_fbo;
		fbo_def_type			 main_fbo;
		fbo_def_type			 main_resolved_fbo;

		// terrain data
		float						height[terrain_gridpoints+1][terrain_gridpoints+1];
		float						normal[terrain_gridpoints+1][terrain_gridpoints+1][3];
		float						tangent[terrain_gridpoints+1][terrain_gridpoints+1][3];
		float						binormal[terrain_gridpoints+1][terrain_gridpoints+1][3];

		// vertex buffer objects
		GLuint						grid_vbo;
		GLuint						grid_vao;
		GLuint						sky_vbo;
		GLuint						sky_vao;
		GLuint						sky_numvertices;

		GLuint						quad_vbo;
		GLuint						quad_vao;

		// programs
		GLuint					quad_program;
		GLuint					sky_program;
		GLuint					terrain_program;
		GLuint					terrain_shadow_program;
		GLuint					water_program;

		// light related variables
		float					LightPosition[3];

		// camera reated variables
		float					CameraPosition[3];
		float					LookAtPosition[3];
		bool					FlyByEnabled;
		int						ViewPointIndex;
		float					CameraSpeed[3];
		float					LookAtSpeed[3];

		float					NormalViewMatrix[4][4];
		float					NormalProjMatrix[4][4];
		float					NormalViewProjMatrix[4][4];
		float					NormalViewProjMatrixInv[4][4];

		float					ShadowViewMatrix[4][4];
		float					ShadowProjMatrix[4][4];
		float					ShadowViewProjMatrix[4][4];
		float					ShadowViewProjMatrixInv[4][4];

		float					ReflectionViewMatrix[4][4];
		float					ReflectionProjMatrix[4][4];
		float					ReflectionViewProjMatrix[4][4];
		float					ReflectionViewProjMatrixInv[4][4];

		float					RefractionViewMatrix[4][4];
		float					RefractionProjMatrix[4][4];
		float					RefractionViewProjMatrix[4][4];
		float					RefractionViewProjMatrixInv[4][4];

		// FFT related variables
		fft_type				FFTData;

		// counters
		float					total_time;
		float					delta_time;
		long					frame_number;

		// input
		int						MouseX,MouseY;
		float					MouseDX,MouseDY;
		float					Alpha;
		float					Beta;

};

float bilinear_interpolation(float fx, float fy, float a, float b, float c, float d);

void checkError(const char *);