// IslandGL4.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "terrain.h"
#include "math_code.h"
#include <windows.h> // for QueryPerformanceFrequency/QueryPerformanceCounter
#include "nvToolsExt.h"

#ifndef PI
#define PI 3.14159265358979323846f
#endif

CTerrain g_Terrain;

		// flyby points
float EyePoints[6][3]=  {{365.0f,  6.0f, 166.0f},
						{478.0f, 15.0f, 248.0f},
						{430.0f,  6.0f, 249.0f},
						{513.0f, 10.0f, 277.0f},
						{303.0f,  6.0f, 449.0f},
						{20.0f,  12.0f, 477.0f}};

float LookAtPoints[6][3]={{330.0f,-11.0f, 259.0f},
						{388.0f,-16.0f, 278.0f},
						{357.0f,-59.0f, 278.0f},
						{438.0f,-12.0f, 289.0f},
						{319.0f,-20.0f, 432.0f},
						{90.0f,  -7.0f, 408.0f}};

void dumpInfo(void)
{
   printf ("\nVendor: %s", glGetString (GL_VENDOR));
   printf ("\nRenderer: %s", glGetString (GL_RENDERER));
   printf ("\nVersion: %s", glGetString (GL_VERSION));
   printf ("\nGLSL: %s", glGetString (GL_SHADING_LANGUAGE_VERSION));
   checkError ("dumpInfo");
}

