
 /* FFTData.cpp
 *
 *  Created on: 02.05.2012
 *      Author: ttcheblokov
 */
#include "stdafx.h"
#include "terrain.h"
#include "math_code.h"
#include "fft.h"
#include <vector>

#ifndef PI
#define PI 3.14159265f
#endif

static float rand_table[][2] =
{
    {.00f, .5f},
    {.01f, .504f},
    {.02f, .508f},
    {.03f, .512f},
    {.06f, .5239f},
    {.08f, .5319f},
    {.11f, .5438f},
    {.13f, .5517f},
    {.16f, .5636f},
    {.18f, .5714f},
    {.21f, .5832f},
    {.23f, .5910f},
    {.26f, .6026f},
    {.28f, .6103f},
    {.31f, .6217f},
    {.34f, .6331f},
    {.36f, .6406f},
    {.39f, .6517f},
    {.42f, .6628f},
    {.44f, .6700f},
    {.47f, .6808f},
    {.50f, .6915f},
    {.53f, .7019f},
    {.56f, .7123f},
    {.59f, .7224f},
    {.62f, .7324f},
    {.65f, .7422f},
    {.68f, .7517f},
    {.71f, .7611f},
    {.74f, .7703f},
    {.78f, .7823f},
    {.81f, .7910f},
    {.85f, .8023f},
    {.88f, .8106f},
    {.92f, .8212f},
    {.96f, .8315f},
    {1.0f, .8413f},
    {1.04f, .8508f},
    {1.09f, .8621f},
    {1.13f, .8708f},
    {1.18f, .8810f},
    {1.23f, .8907f},
    {1.29f, .9015f},
    {1.35f, .9115f},
    {1.41f, .9207f},
    {1.48f, .9306f},
    {1.56f, .9406f},
    {1.65f, .9505f},
    {1.76f, .9608f},
    {1.89f, .9706f},
    {2.06f, .9803f},
    {2.33f, .9901f},
    {99.0f, 1.0f}
};

float randnormal(float mean, float stdev)
{
    int i = 0;
    float u = rand() / (float) RAND_MAX;
    float n;

    if (u >= 0.5)
    {
        while (u > rand_table[i][1])
        {
            i++;
        }
        n = rand_table[i-1][0];
    }
    else
    {
        u = 1 - u;
        while (u > rand_table[i][1])
        {
            i++;
        }
        n = 1 - rand_table[i-1][0];
    }
    return (mean + stdev * n);
}


float philipsSpectrumValue (float kx, float ky)
{
    //float l = fft_wind_speed * fft_wind_speed / fft_gravity;   // largest possible wave from constant wind of velocity $v$
    //float w = l * fft_small_waves_damper;      // damp out waves with very small length $w << l$

    float k2 = kx * kx + ky * ky;
    if(k2==0)return 0.0f;
    float k4 = k2*k2;
    float kdotw = (float) (kx * cos(fft_wind_direction) + ky * sin(fft_wind_direction));
    float kdotwhat = kdotw*kdotw/k2;
    float eterm = exp(-fft_gravity*fft_gravity / (k2*fft_wind_speed*fft_wind_speed*fft_wind_speed*fft_wind_speed)) / k4;

    float specresult = fft_waves_amplitude * eterm * kdotwhat * exp(-k2 * fft_water_tilesize * fft_small_waves_damper);
    if (kdotw < 0) specresult *=0.05f;  // filter out waves moving opposite to wind
  return specresult;
}


void CTerrain::SeedInitialFFTData(void)
{
    int i, j;
    float kx, ky;
    float multiplier, amplitude, theta;

    for (i = 0; i < fft_N; i++)
    {
        for (j = 0; j < fft_N; j++)
        {
            kx = (-(float)fft_N / 2.0f + (float)i) * (2.0f * PI / (float)fft_water_tilesize);
            ky = (-(float)fft_N / 2.0f + (float)j) * (2.0f * PI / (float)fft_water_tilesize);
                // set angular frequency $\omega = \sqrt{g k}$
            FFTData.w[i][j] = (float) sqrt(fft_gravity * sqrt(kx * kx + ky * ky));
                // set $\tilde{h}_0$
            if (kx == 0 && ky == 0) multiplier = 0;
            else multiplier = (float) (sqrt(philipsSpectrumValue(kx, ky)));
            amplitude = randnormal(0.0f, 1.0f);
            theta = rand() / (float) RAND_MAX * 2.0f * PI;
            FFTData.htilda0[i][j].re = (float) (multiplier * amplitude * sin(theta));
            FFTData.htilda0[i][j].im = (float) (multiplier * amplitude * cos(theta));
        }
    }
}

