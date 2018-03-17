<<<VSTEXT>>>
#version 420 core

in vec4 a_Position;
in vec2 a_Texcoord;

uniform vec4 g_QuadParams;

out vec2 texcoord;

void main()
{
    gl_Position = vec4(g_QuadParams.zw * a_Position.xy + g_QuadParams.xy,0.0,1.0);
    texcoord = a_Texcoord.xy;
}

<<<FSTEXT>>>
#version 420 core

uniform sampler2D g_QuadTexture;

in vec2 texcoord;

out vec4 color;

void main()
{
	color.rgb = texture(g_QuadTexture,texcoord).rgb;
	color.a = 1;
}
