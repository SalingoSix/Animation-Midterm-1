#include "cSkinnedMesh.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtx\matrix_decompose.hpp>

#include <glad\glad.h>
#include <sstream>

#include "cShaderProgram.h"

#include <SOIL2\SOIL2.h>

unsigned int SMTextureFromFile(const char* path, const std::string &directory, bool gamma = false);

glm::mat4 AIMatrixToGLMMatrix(const aiMatrix4x4& mat)
{
	return glm::mat4(mat.a1, mat.b1, mat.c1, mat.d1,
		mat.a2, mat.b2, mat.c2, mat.d2,
		mat.a3, mat.b3, mat.c3, mat.d3,
		mat.a4, mat.b4, mat.c4, mat.d4);
}

void cSkinnedMesh::sVertexBoneData::AddBoneData(unsigned int BoneID, float Weight)
{
	unsigned int size = sizeof(this->Ids) / sizeof(this->Ids[0]);
	for (unsigned int Index = 0; Index < size; Index++)
	{
		if (this->Weights[Index] == 0.0f)
		{
			this->Ids[Index] = (float)BoneID;
			this->Weights[Index] = Weight;
			return;
		}
	}
}

cSkinnedMesh::cSkinnedMesh(const std::string& filename)
{
	this->Scene = 0;

	this->NumBones = 0;
	this->NumVertices = 0;
	this->NumIndices = 0;
	this->NumTriangles = 0;

	this->Filename = filename;
	this->Name = filename;

	LoadMeshFromFile(filename);

	return;
}

cSkinnedMesh::~cSkinnedMesh()
{

}

bool cSkinnedMesh::LoadMeshFromFile(const std::string &filename)
{
	unsigned int Flags = aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_JoinIdenticalVertices;

	this->Scene = this->Importer.ReadFile(filename.c_str(), Flags);
	if (this->Scene)
	{
		this->Filename = filename;
		this->Name = filename;

		this->GlobalInverseTransformation = AIMatrixToGLMMatrix(Scene->mRootNode->mTransformation);
		this->GlobalInverseTransformation = glm::inverse(this->GlobalInverseTransformation);

		//this->Initialize();
	}
	this->directory = filename.substr(0, filename.find_last_of('/'));
	processNode(Scene->mRootNode, Scene);
	return true;
}

bool cSkinnedMesh::Initialize(int index)
{
	this->NumVertices = this->Scene->mMeshes[index]->mNumVertices;

	this->VecVertexBoneData.clear();
	this->VecVertexBoneData.resize(this->NumVertices);

	this->LoadBones(this->Scene->mMeshes[index], this->VecVertexBoneData);

	return true;
}

bool cSkinnedMesh::Initialize(void)
{
	this->NumVertices = this->Scene->mMeshes[0]->mNumVertices;
	for (int i = 0; i != this->Scene->mNumMeshes; i++)
	{
		printf("%d", this->Scene->mMeshes[i]->mNumBones);
	}

	// This is the vertex information for JUST the bone stuff
	this->VecVertexBoneData.resize(this->NumVertices);

	this->LoadBones(this->Scene->mMeshes[0], this->VecVertexBoneData);

	return true;
}

void cSkinnedMesh::LoadBones(const aiMesh* Mesh, std::vector<sVertexBoneData> &vertexBoneData)
{
	for (unsigned int boneIndex = 0; boneIndex != Mesh->mNumBones; boneIndex++)
	{
		unsigned int BoneIndex = 0;
		std::string BoneName(Mesh->mBones[boneIndex]->mName.data);

		std::map<std::string, unsigned int>::iterator it = this->MapBoneNameToBoneIndex.find(BoneName);
		if (it == this->MapBoneNameToBoneIndex.end())
		{
			BoneIndex = this->NumBones;
			this->NumBones++;
			sBoneInfo bi;
			this->VecBoneInfo.push_back(bi);

			this->VecBoneInfo[BoneIndex].BoneOffset = AIMatrixToGLMMatrix(Mesh->mBones[boneIndex]->mOffsetMatrix);
			this->MapBoneNameToBoneIndex[BoneName] = BoneIndex;
		}
		else
		{
			BoneIndex = it->second;
		}

		for (unsigned int WeightIndex = 0; WeightIndex != Mesh->mBones[boneIndex]->mNumWeights; WeightIndex++)
		{
			unsigned int VertexID = /*mMeshEntries[MeshIndex].BaseVertex +*/ Mesh->mBones[boneIndex]->mWeights[WeightIndex].mVertexId;
			float Weight = Mesh->mBones[boneIndex]->mWeights[WeightIndex].mWeight;
			vertexBoneData[VertexID].AddBoneData(BoneIndex, Weight);
		}
	}
	return;
}

