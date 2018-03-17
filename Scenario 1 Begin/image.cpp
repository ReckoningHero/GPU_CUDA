#include "stdafx.h"
#include <ddraw.h>
#include "image.h"


gliGenericImage *ReadDDSFile(const char *filename, int * bufsize, int * numMipmaps) 
{
    gliGenericImage *genericImage;
    DDSURFACEDESC2 ddsd;
    char filecode[4];
    FILE *fp;
    int factor,uncompressed_components,format_found;
    /* try to open the file */
    fopen_s(&fp, filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr,"\nUnable to load texture file [%s]",filename);
        return NULL;
    }
    /* verify the type of file */
    fread(filecode, 1, 4, fp);
    if (strncmp(filecode, "DDS ", 4) != 0) 
    {
        fprintf(stderr,"\nThe file [%s] is not a dds file",filename);
        fclose(fp);
        return NULL;
    }
    /* get the surface desc */
    fread(&ddsd, sizeof(ddsd), 1, fp);
    if((ddsd.dwFlags & DDSD_PIXELFORMAT) != DDSD_PIXELFORMAT) 
    {
        fprintf(stderr,"\nDDS header pixelformat field contains no valid data,\nunable to load texture.");
        return 0;
    }
    if(((ddsd.dwFlags & DDSD_WIDTH) != DDSD_WIDTH) || ((ddsd.dwFlags & DDSD_HEIGHT) != DDSD_HEIGHT)) 
    {
        fprintf(stderr,"\nDDS header width/height fields contains no valid data,\nunable to load texture.");
        return 0;
    }
    if((ddsd.dwFlags & DDSD_MIPMAPCOUNT) != DDSD_MIPMAPCOUNT) 
    {
        fprintf(stderr,"\nDDS header mipmapcount field contains no valid data,\nassuming 0 mipmaps.");
        //return 0;
    }
    if(((ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC) != DDPF_FOURCC) && ((ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB) != DDPF_RGB)) 
    {
        fprintf(stderr,"\nDDS header pixelformat field contains no valid FOURCC and RGB data,\nunable to load texture.");
        return 0;
    }
    if(((ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC) == DDPF_FOURCC) && (ddsd.dwMipMapCount<=1) && ((ddsd.ddpfPixelFormat.dwFourCC==FOURCC_DXT1)||(ddsd.ddpfPixelFormat.dwFourCC==FOURCC_DXT3)||(ddsd.ddpfPixelFormat.dwFourCC==FOURCC_DXT5)) ) 
    {
        fprintf(stderr,"\nDDS header contains DXTx FOURCC code and no mipmaps,\nprogram does not support loading DXTx textures without mipmaps,\nunable to load texture.");
        return 0;
    }
    genericImage = (gliGenericImage*) malloc(sizeof(gliGenericImage));
    memset(genericImage,0,sizeof(gliGenericImage));
    factor=0;
    uncompressed_components=0;
    format_found=0;
    genericImage->width       = ddsd.dwWidth;
    genericImage->height      = ddsd.dwHeight;
    if((ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC) == DDPF_FOURCC) // if FOURCC code is valid
    {
        switch(ddsd.ddpfPixelFormat.dwFourCC)
        {
            case FOURCC_DXT1:
                if(ddsd.dwAlphaBitDepth!=0)
                {
                    genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                    genericImage->components = 4;
                }
                else 
                {
                    genericImage->format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                    genericImage->components = 3;
                }
                factor = 2;
                format_found=1;
            break;
            case FOURCC_DXT3:
                genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                genericImage->components = 4;
                factor = 4;
                format_found=1;
            break;
            case FOURCC_DXT5:
                genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                genericImage->components = 4;
                factor = 4;
                format_found=1;
            break;
            default:
            break;
        }
    }
    if(format_found==0)
    {
        switch (ddsd.ddpfPixelFormat.dwRGBBitCount)
        {
        case 24:
            genericImage->format = GL_RGB;
            genericImage->components = 3;
            format_found=2;
            break;
        case 32:
            genericImage->format = GL_RGBA;
            genericImage->components = 4;
            format_found=2;
        default:
            break;
        }
    }
    if(format_found==1)  // found compressed format
    {
      *bufsize = ddsd.dwMipMapCount > 1 ? ddsd.dwLinearSize * factor : ddsd.dwLinearSize;
    }
    if(format_found==2)
    {
      *bufsize = ddsd.dwMipMapCount > 1 ? ddsd.dwWidth*ddsd.dwHeight*genericImage->components+(ddsd.dwWidth*ddsd.dwHeight*genericImage->components)/2 : ddsd.dwWidth*ddsd.dwHeight*genericImage->components;
    }
    if(format_found==0)
    {
        fprintf(stderr,"\nUnsupported DDS format,\nthe program only supports DXTx and RGB/RGBA formats containing 8 bits per component");
        free(genericImage);
        return 0;
    }
    genericImage->pixels = (unsigned char*)malloc(*bufsize);
    fread(genericImage->pixels, 1, *bufsize, fp);
    fclose(fp);
    *numMipmaps = ddsd.dwMipMapCount;
    genericImage->numMipmaps=ddsd.dwMipMapCount;
    return genericImage;
}

