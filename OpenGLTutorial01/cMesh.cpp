#include "cMesh.h"

cMesh::cMesh(std::vector<sVertex> theVertices, std::vector<unsigned int> theIndices, std::vector<sTexture> theTextures)
{
	vertices = theVertices;
	indices = theIndices;
	textures = theTextures;
	skinnedMesh = false;
	setupMesh();
}

cMesh::cMesh(std::vector<sSkinnedMeshVertex> theVertices, std::vector<unsigned int> theIndices, std::vector<sTexture> theTextures)
{
	skinnedVertices = theVertices;
	indices = theIndices;
	textures = theTextures;
	skinnedMesh = true;
	setupMesh();
}

void cMesh::Draw(cShaderProgram shader)
{
	unsigned int diffuseNum = 1;
	unsigned int specularNum = 1;

	for (int index = 0; index < textures.size(); index++)
	{
		glActiveTexture(GL_TEXTURE0 + index);

		std::string number;
		std::string name = textures[index].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNum++);
		else if (name == "texture_specular")
			number = std::to_string(specularNum++);

		shader.setInt(("material." + name + number).c_str(), index);
		glBindTexture(GL_TEXTURE_2D, textures[index].ID);
	}
	glActiveTexture(GL_TEXTURE0);

	//Once all textures are bound, draw
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void cMesh::setupMesh()
{

	if (skinnedMesh)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		//Bind VAO
		glBindVertexArray(VAO);
		//Set vertex data
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, skinnedVertices.size() * sizeof(sSkinnedMeshVertex), &skinnedVertices[0], GL_STATIC_DRAW);
		//Set index data
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//Set vertex attributes
		//Position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(sSkinnedMeshVertex), (void*)0);
		glEnableVertexAttribArray(0);
		//Normals
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(sSkinnedMeshVertex), (void*)offsetof(sSkinnedMeshVertex, Normal));
		glEnableVertexAttribArray(1);
		//Texture coordinates
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(sSkinnedMeshVertex), (void*)offsetof(sSkinnedMeshVertex, TexCoords));
		glEnableVertexAttribArray(2);
		//Tangent
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(sSkinnedMeshVertex), (void*)offsetof(sSkinnedMeshVertex, Tangent));
		glEnableVertexAttribArray(3);
		//BiTangent
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(sSkinnedMeshVertex), (void*)offsetof(sSkinnedMeshVertex, BiTangent));
		glEnableVertexAttribArray(4);
		//Bone IDs
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(sSkinnedMeshVertex), (void*)offsetof(sSkinnedMeshVertex, BoneID));
		glEnableVertexAttribArray(5);
		//Bone Weights
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(sSkinnedMeshVertex), (void*)offsetof(sSkinnedMeshVertex, BoneWeights));
		glEnableVertexAttribArray(6);

		glBindVertexArray(0);
	}

	else
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		//Bind VAO
		glBindVertexArray(VAO);
		//Set vertex data
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(sVertex), &vertices[0], GL_STATIC_DRAW);
		//Set index data
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//Set vertex attributes
		//Position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)0);
		glEnableVertexAttribArray(0);
		//Normals
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, Normal));
		glEnableVertexAttribArray(1);
		//Texture coordinates
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(sVertex), (void*)offsetof(sVertex, TexCoords));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}

}