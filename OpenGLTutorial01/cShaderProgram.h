#ifndef _HG_cShaderProgram_
#define _HG_cShaderProgram_

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <iostream>

class cShader
{
public:
	cShader();
	~cShader();

	void setPath(std::string);
	bool readFile(std::string);

	int ID;
	std::string path;
	std::vector<std::string> shaderSource;
	unsigned int numberOfLines;
	char** arraySource;

private:
	void setArray();
};

class cShaderProgram
{
public:
	cShaderProgram();
	~cShaderProgram();

	void compileProgram(std::string path, std::string vertFile, std::string fragFile);
	void useProgram();
	void setBool(std::string name, bool value);
	void setInt(std::string name, int value);
	void setFloat(std::string name, float value);
	void setVec3(std::string name, glm::vec3 value);
	void setVec3(std::string name, float x, float y, float z);
	void setMat4(std::string name, glm::mat4 value);
	void setMat4(std::string name, int count, glm::mat4* value);

	int ID;
	cShader vertexShader;
	cShader fragmentShader;
};

#endif