int LoadTexture(char * filename, GLuint * texID)
{
    gliGenericImage *ggi = NULL;
    int bufsize,numMipmaps,blocksize,i;
    long offset;
    GLint size,width,height;
    unsigned char c1,c2,c3,c4;
    long n;
    fprintf(stderr,"\nLoading %s: ", filename);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    ggi = ReadDDSFile(filename, &bufsize, &numMipmaps);
    if(ggi==NULL)
    {
        fprintf(stderr,"The file [%s] hasn't been loaded successfully.",filename);
        return 0;
    }
    height = ggi->height;
    width = ggi->width;
    switch (ggi->format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            glGenTextures(1,texID);
            glBindTexture(GL_TEXTURE_2D, *texID);
            blocksize=8;
            offset = 0;
            height = ggi->height;
            width = ggi->width;
            for (i = 0; i < numMipmaps && (width || height); ++i)
            {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                size = ((width+3)/4)*((height+3)/4)*blocksize;
                glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 0, size, ggi->pixels+offset);
                offset += size;
                width >>= 1;
                height >>= 1;
            }
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            glGenTextures(1,texID);
            glBindTexture(GL_TEXTURE_2D, *texID);
            blocksize=8;
            offset = 0;
            height = ggi->height;
            width = ggi->width;
            for (i = 0; i < numMipmaps && (width || height); ++i)
            {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                size = ((width+3)/4)*((height+3)/4)*blocksize;
                glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 0, size, ggi->pixels + offset);
                offset += size;
                width >>= 1;
                height >>= 1;
            }
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            glGenTextures(1,texID);
            glBindTexture(GL_TEXTURE_2D, *texID);
            blocksize=16;
            offset = 0;
            height = ggi->height;
            width = ggi->width;
            for (i = 0; i < numMipmaps && (width || height); ++i)
            {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                size = ((width+3)/4)*((height+3)/4)*blocksize;
                glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 0, size, ggi->pixels + offset);
                offset += size;
                width >>= 1;
                height >>= 1;
            }
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            glGenTextures(1,texID);
            glBindTexture(GL_TEXTURE_2D, *texID);
            blocksize=16;
            offset = 0;
            height = ggi->height;
            width = ggi->width;
            for (i = 0; i < numMipmaps && (width || height); ++i)
                {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                size = ((width+3)/4)*((height+3)/4)*blocksize;
                glCompressedTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height, 
                    0, size, ggi->pixels + offset);
                offset += size;
                width >>= 1;
                height >>= 1;
            }
            break;
        case GL_RGB:
            glGenTextures(1,texID);
            glBindTexture(GL_TEXTURE_2D, *texID);
            offset = 0;
            height = ggi->height;
            width = ggi->width;
            if(numMipmaps<=1)
            {
                for(n=0;n<width*height*3;n+=3)
                {
                    c1=*(ggi->pixels+n); // switching R and B
                    c3=*(ggi->pixels+n+2);
                    *(ggi->pixels+n)=c3;
                    *(ggi->pixels+n+2)=c1;
                }
                gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, width,height,ggi->format, GL_UNSIGNED_BYTE, ggi->pixels);
            }else
            for (i = 0; i < numMipmaps && (width || height); ++i)
            {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                size = width*height*3;
                for(n=0;n<size;n+=3)
                {
                    c1=*(ggi->pixels+offset+n); // switching R and B
                    c3=*(ggi->pixels+offset+n+2);
                    *(ggi->pixels+offset+n)=c3;
                    *(ggi->pixels+offset+n+2)=c1;
                }
                glTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height,0, ggi->format, GL_UNSIGNED_BYTE, ggi->pixels + offset);
                offset += size;
                width >>= 1;
                height >>= 1;
            }
            break;
        case GL_RGBA:
            glGenTextures(1,texID);
            glBindTexture(GL_TEXTURE_2D, *texID);
            offset = 0;
            height = ggi->height;
            width = ggi->width;
            if(numMipmaps<=1)
            {
                for(n=0;n<width*height*4;n+=4)
                {
                    c1=*(ggi->pixels+n); // switching BGRA to RGBA
                    c2=*(ggi->pixels+n+1);
                    c3=*(ggi->pixels+n+2);
                    c4=*(ggi->pixels+n+3);
                    *(ggi->pixels+n)=c3;
                    *(ggi->pixels+n+1)=c2;
                    *(ggi->pixels+n+2)=c1;
                    *(ggi->pixels+n+3)=c4;
                }
                gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, width,height,ggi->format, GL_UNSIGNED_BYTE, ggi->pixels);
            }else
            for (i = 0; i < numMipmaps && (width || height); ++i)
            {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                size = width*height*4;
                for(n=0;n<size;n+=4)
                {
                    c1=*(ggi->pixels+offset+n); // switching BGRA to RGBA
                    c2=*(ggi->pixels+offset+n+1);
                    c3=*(ggi->pixels+offset+n+2);
                    c4=*(ggi->pixels+offset+n+3);
                    *(ggi->pixels+offset+n)=c3;
                    *(ggi->pixels+offset+n+1)=c2;
                    *(ggi->pixels+offset+n+2)=c1;
                    *(ggi->pixels+offset+n+3)=c4;
                }
                glTexImage2D(GL_TEXTURE_2D, i, ggi->format, width, height,0, ggi->format, GL_UNSIGNED_BYTE, ggi->pixels + offset);
                offset += size;
                width >>= 1;
                height >>= 1;
            }
            break;
    }
    free(ggi->pixels);
    free(ggi);
    fprintf(stderr," Loaded");
    return 1;
}