float cSkinnedMesh::FindAnimationTotalTime(std::string animationName)
{
	std::map< std::string, const aiScene* >::iterator itAnimation = this->MapAnimationNameToScene.find(animationName);

	if (itAnimation == this->MapAnimationNameToScene.end())
	{
		return 0.0f;
	}
	return (float)itAnimation->second->mAnimations[0]->mDuration;
}

bool cSkinnedMesh::LoadMeshAnimation(const std::string &filename)
{
	unsigned int Flags = aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_JoinIdenticalVertices;

	Assimp::Importer* pImporter = new Assimp::Importer();
	const aiScene* pAniScene = pImporter->ReadFile(filename.c_str(), Flags);
	if (!pAniScene)
	{
		return false;
	}
	this->MapAnimationNameToScene[filename] = pAniScene;

	return true;
}

void cSkinnedMesh::BoneTransform(float TimeInSeconds,
	std::string animationName,		// Now we can pick the animation
	std::vector<glm::mat4> &FinalTransformation,
	std::vector<glm::mat4> &Globals,
	std::vector<glm::mat4> &Offsets)
{
	glm::mat4 Identity(1.0f);

	float TicksPerSecond = static_cast<float>(this->Scene->mAnimations[0]->mTicksPerSecond != 0 ?
		this->Scene->mAnimations[0]->mTicksPerSecond : 25.0);

	float TimeInTicks = TimeInSeconds * TicksPerSecond;
	float AnimationTime = fmod(TimeInTicks, (float)this->Scene->mAnimations[0]->mDuration);

	this->ReadNodeHierarchy(AnimationTime, animationName, this->Scene->mRootNode, Identity);

	FinalTransformation.resize(this->NumBones);
	Globals.resize(this->NumBones);
	Offsets.resize(this->NumBones);

	for (unsigned int BoneIndex = 0; BoneIndex < this->NumBones; BoneIndex++)
	{
		FinalTransformation[BoneIndex] = this->VecBoneInfo[BoneIndex].FinalTransformation;
		Globals[BoneIndex] = this->VecBoneInfo[BoneIndex].ObjectBoneTransformation;
		Offsets[BoneIndex] = this->VecBoneInfo[BoneIndex].BoneOffset;
	}
}

const aiNodeAnim* cSkinnedMesh::FindNodeAnimationChannel(const aiAnimation* pAnimation, aiString boneName)
{
	for (unsigned int ChannelIndex = 0; ChannelIndex != pAnimation->mNumChannels; ChannelIndex++)
	{
		if (pAnimation->mChannels[ChannelIndex]->mNodeName == boneName)
		{
			return pAnimation->mChannels[ChannelIndex];
		}
	}
	return 0;
}

unsigned int cSkinnedMesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (unsigned int RotationKeyIndex = 0; RotationKeyIndex != pNodeAnim->mNumRotationKeys - 1; RotationKeyIndex++)
	{
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[RotationKeyIndex + 1].mTime)
		{
			return RotationKeyIndex;
		}
	}

	return 0;
}

unsigned int cSkinnedMesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (unsigned int PositionKeyIndex = 0; PositionKeyIndex != pNodeAnim->mNumPositionKeys - 1; PositionKeyIndex++)
	{
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[PositionKeyIndex + 1].mTime)
		{
			return PositionKeyIndex;
		}
	}

	return 0;
}