void display(void)
{
	static LARGE_INTEGER freq, counter, old_counter;

	float viewpoints_slide_speed_factor=0.01f;
	float viewpoints_slide_speed_damp=0.97f;
	float scaled_time=(1.0f+g_Terrain.total_time/15.0f);
	
	float predicted_camera_position[3];
	float dh;

	g_Terrain.Render();

	// timing
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);

	g_Terrain.delta_time = (float)(((double)(counter.QuadPart) - (double)(old_counter.QuadPart))/(double)freq.QuadPart);
	g_Terrain.total_time += g_Terrain.delta_time;
	if(g_Terrain.total_time>=36000.0f) g_Terrain.total_time=0;
	old_counter = counter;
	//fprintf(stderr,"\n%3.5f %3.5f",g_Terrain.delta_time, g_Terrain.total_time);

    nvtxRangePushA("Water Update");
	// updating FFT 
	g_Terrain.UpdateFFTPhases();
	g_Terrain.PerformFFTforHeights();
	g_Terrain.PerformFFTforChop();
	g_Terrain.ExtractNormals();
	g_Terrain.UpdateFFTTextures();
	nvtxRangePop();

	// handling camera flyby if needed
	
	g_Terrain.ViewPointIndex = (int)(scaled_time)%6;

	if(g_Terrain.FlyByEnabled)
	{
		g_Terrain.CameraSpeed[0]+=(EyePoints[g_Terrain.ViewPointIndex][0]-g_Terrain.CameraPosition[0])*viewpoints_slide_speed_factor;
		g_Terrain.CameraSpeed[1]+=(EyePoints[g_Terrain.ViewPointIndex][1]-g_Terrain.CameraPosition[1])*viewpoints_slide_speed_factor;
		g_Terrain.CameraSpeed[2]+=(EyePoints[g_Terrain.ViewPointIndex][2]-g_Terrain.CameraPosition[2])*viewpoints_slide_speed_factor;

		g_Terrain.LookAtSpeed[0]+=(LookAtPoints[g_Terrain.ViewPointIndex][0]-g_Terrain.LookAtPosition[0])*viewpoints_slide_speed_factor;
		g_Terrain.LookAtSpeed[1]+=(LookAtPoints[g_Terrain.ViewPointIndex][1]-g_Terrain.LookAtPosition[1])*viewpoints_slide_speed_factor;
		g_Terrain.LookAtSpeed[2]+=(LookAtPoints[g_Terrain.ViewPointIndex][2]-g_Terrain.LookAtPosition[2])*viewpoints_slide_speed_factor;

		predicted_camera_position[0]=(g_Terrain.CameraPosition[0]+g_Terrain.CameraSpeed[0]*g_Terrain.delta_time*15.0f)/terrain_geometry_scale;
		predicted_camera_position[1]=(g_Terrain.CameraPosition[1]+g_Terrain.CameraSpeed[1]*g_Terrain.delta_time*15.0f)/terrain_geometry_scale;
		predicted_camera_position[2]=(g_Terrain.CameraPosition[2]+g_Terrain.CameraSpeed[2]*g_Terrain.delta_time*15.0f)/terrain_geometry_scale;

		dh=predicted_camera_position[1]-g_Terrain.height[((int)predicted_camera_position[0])%terrain_gridpoints]
													   [((int)(terrain_gridpoints-predicted_camera_position[2]))%terrain_gridpoints]-3.0f;
		if(dh<0)
		{
			g_Terrain.CameraSpeed[1]-=dh;
		}
		
		dh=predicted_camera_position[1]-3.0f;

		if(dh<0)
		{
			g_Terrain.CameraSpeed[1]-=dh;
		}

		g_Terrain.CameraSpeed[0]*=viewpoints_slide_speed_damp;
		g_Terrain.CameraSpeed[1]*=viewpoints_slide_speed_damp;
		g_Terrain.CameraSpeed[2]*=viewpoints_slide_speed_damp;

		g_Terrain.LookAtSpeed[0]*=viewpoints_slide_speed_damp;
		g_Terrain.LookAtSpeed[1]*=viewpoints_slide_speed_damp;
		g_Terrain.LookAtSpeed[2]*=viewpoints_slide_speed_damp;

		g_Terrain.CameraPosition[0]+=g_Terrain.CameraSpeed[0]*g_Terrain.delta_time;
		g_Terrain.CameraPosition[1]+=g_Terrain.CameraSpeed[1]*g_Terrain.delta_time;
		g_Terrain.CameraPosition[2]+=g_Terrain.CameraSpeed[2]*g_Terrain.delta_time;
		
		g_Terrain.LookAtPosition[0]+=g_Terrain.LookAtSpeed[0]*g_Terrain.delta_time;
		g_Terrain.LookAtPosition[1]+=g_Terrain.LookAtSpeed[1]*g_Terrain.delta_time;
		g_Terrain.LookAtPosition[2]+=g_Terrain.LookAtSpeed[2]*g_Terrain.delta_time;

		/*
		if(g_EyePoint.x<0)g_EyePoint.x=0;
		if(g_EyePoint.y<0)g_EyePoint.y=0;
		if(g_EyePoint.z<0)g_EyePoint.z=0;

		if(g_EyePoint.x>terrain_gridpoints*terrain_geometry_scale)g_EyePoint.x=terrain_gridpoints*terrain_geometry_scale;
		if(g_EyePoint.y>terrain_gridpoints*terrain_geometry_scale)g_EyePoint.y=terrain_gridpoints*terrain_geometry_scale;
		if(g_EyePoint.z>terrain_gridpoints*terrain_geometry_scale)g_EyePoint.z=terrain_gridpoints*terrain_geometry_scale;
		*/

	}
	else
	{
		g_Terrain.CameraSpeed[0]=0;
		g_Terrain.CameraSpeed[1]=0;
		g_Terrain.CameraSpeed[2]=0;
		g_Terrain.LookAtSpeed[0]=0;
		g_Terrain.LookAtSpeed[1]=0;
		g_Terrain.LookAtSpeed[2]=0;
	}

	g_Terrain.frame_number++;
}

void reshape (int w, int h)
{
	g_Terrain.ScreenWidth = w;
	g_Terrain.ScreenHeight = h;

	// need to recreate FBOs on reshape

	checkError ("reshape");
}

