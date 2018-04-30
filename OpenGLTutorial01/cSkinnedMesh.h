#ifndef _HG_cSkinnedMesh_
#define _HG_cSkinnedMesh_

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include <map>
#include <vector>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm\glm.hpp>

#include "cMesh.h"
class cShaderProgram;

class cSkinnedMesh
{
private:
	static const int MAX_BONES_PER_VERTEX = 4;

	struct sVertexBoneData
	{
		std::array<float, MAX_BONES_PER_VERTEX> Ids;
		std::array<float, MAX_BONES_PER_VERTEX> Weights;

		void AddBoneData(unsigned int BoneID, float Weight);
	};

	struct sBoneInfo
	{
		glm::mat4 BoneOffset;
		glm::mat4 FinalTransformation;
		glm::mat4 ObjectBoneTransformation;
	};

public:
	unsigned int NumVertices;
	unsigned int NumIndices;
	unsigned int NumTriangles;
	unsigned int NumBones;

	std::string Filename;
	std::string Name;

	const aiScene* Scene;
	Assimp::Importer Importer;

	std::map<std::string, const aiScene*> MapAnimationNameToScene;
	std::vector<sVertexBoneData> VecVertexBoneData;
	std::map<std::string, unsigned int> MapBoneNameToBoneIndex;
	std::vector<sBoneInfo> VecBoneInfo;
	std::vector<cMesh> VecMeshes;

	glm::mat4 GlobalInverseTransformation;

	cSkinnedMesh(const std::string& filename);
	~cSkinnedMesh();

	bool LoadMeshFromFile(const std::string& filename);
	bool LoadMeshAnimation(const std::string& filename);

	float FindAnimationTotalTime(std::string animationName);
	float GetDuration();
	float GetAnimationDuration(const aiScene* scene);

	void BoneTransform(float time, std::string animationName, std::vector<glm::mat4>& finalTransformation, std::vector<glm::mat4>& globals, std::vector<glm::mat4>& offsets);

	bool Initialize();
	bool Initialize(int index);

	void CalcInterpolatedRotation(float AnimationTime, const aiNodeAnim* pNodeAnim, aiQuaternion& out);
	void CalcInterpolatedPosition(float AnimationTime, const aiNodeAnim* pNodeAnim, aiVector3D& out);
	void CalcInterpolatedScaling(float AnimationTime, const aiNodeAnim* pNodeAnim, aiVector3D& out);

	void CalcGLMInterpolatedRotation(float AnimationTime, const aiNodeAnim* pNodeAnim, glm::quat& out);
	void CalcGLMInterpolatedPosition(float AnimationTime, const aiNodeAnim* pNodeAnim, glm::vec3& out);
	void CalcGLMInterpolatedScaling(float AnimationTime, const aiNodeAnim* pNodeAnim, glm::vec3& out);

	unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);

	const aiNodeAnim* FindNodeAnimationChannel(const aiAnimation* pAnimation, aiString nodeOrBoneName);

	void ReadNodeHierarchy(float animationTime, std::string animationName, const aiNode* node, const glm::mat4 &parentTransformMatrix);
	void LoadBones(const aiMesh* mesh, std::vector<sVertexBoneData>& bones);

	void Draw(cShaderProgram shader);
private:
	std::vector<cMesh> vecMeshes;
	std::vector<sTexture> vecTexturesLoaded;
	std::string directory;
	void loadModel(std::string path);
	bool processNode(aiNode* node, const aiScene* scene);
	cMesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<sTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};


#endif