unsigned int cSkinnedMesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (unsigned int ScalingKeyIndex = 0; ScalingKeyIndex != pNodeAnim->mNumScalingKeys - 1; ScalingKeyIndex++)
	{
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[ScalingKeyIndex + 1].mTime)
		{
			return ScalingKeyIndex;
		}
	}

	return 0;
}

void cSkinnedMesh::ReadNodeHierarchy(float AnimationTime,
	std::string animationName,
	const aiNode* pNode,
	const glm::mat4 &ParentTransformMatrix)
{
	aiString NodeName(pNode->mName.data);

	const aiAnimation* pAnimation = this->Scene->mAnimations[0];
	std::map< std::string, const aiScene* >::iterator itAnimation = MapAnimationNameToScene.find(animationName);
	if (itAnimation != MapAnimationNameToScene.end())
	{
		pAnimation = reinterpret_cast<const aiAnimation*>(itAnimation->second->mAnimations[0]);
	}

	glm::mat4 NodeTransformation = AIMatrixToGLMMatrix(pNode->mTransformation);
	const aiNodeAnim* pNodeAnim = this->FindNodeAnimationChannel(pAnimation, NodeName);

	if (pNodeAnim)
	{
		// Get interpolated scaling
		glm::vec3 scale;
		this->CalcGLMInterpolatedScaling(AnimationTime, pNodeAnim, scale);
		glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), scale);

		// Get interpolated rotation (quaternion)
		glm::quat ori;
		this->CalcGLMInterpolatedRotation(AnimationTime, pNodeAnim, ori);
		glm::mat4 RotationM = glm::mat4_cast(ori);

		// Get interpolated position 
		glm::vec3 pos;
		this->CalcGLMInterpolatedPosition(AnimationTime, pNodeAnim, pos);
		glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), pos);

		// Combine the above transformations
		NodeTransformation = TranslationM * RotationM * ScalingM;
	}

	glm::mat4 ObjectBoneTransformation = ParentTransformMatrix * NodeTransformation;

	std::map<std::string, unsigned int>::iterator it = this->MapBoneNameToBoneIndex.find(std::string(NodeName.data));
	if (it != this->MapBoneNameToBoneIndex.end())
	{
		unsigned int BoneIndex = it->second;
		this->VecBoneInfo[BoneIndex].ObjectBoneTransformation = ObjectBoneTransformation;
		this->VecBoneInfo[BoneIndex].FinalTransformation = this->GlobalInverseTransformation
			* ObjectBoneTransformation
			* this->VecBoneInfo[BoneIndex].BoneOffset;
	}
	else
	{
		int breakpoint = 0;
	}

	for (unsigned int ChildIndex = 0; ChildIndex != pNode->mNumChildren; ChildIndex++)
	{
		this->ReadNodeHierarchy(AnimationTime, animationName,
			pNode->mChildren[ChildIndex], ObjectBoneTransformation);
	}
}