void CTerrain::UpdateFFTPhases(void)
{
    int i, j;
    float kx, ky;
    int mi, mj;                // indices corresponding to -K
    complex_type plus, minus;  // the terms $\tilde{h}_0(K) e^{i \omega(K) t}$ and $\tilde{h}_0^*(-K) e^{-i \omega(K) t}$,
                               //  respectively, which sum to $\tilde{h}$

    float t= total_time*2.0f;

    float s;
    float c;
    // compute $\tilde{h}$ -- since $\tilde{h}(K) = \tilde{h}^*(-K)$ (a Hermitian property),
      //  we need only compute half the matrix, i.e., the matrix for $i < N / 2$ instead of all $i <= N$.
    for (i = 0; i < fft_N/2 /*i <= fft_N*/; i++)
    {
        kx = (-fft_N / 2.0f + i) * (2.0f * PI / fft_water_tilesize);
        mi = fft_N - i;
        for (j = 0; j < fft_N /*j <= fft_N*/; j++)
        {
            ky = (-fft_N / 2.0f + j) * (2.0f * PI / fft_water_tilesize);
            mj = fft_N - j;
            // (one optimization might be to save these terms in a matrix, and precompute the sines and cosines for each
            //  $\omega$ for a given timestep $dt$ -- but then waves could only move by the precomputed time step)

            //FFTData.w[i][j]=0.0f;
            //t=0;
            s = sin(FFTData.w[i][j] * t);
            c = cos(FFTData.w[i][j] * t);
            plus.re = (float) (FFTData.htilda0[i][j].re * c - FFTData.htilda0[i][j].im * s);
            plus.im = (float) (FFTData.htilda0[i][j].re * s + FFTData.htilda0[i][j].im * c);
            minus.re = (float) (FFTData.htilda0[mi][mj].re * c - (-FFTData.htilda0[mi][mj].im) * (-s));
            minus.im = (float) (FFTData.htilda0[mi][mj].re * s + (-FFTData.htilda0[mi][mj].im) * c);
            // now sum the plus and minus waves to get the total wave amplitude h
            FFTData.htilda[i][j].re = plus.re + minus.re;
            FFTData.htilda[i][j].im = plus.im + minus.im;
        }
    }
}

static int Powerof2(int v, int *m, int *twopm)
{
    int nn = 1;
    int mm=0;
    if (v<=0) return 0;
    while(nn<v)
    {
        nn<<=1;
        ++mm;
    }
    *m = mm;
    *twopm = nn;
    return 1;
}



int FFT(int dir,int m,float *x,float *y)
{
   long nn,i,i1,j,k,i2,l,l1,l2;
   float c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   nn = 1;
   for (i=0;i<m;i++)
      nn *= 2;

   /* Do the bit reversal */
   i2 = nn >> 1;
   j = 0;
   for (i=0;i<nn-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0f;
   c2 = 0.0f;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0f;
      u2 = 0.0f;
      for (j=0;j<l1;j++) {
         for (i=j;i<nn;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1;
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0f - c1) / 2.0f);
      if (dir == 1)
         c2 = -c2;
      c1 = sqrt((1.0f + c1) / 2.0f);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<nn;i++) {
         x[i] /= (float)nn;
         y[i] /= (float)nn;
      }
   }
   return(true);
}

/*-------------------------------------------------------------------------
   Perform a 2D FFT inplace given a complex 2D array
   The direction dir, 1 for forward, -1 for reverse
   The size of the array (nx,ny)
   Return false if there are memory problems or
      the dimensions are not powers of 2
*/
int CTerrain::FFT2D(complex_type c[fft_N][fft_N],int nx,int ny,int dir)
{
   int i,j;
   int m,twopm;

   /* Transform the rows */
   if (!Powerof2(nx,&m,&twopm) || twopm != nx)
      return(false);
   for (j=0;j<ny;j++) {
      for (i=0;i<nx;i++) {
         FFTData.tmp_re[i] = c[i][j].re;
         FFTData.tmp_im[i] = c[i][j].im;
      }
      FFT(dir,m,FFTData.tmp_re,FFTData.tmp_im);
      for (i=0;i<nx;i++) {
         c[i][j].re = FFTData.tmp_re[i];
         c[i][j].im = FFTData.tmp_im[i];
      }
   }
   float a = -1;
   for (i=0;i<nx;i++) {
      for (j=0;j<ny;j++) {
          FFTData.tmp_re[j] = c[i][j].re;
          FFTData.tmp_im[j] = c[i][j].im;
      }
      FFT(dir,m,FFTData.tmp_re,FFTData.tmp_im);
      a = -a;
      for (j=0;j<ny;j++) {
          a=-a;
         c[i][j].re = FFTData.tmp_re[j]*a;
         c[i][j].im = FFTData.tmp_im[j]*a;
      }
   }
   return(true);
}


