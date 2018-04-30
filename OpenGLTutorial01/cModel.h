#ifndef _HG_cModel_
#define _HG_cModel_

#include <vector>
#include <string>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "cShaderProgram.h"
#include "cMesh.h"

class cModel
{
public:
	cModel(std::string path);
	void Draw(cShaderProgram shader);

private:
	std::vector<sTexture> textures_loaded;
	std::vector<cMesh> meshes;
	std::string directory;

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	cMesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<sTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
	unsigned int TextureFromFile(const char* path, const std::string &directory, bool gamma = false);
};

#endif