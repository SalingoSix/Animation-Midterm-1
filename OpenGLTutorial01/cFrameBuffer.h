#ifndef _HG_cFrameBuffer_
#define _HG_cFrameBuffer_

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>

class cFrameBuffer
{
public:
	cFrameBuffer(unsigned int SCR_HEIGHT, unsigned int SCR_WIDTH);

	void resize(unsigned int SCR_HEIGHT, unsigned int SCR_WIDTH);

	unsigned int FBO, textureID;

private:
	unsigned int RBO;
};

#endif