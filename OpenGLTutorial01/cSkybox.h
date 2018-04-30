#ifndef _HG_cSkybox_
#define _HG_cSkybox_

#include <string>
#include <vector>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL2/SOIL2.h>

class cSkybox
{
public:
	std::string directory;
	std::vector<std::string> boxNames;
	unsigned int textureID;
	unsigned int VAO;
	unsigned int VBO;

	cSkybox(std::string);

private:
	float* skyboxVertices;

	void skyboxInit();
};

#endif