void keyboard(unsigned char key, int x, int y)
{
	float direction[3];
	float strafe[3];
	float up[3] = {0.0f,1.0f,0.0f};
	
	direction[0]=g_Terrain.LookAtPosition[0] - g_Terrain.CameraPosition[0];
	direction[1]=g_Terrain.LookAtPosition[1] - g_Terrain.CameraPosition[1];
	direction[2]=g_Terrain.LookAtPosition[2] - g_Terrain.CameraPosition[2];
	vec3Normalize(direction);
	vec3CrossProductNormalized(strafe,up,direction);

	switch (key) 
	{
		case 27:
			exit(0);
			break;
		case 'w':
			g_Terrain.CameraPosition[0]+=0.5f*direction[0];
			g_Terrain.CameraPosition[1]+=0.5f*direction[1];
			g_Terrain.CameraPosition[2]+=0.5f*direction[2];
			g_Terrain.LookAtPosition[0]+=0.5f*direction[0];
			g_Terrain.LookAtPosition[1]+=0.5f*direction[1];
			g_Terrain.LookAtPosition[2]+=0.5f*direction[2];
			break;
		case 's':
			g_Terrain.CameraPosition[0]-=0.5f*direction[0];
			g_Terrain.CameraPosition[1]-=0.5f*direction[1];
			g_Terrain.CameraPosition[2]-=0.5f*direction[2];
			g_Terrain.LookAtPosition[0]-=0.5f*direction[0];
			g_Terrain.LookAtPosition[1]-=0.5f*direction[1];
			g_Terrain.LookAtPosition[2]-=0.5f*direction[2];
			break;
		case 'a':
			g_Terrain.CameraPosition[0]+=0.5f*strafe[0];
			g_Terrain.CameraPosition[1]+=0.5f*strafe[1];
			g_Terrain.CameraPosition[2]+=0.5f*strafe[2];
			g_Terrain.LookAtPosition[0]+=0.5f*strafe[0];
			g_Terrain.LookAtPosition[1]+=0.5f*strafe[1];
			g_Terrain.LookAtPosition[2]+=0.5f*strafe[2];
			break;
		case 'd':
			g_Terrain.CameraPosition[0]-=0.5f*strafe[0];
			g_Terrain.CameraPosition[1]-=0.5f*strafe[1];
			g_Terrain.CameraPosition[2]-=0.5f*strafe[2];
			g_Terrain.LookAtPosition[0]-=0.5f*strafe[0];
			g_Terrain.LookAtPosition[1]-=0.5f*strafe[1];
			g_Terrain.LookAtPosition[2]-=0.5f*strafe[2];
			break;
		case '1':
			g_Terrain.CameraPosition[0]=EyePoints[0][0];
			g_Terrain.CameraPosition[1]=EyePoints[0][1];
			g_Terrain.CameraPosition[2]=EyePoints[0][2];
			g_Terrain.LookAtPosition[0]=LookAtPoints[0][0];
			g_Terrain.LookAtPosition[1]=LookAtPoints[0][1];
			g_Terrain.LookAtPosition[2]=LookAtPoints[0][2];
			break;
		case '2':
			g_Terrain.CameraPosition[0]=EyePoints[1][0];
			g_Terrain.CameraPosition[1]=EyePoints[1][1];
			g_Terrain.CameraPosition[2]=EyePoints[1][2];
			g_Terrain.LookAtPosition[0]=LookAtPoints[1][0];
			g_Terrain.LookAtPosition[1]=LookAtPoints[1][1];
			g_Terrain.LookAtPosition[2]=LookAtPoints[1][2];
			break;
		case '3':
			g_Terrain.CameraPosition[0]=EyePoints[2][0];
			g_Terrain.CameraPosition[1]=EyePoints[2][1];
			g_Terrain.CameraPosition[2]=EyePoints[2][2];
			g_Terrain.LookAtPosition[0]=LookAtPoints[2][0];
			g_Terrain.LookAtPosition[1]=LookAtPoints[2][1];
			g_Terrain.LookAtPosition[2]=LookAtPoints[2][2];
			break;
		case '4':
			g_Terrain.CameraPosition[0]=EyePoints[3][0];
			g_Terrain.CameraPosition[1]=EyePoints[3][1];
			g_Terrain.CameraPosition[2]=EyePoints[3][2];
			g_Terrain.LookAtPosition[0]=LookAtPoints[3][0];
			g_Terrain.LookAtPosition[1]=LookAtPoints[3][1];
			g_Terrain.LookAtPosition[2]=LookAtPoints[3][2];
			break;
		case '5':
			g_Terrain.CameraPosition[0]=EyePoints[4][0];
			g_Terrain.CameraPosition[1]=EyePoints[4][1];
			g_Terrain.CameraPosition[2]=EyePoints[4][2];
			g_Terrain.LookAtPosition[0]=LookAtPoints[4][0];
			g_Terrain.LookAtPosition[1]=LookAtPoints[4][1];
			g_Terrain.LookAtPosition[2]=LookAtPoints[4][2];
			break;
		case '6':
			g_Terrain.CameraPosition[0]=EyePoints[5][0];
			g_Terrain.CameraPosition[1]=EyePoints[5][1];
			g_Terrain.CameraPosition[2]=EyePoints[5][2];
			g_Terrain.LookAtPosition[0]=LookAtPoints[5][0];
			g_Terrain.LookAtPosition[1]=LookAtPoints[5][1];
			g_Terrain.LookAtPosition[2]=LookAtPoints[5][2];
			break;
		case ' ':
			g_Terrain.FlyByEnabled = !g_Terrain.FlyByEnabled;
			break;

   }
}

