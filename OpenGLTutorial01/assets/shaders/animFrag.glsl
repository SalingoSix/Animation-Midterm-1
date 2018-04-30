#version 400

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 fTangent;		// For bump (or normal) mapping
in vec3 fBitangent;

out vec4 FragColor;

uniform int redOrBlue;

void main()
{	
	if (redOrBlue == 0)
	{	//Drawing the red man
		FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else
	{	//Drawing a blue man
		FragColor = vec4(0.0, 0.75, 1.0, 1.0);
	}
}