
#include "stdafx.h"

#ifndef __IMAGE_H__
#define __IMAGE_H__


typedef struct {
  GLsizei  width;
  GLsizei  height;
  GLint    components;
  GLenum   format;
  GLint	   numMipmaps;
  GLubyte *pixels;  
} gliGenericImage;



int LoadTexture(char *, GLuint *);


#endif /* !__IMAGE_H__! */
