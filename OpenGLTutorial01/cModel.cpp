#define STB_IMAGE_IMPLEMENTATION
#include "cModel.h"
#include "src\stb_image.h"

cModel::cModel(std::string path)
{
	loadModel(path);
}

void cModel::Draw(cShaderProgram shader)
{
	for (int index = 0; index < meshes.size(); index++)
	{
		meshes[index].Draw(shader);
	}
}

void cModel::loadModel(std::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);
}

void cModel::processNode(aiNode* node, const aiScene* scene)
{
	//Process all the node's meshes (if any)
	for (int index = 0; index < node->mNumMeshes; index++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[index]];
		meshes.push_back(processMesh(mesh, scene));
	}
	//Then recursively call each of the node's children
	for (int index = 0; index < node->mNumChildren; index++)
	{
		processNode(node->mChildren[index], scene);
	}
}

cMesh cModel::processMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<sVertex> theVertices;
	std::vector<unsigned int> theIndices;
	std::vector<sTexture> theTextures;

	for (int index = 0; index < mesh->mNumVertices; index++)
	{
		sVertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[index].x;
		vector.y = mesh->mVertices[index].y;
		vector.z = mesh->mVertices[index].z;
		vertex.Position = vector;
		vector.x = mesh->mNormals[index].x;
		vector.y = mesh->mNormals[index].y;
		vector.z = mesh->mNormals[index].z;
		vertex.Normal = vector;
		if (mesh->mTextureCoords[0])
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][index].x;
			vec.y = mesh->mTextureCoords[0][index].y;
			vertex.TexCoords = vec;
		}
		else
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);

		theVertices.push_back(vertex);
	}

	for (int index = 0; index < mesh->mNumFaces; index++)
	{
		aiFace face = mesh->mFaces[index];
		for (int faceIndex = 0; faceIndex < face.mNumIndices; faceIndex++)
		{
			theIndices.push_back(face.mIndices[faceIndex]);
		}
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<sTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		theTextures.insert(theTextures.end(), diffuseMaps.begin(), diffuseMaps.end());
		std::vector<sTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		theTextures.insert(theTextures.end(), specularMaps.begin(), specularMaps.end());
	}

	return cMesh(theVertices, theIndices, theTextures);
}

std::vector<sTexture> cModel::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<sTexture> textures;
	for (int index = 0; index < mat->GetTextureCount(type); index++)
	{
		aiString str;
		mat->GetTexture(type, index, &str);
		bool skip = false;
		for (int i = 0; i < textures_loaded.size(); i++)
		{
			if (std::strcmp(textures_loaded[i].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(textures_loaded[i]);
				skip = true;
				break;
			}
		}
		if (!skip)
		{	//If the textures has not been loaded yet, load it now
			sTexture texture;
			texture.ID = TextureFromFile(str.C_Str(), directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}
	return textures;
}

unsigned int cModel::TextureFromFile(const char* path, const std::string &directory, bool gamma)
{
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
	}

	stbi_image_free(data);
	return textureID;
}