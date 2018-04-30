#ifndef _HG_cScreenQuad_
#define _HG_cScreenQuad_

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class cScreenQuad
{
public:
	cScreenQuad();

	unsigned int VAO;

private:
	unsigned int VBO;
};


#endif