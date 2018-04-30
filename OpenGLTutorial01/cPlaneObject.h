#ifndef _HG_cPlaneObject_
#define _HG_cPlaneObject_

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class cPlaneObject
{
public:
	cPlaneObject();

	unsigned int VAO;

private:
	unsigned int VBO, EBO;
};

#endif