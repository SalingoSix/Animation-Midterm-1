#version 430

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in vec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
const int MAXNUMBEROFBONES = 100;
uniform mat4 bones[MAXNUMBEROFBONES];
// Pass the acutal number you are using to control how often this loops
uniform int numBonesUsed;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 fTangent;		// For bump (or normal) mapping
out vec3 fBitangent;	// For bump (or normal) mapping

void main()
{
    //gl_Position = MVP * vec4(vPos, 0.0, 1.0);	
	vec4 vertPosition = vec4(aPos, 1.0f);
	
	mat4 matModel = model;	

	mat4 BoneTransform = bones[ int(aBoneIDs[0]) ] * aBoneWeights[0];
	BoneTransform += bones[ int(aBoneIDs[1]) ] * aBoneWeights[1];
	BoneTransform += bones[ int(aBoneIDs[2]) ] * aBoneWeights[2];
	BoneTransform += bones[ int(aBoneIDs[3]) ] * aBoneWeights[3];
	vertPosition = BoneTransform * vertPosition;
	
	mat4 matMVP = projection * view * model;		// m = p * v * m;
	
	// Final screen space position	
	gl_Position = matMVP * vertPosition;	
	
	// Inverse transform to keep ONLY rotation...
	mat4 matNormal = inverse( transpose(BoneTransform * model) );
	
	Normal = mat3(matNormal) * normalize(aNormal.xyz);
	fTangent = 	mat3(matNormal) * normalize(aTangent.xyz);
	fBitangent = 	mat3(matNormal) * normalize(aBitangent.xyz);
	
	FragPos = (model * vertPosition).xyz;
	
	TexCoords = aTexCoord;			// Sent to fragment shader

}		