void cSkinnedMesh::CalcInterpolatedRotation(float AnimationTime, const aiNodeAnim* pNodeAnim, aiQuaternion &out)
{
	if (pNodeAnim->mNumRotationKeys == 1)
	{
		out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	unsigned int RotationIndex = this->FindRotation(AnimationTime, pNodeAnim);
	unsigned int NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
	aiQuaternion::Interpolate(out, StartRotationQ, EndRotationQ, Factor);
	out = out.Normalize();

	return;
}

void cSkinnedMesh::CalcInterpolatedPosition(float AnimationTime, const aiNodeAnim* pNodeAnim, aiVector3D &out)
{
	if (pNodeAnim->mNumPositionKeys == 1)
	{
		out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}

	unsigned int PositionIndex = this->FindPosition(AnimationTime, pNodeAnim);
	unsigned int NextPositionIndex = (PositionIndex + 1);
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& StartPosition = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& EndPosition = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	out = (EndPosition - StartPosition) * Factor + StartPosition;

	return;
}

void cSkinnedMesh::CalcInterpolatedScaling(float AnimationTime, const aiNodeAnim* pNodeAnim, aiVector3D &out)
{
	if (pNodeAnim->mNumScalingKeys == 1)
	{
		out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	unsigned int ScalingIndex = this->FindScaling(AnimationTime, pNodeAnim);
	unsigned int NextScalingIndex = (ScalingIndex + 1);
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& StartScale = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& EndScale = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	out = (EndScale - StartScale) * Factor + StartScale;

	return;
}

void cSkinnedMesh::CalcGLMInterpolatedRotation(float AnimationTime, const aiNodeAnim* pNodeAnim, glm::quat &out)
{
	if (pNodeAnim->mNumRotationKeys == 1)
	{
		out.w = pNodeAnim->mRotationKeys[0].mValue.w;
		out.x = pNodeAnim->mRotationKeys[0].mValue.x;
		out.y = pNodeAnim->mRotationKeys[0].mValue.y;
		out.z = pNodeAnim->mRotationKeys[0].mValue.z;
		return;
	}

	unsigned int RotationIndex = this->FindRotation(AnimationTime, pNodeAnim);
	unsigned int NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	if (Factor < 0.0f) Factor = 0.0f;
	if (Factor > 1.0f) Factor = 1.0f;
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;

	glm::quat StartGLM = glm::quat(StartRotationQ.w, StartRotationQ.x, StartRotationQ.y, StartRotationQ.z);
	glm::quat EndGLM = glm::quat(EndRotationQ.w, EndRotationQ.x, EndRotationQ.y, EndRotationQ.z);

	out = glm::slerp(StartGLM, EndGLM, Factor);

	out = glm::normalize(out);

	return;
}

void cSkinnedMesh::CalcGLMInterpolatedPosition(float AnimationTime, const aiNodeAnim* pNodeAnim, glm::vec3 &out)
{
	if (pNodeAnim->mNumPositionKeys == 1)
	{
		out.x = pNodeAnim->mPositionKeys[0].mValue.x;
		out.y = pNodeAnim->mPositionKeys[0].mValue.y;
		out.z = pNodeAnim->mPositionKeys[0].mValue.z;
		return;
	}

	unsigned int PositionIndex = this->FindPosition(AnimationTime, pNodeAnim);
	unsigned int NextPositionIndex = (PositionIndex + 1);
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	if (Factor < 0.0f) Factor = 0.0f;
	if (Factor > 1.0f) Factor = 1.0f;
	const aiVector3D& StartPosition = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& EndPosition = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	glm::vec3 start = glm::vec3(StartPosition.x, StartPosition.y, StartPosition.z);
	glm::vec3 end = glm::vec3(EndPosition.x, EndPosition.y, EndPosition.z);

	out = (end - start) * Factor + start;

	return;
}

void cSkinnedMesh::CalcGLMInterpolatedScaling(float AnimationTime, const aiNodeAnim* pNodeAnim, glm::vec3 &out)
{
	if (pNodeAnim->mNumScalingKeys == 1)
	{
		out.x = pNodeAnim->mScalingKeys[0].mValue.x;
		out.y = pNodeAnim->mScalingKeys[0].mValue.y;
		out.z = pNodeAnim->mScalingKeys[0].mValue.z;
		return;
	}

	unsigned int ScalingIndex = this->FindScaling(AnimationTime, pNodeAnim);
	unsigned int NextScalingIndex = (ScalingIndex + 1);
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	if (Factor < 0.0f) Factor = 0.0f;
	if (Factor > 1.0f) Factor = 1.0f;
	const aiVector3D& StartScale = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& EndScale = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	glm::vec3 start = glm::vec3(StartScale.x, StartScale.y, StartScale.z);
	glm::vec3 end = glm::vec3(EndScale.x, EndScale.y, EndScale.z);
	out = (end - start) * Factor + start;

	return;
}

float cSkinnedMesh::GetDuration(void)
{
	float duration = (float)(this->Scene->mAnimations[0]->mDuration / this->Scene->mAnimations[0]->mTicksPerSecond);
	return duration;
}

float cSkinnedMesh::GetAnimationDuration(const aiScene* scene)
{
	float duration = (float)(scene->mAnimations[0]->mDuration / scene->mAnimations[0]->mTicksPerSecond);
	return duration;
}

void cSkinnedMesh::Draw(cShaderProgram shader)
{
	for (unsigned int i = 0; i < this->vecMeshes.size(); i++)
		this->vecMeshes[i].Draw(shader);
}

bool cSkinnedMesh::processNode(aiNode * node, const aiScene * scene)
{
	if (node->mNumMeshes > 0)
	{
		aiMesh * mesh = scene->mMeshes[node->mMeshes[0]];

		this->Initialize(0);
		this->vecMeshes.push_back(processMesh(mesh, scene));
		return true;
	}
	else
	{
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			if (processNode(node->mChildren[i], scene))
			{
				return true;
			}
		}
	}

	//for (unsigned int i = 0; i < node->mNumMeshes; i++)
	//{
	//	aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];

	//	//this->Initialize(i);
	//	this->Initialize(i);
	//	this->vecMeshes.push_back(processMesh(mesh, scene));
	//}
	//for (unsigned int i = 0; i < node->mNumChildren; i++)
	//{
	//	(processNode(node->mChildren[i], scene))
	//}
}

cMesh cSkinnedMesh::processMesh(aiMesh * mesh, const aiScene * scene)
{
	std::vector<sSkinnedMeshVertex> vertices;
	std::vector<GLuint> indices;
	std::vector<sTexture> textures;


	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		sSkinnedMeshVertex vertex;
		glm::vec3 vector;

		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;

		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;

		if (mesh->mTextureCoords[0])
		{
			glm::vec2 vec;

			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
		{
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		}

		if (mesh->HasTangentsAndBitangents())
		{
			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.Tangent = vector;

			vector.x = mesh->mBitangents[i].x;
			vector.y = mesh->mBitangents[i].y;
			vector.z = mesh->mBitangents[i].z;
			vertex.BiTangent = vector;
		}
		else
		{
			vertex.Tangent = glm::vec3(0.0f);
			vertex.BiTangent = glm::vec3(0.0f);
		}

		vertex.BoneID[0] = this->VecVertexBoneData[i].Ids[0];
		vertex.BoneID[1] = this->VecVertexBoneData[i].Ids[1];
		vertex.BoneID[2] = this->VecVertexBoneData[i].Ids[2];
		vertex.BoneID[3] = this->VecVertexBoneData[i].Ids[3];

		vertex.BoneWeights[0] = this->VecVertexBoneData[i].Weights[0];
		vertex.BoneWeights[1] = this->VecVertexBoneData[i].Weights[1];
		vertex.BoneWeights[2] = this->VecVertexBoneData[i].Weights[2];
		vertex.BoneWeights[3] = this->VecVertexBoneData[i].Weights[3];

		vertices.push_back(vertex);
	}
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	std::vector<sTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

	std::vector<sTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

	std::vector<sTexture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

	std::vector<sTexture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	return cMesh(vertices, indices, textures);
}

std::vector<sTexture> cSkinnedMesh::loadMaterialTextures(aiMaterial * mat, aiTextureType type, std::string typeName)
{
	std::vector<sTexture> textures;

	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (unsigned int j = 0; j < this->vecTexturesLoaded.size(); j++)
		{
			if (std::strcmp(this->vecTexturesLoaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(this->vecTexturesLoaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip)
		{
			sTexture texture;
			texture.ID = SMTextureFromFile(str.C_Str(), this->directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			this->vecTexturesLoaded.push_back(texture);
		}
	}
	return textures;
}

unsigned int SMTextureFromFile(const char *path, const std::string &directory, bool gamma)
{
	std::string pathString(path);
	std::size_t count = pathString.find_last_of("/\\");
	std::string filename = pathString.substr(count + 1, pathString.size());

	//std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char * data = SOIL_load_image(filename.c_str(), &width, &height, &nrComponents, SOIL_LOAD_AUTO);
	if (data)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		SOIL_free_image_data(data);
	}
	else
	{
		printf("Skinned Mesh Texture failed to load.\n");
	}
	return textureID;
}