static void mousefunc(int button, int state, int x, int y)
{
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
	{
		g_Terrain.MouseX=x;
		g_Terrain.MouseY=y;
	}
}

static void mousemotionfunc(int x, int y)
{
	float initial_direction[4]={1.0f,0.0f,0.0f,1.0f};
	float direction[4];
	float r1[4][4],r2[4][4],rotation[4][4];
	//if(((x!=player.mouse_x)||(y!=player.mouse_y)))
	if(((x!=g_Terrain.ScreenWidth/2)||(y!=g_Terrain.ScreenHeight/2)))
	{
		
		g_Terrain.MouseDX=(float)x-(float)g_Terrain.MouseX;
		g_Terrain.MouseDY=(float)y-(float)g_Terrain.MouseY;

		g_Terrain.MouseX=x;
		g_Terrain.MouseY=y;
		//glutWarpPointer(g_Terrain.ScreenWidth/2,g_Terrain.ScreenHeight/2);
	}else
	{
		g_Terrain.MouseX=x;
		g_Terrain.MouseY=y;
	}

	g_Terrain.Alpha+=g_Terrain.MouseDX*0.002f;
	g_Terrain.Beta+=g_Terrain.MouseDY*0.002f;
	if(g_Terrain.Beta>PI*0.49f)g_Terrain.Beta = PI*0.49f;
	if(g_Terrain.Beta<-PI*0.49f)g_Terrain.Beta = -PI*0.49f;
	if(g_Terrain.FlyByEnabled==false)
	{
		mat4CreateRotation(r1,g_Terrain.Alpha,'y');
		mat4CreateRotation(r2,g_Terrain.Beta,'z');
		mat4Mat4Mul(rotation,r1,r2);
		vec4Mat4Mul(direction,initial_direction,rotation);
		g_Terrain.LookAtPosition[0]=g_Terrain.CameraPosition[0]+direction[0];
		g_Terrain.LookAtPosition[1]=g_Terrain.CameraPosition[1]+direction[1];
		g_Terrain.LookAtPosition[2]=g_Terrain.CameraPosition[2]+direction[2];
	}
}


int main(int argc, char** argv)
{
	// initializing rendering related constants
	g_Terrain.ScreenWidth = 1280;
	g_Terrain.ScreenHeight = 720;

	// initializing freeglut
	glutInit(&argc, argv);
    //glutInitContextProfile(GLUT_CORE_PROFILE);
    
    //glutInitContextFlags (GLUT_FORWARD_COMPATIBLE);
	// overriding the context creation flags if needed
	/*
	if ((argc != 2) || (strcmp (argv[1], "classic") != 0)) 
	{
		glutInitContextProfile(GLUT_CORE_PROFILE);
		glutInitContextVersion (4, 2);
		glutInitContextFlags (GLUT_FORWARD_COMPATIBLE);
	}
	*/
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   
	glutInitWindowSize(g_Terrain.ScreenWidth,g_Terrain.ScreenHeight);
	glutCreateWindow("IslandGL");
	dumpInfo ();

	// initializing glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "\nGLEW Error: %s", glewGetErrorString(err));
	}
	fprintf(stdout, "\nUsing GLEW %s", glewGetString(GLEW_VERSION));

	fprintf(stdout, "\n\nInitializing:");

	// initializing terrain
	g_Terrain.Initialize();

	// initializing FFT
	g_Terrain.SeedInitialFFTData();
	g_Terrain.total_time = 0;
	g_Terrain.UpdateFFTPhases();

	// setting freeglut callbacks
	glutDisplayFunc(display); 
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(mousemotionfunc);
	glutMouseFunc(mousefunc);
	glutIdleFunc(display);
	
	// entering rendering loop
	fprintf(stdout, "\n\nEntering main loop:");
	glutMainLoop();
	return 0;
}