void CTerrain::PerformFFTforHeights(void)
{
    memcpy(FFTData.h,FFTData.htilda, fft_N*fft_N*sizeof(float)*2);
    FFT2D(FFTData.h,fft_N,fft_N,-1);
}

void CTerrain::PerformFFTforChop(void)
{
    float kx,ky;
    int i,j;
    float k;

    for (i = 0; i < fft_N; i++) {
            kx = (float) (i - fft_N/2);
      for (j = 0; j < fft_N; j++) {        // -i (n1,n2)/sqrt(n1^2+n2^2) * htilda(n1, n2)
                ky = (float) (j - fft_N/2);
        k = (float) sqrt(kx * kx + ky * ky+0.01);    // note k != 0
        FFTData.wave_dx[i][j].re =  FFTData.htilda[i][j].im * kx / k;
        FFTData.wave_dx[i][j].im =  -FFTData.htilda[i][j].re * kx / k;
        FFTData.wave_dy[i][j].re =  FFTData.htilda[i][j].im * ky / k;
        FFTData.wave_dy[i][j].im =  -FFTData.htilda[i][j].re * ky / k;
      }
    }
    FFT2D(FFTData.wave_dx,fft_N,fft_N,-1);
    FFT2D(FFTData.wave_dy,fft_N,fft_N,-1);
}

int nid(int i)
{
    return i>=0?i%fft_N:i+fft_N;
}

void CTerrain::ExtractNormals(void)
{
    int i,j;
    float ndx;
    float ndz;
    float dxdzscale = 0.6f;
    for(i=0;i<fft_N;i++)
    {
        for(j=0;j<fft_N;j++)
        {
            ndx = FFTData.h[nid(i+1)][j].re - FFTData.h[nid(i-1)][j].re;
            ndz = FFTData.h[i][nid(j+1)].re - FFTData.h[i][nid(j-1)].re;
            ndx *= dxdzscale;
            ndz *= dxdzscale;
            FFTData.normal[i][j][0] = -ndx/(2.0f*dxdzscale);
            FFTData.normal[i][j][2] = -ndz/(2.0f*dxdzscale);
            FFTData.normal[i][j][1] = 1.0f;
            vec3Normalize(FFTData.normal[i][j]);
        }
    }
}

void CTerrain::UpdateFFTTextures(void)
{
    static std::vector<GLfloat> texture_data_buffer(fft_N*fft_N*4);
    GLfloat* texture_data = &texture_data_buffer[0];

    for(int i=0;i<fft_N;i++)
    {
        for(int j=0;j<fft_N;j++)
        {
            texture_data[(j*fft_N+i)*4 + 0] = FFTData.normal[i][j][0];
            texture_data[(j*fft_N+i)*4 + 1] = FFTData.normal[i][j][1];
            texture_data[(j*fft_N+i)*4 + 2] = FFTData.normal[i][j][2];
            texture_data[(j*fft_N+i)*4 + 3] = 0;
        }
    }
    glBindTexture(GL_TEXTURE_2D, water_normal_textures[(frame_number)%2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, fft_N, fft_N,0, GL_RGBA, GL_FLOAT, texture_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    for(int i=0;i<fft_N;i++)
    {
        for(int j=0;j<fft_N;j++)
        {
            texture_data[(j*fft_N+i)*4 + 0] = FFTData.wave_dx[i][j].re;
            texture_data[(j*fft_N+i)*4 + 1] = FFTData.h[i][j].re;
            texture_data[(j*fft_N+i)*4 + 2] = FFTData.wave_dy[i][j].re;
            texture_data[(j*fft_N+i)*4 + 3] = 0;
        }
    }
    glBindTexture(GL_TEXTURE_2D, water_displacement_textures[(frame_number)%2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, fft_N, fft_N,0, GL_RGBA, GL_FLOAT, texture_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}
