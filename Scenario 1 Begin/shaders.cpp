#include "stdafx.h"
#include <vector>

#pragma warning(disable:4996)

GLuint compile_shader(char *text, GLenum type) //returns shader handle or 0 if error
{
    GLint length, compiled;

    GLuint shader;
    GLsizei maxlength=65535;
    static std::vector<char> infolog_buffer(65535);
    char* infolog = &infolog_buffer[0];
    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar **)&text, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        fprintf(stderr,"\nshader [%i] compilation error",type);
        fprintf(stderr,"\n...\n%s\n...\n",text);
        glGetShaderInfoLog(shader, maxlength, &length, infolog);
        fprintf(stderr,"\ninfolog: %s",infolog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint LoadProgram(const char* filename) //returns program object handle or 0 if error
{
    FILE *f;
    static std::vector<char> text_buffer(65535);
    char* text = &text_buffer[0];
    static std::vector<char> tmp_text_buffer(65535);
    char* tmp_text = &tmp_text_buffer[0];
    char *tmp_pos_start; 
    char *tmp_pos_end; 
    GLint length;

    GLuint result = 0;
    GLenum program;
    GLsizei maxlength=65535;
    static std::vector<char> infolog_buffer(65535);
    char* infolog = &infolog_buffer[0];
    GLint compiled;

    fprintf(stderr,"\nLoading & compiling shader [%s]: ",filename);

    program = glCreateProgram();

    fopen_s(&f, filename,"rb");
    if(f==NULL)
    {
        fprintf(stderr,"\ncould not open [%s]\n",filename);
        return 0;
    } 
    else
    {
        //fprintf(stdout, "\n - %s opened successfully",filename);
    }
    length=fread(text,1,65535,f);
    fclose(f);
    text[length]=0;

    // looking for vertex shader
    tmp_pos_start = strstr(text,"<<<VSTEXT>>>");
    if(tmp_pos_start!=NULL)
    {
        tmp_pos_start+=12;
        tmp_pos_end = strstr(tmp_pos_start,"<<<");  
        if (tmp_pos_end==NULL) tmp_pos_end = text + length;
        strncpy(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
        tmp_text[tmp_pos_end-tmp_pos_start]=0;
        result = compile_shader(tmp_text,GL_VERTEX_SHADER);
        if(result)
        {
            glAttachShader(program,result);
            glDeleteShader(result);
        }
    }

    // looking for tessellation control shader
    tmp_pos_start = strstr(text,"<<<TCTEXT>>>");
    if(tmp_pos_start!=NULL)
    {
        tmp_pos_start+=12;
        tmp_pos_end = strstr(tmp_pos_start,"<<<");  
        if (tmp_pos_end==NULL) tmp_pos_end = text + length;
        strncpy(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
        tmp_text[tmp_pos_end-tmp_pos_start]=0;
        result = compile_shader(tmp_text,GL_TESS_CONTROL_SHADER);
        if(result)
        {
            glAttachShader(program,result);
            glDeleteShader(result);
        }
    }
    
    // looking for tessellation evaluation shader
    tmp_pos_start = strstr(text,"<<<TETEXT>>>");
    if(tmp_pos_start!=NULL)
    {
        tmp_pos_start+=12;
        tmp_pos_end = strstr(tmp_pos_start,"<<<");  
        if (tmp_pos_end==NULL) tmp_pos_end = text + length;
        strncpy(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
        tmp_text[tmp_pos_end-tmp_pos_start]=0;
        result = compile_shader(tmp_text,GL_TESS_EVALUATION_SHADER);
        if(result)
        {
            glAttachShader(program,result);
            glDeleteShader(result);
        }
    }

    // looking for geometry shader
    tmp_pos_start = strstr(text,"<<<GSTEXT>>>");
    if(tmp_pos_start!=NULL)
    {
        tmp_pos_start+=12;
        tmp_pos_end = strstr(tmp_pos_start,"<<<");  
        if (tmp_pos_end==NULL) tmp_pos_end = text + length;
        strncpy(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
        tmp_text[tmp_pos_end-tmp_pos_start]=0;
        result = compile_shader(tmp_text,GL_GEOMETRY_SHADER);
        if(result)
        {
            glAttachShader(program,result);
            glDeleteShader(result);
        }
    }

    // looking for fragment shader
    tmp_pos_start = strstr(text,"<<<FSTEXT>>>");
    if(tmp_pos_start!=NULL)
    {
        tmp_pos_start+=12;
        tmp_pos_end = strstr(tmp_pos_start,"<<<");  
        if (tmp_pos_end==NULL) tmp_pos_end = text + length;
        strncpy(tmp_text,tmp_pos_start,tmp_pos_end-tmp_pos_start);
        tmp_text[tmp_pos_end-tmp_pos_start]=0;
        result = compile_shader(tmp_text,GL_FRAGMENT_SHADER);
        if(result)
        {
            glAttachShader(program,result);
            glDeleteShader(result);
        }
    }

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &compiled);
    if (!compiled) {
        fprintf(stderr,"program link error\n");
        glGetProgramInfoLog(program, maxlength, &length, infolog);
        fprintf(stderr,"\ninfolog: %s\n",infolog);
        return 0;
    }
    fprintf(stderr, "Ok");
    return program;
}
