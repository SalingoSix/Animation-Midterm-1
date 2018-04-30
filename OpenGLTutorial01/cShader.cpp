#include "cShaderProgram.h"

#include <fstream>
#include <sstream>

cShader::cShader()
{
	ID = -1;
	numberOfLines = 0;
}

cShader::~cShader()
{

}

void cShader::setPath(std::string setPath)
{
	this->path = setPath;
}

bool cShader::readFile(std::string name)
{
	std::string fileName = path + name;
	std::ifstream file(fileName.c_str());
	if (!file.is_open())
	{
		return false;
	}

	this->shaderSource.clear();

	char pLineTemp[65536] = { 0 };
	while (file.getline(pLineTemp, 65536))
	{
		std::string thisLine(pLineTemp);
		shaderSource.push_back(thisLine);
	}

	numberOfLines = shaderSource.size();

	setArray();

	return true;
}

void cShader::setArray()
{
	arraySource = new char*[numberOfLines];

	memset(arraySource, 0, numberOfLines);

	for (int index = 0; index < shaderSource.size(); index++)
	{
		unsigned int lineLength = shaderSource[index].size();

		arraySource[index] = new char[lineLength + 2];
		memset(arraySource[index], 0, lineLength + 2);

		for (int charIndex = 0; charIndex < shaderSource[index].size(); charIndex++)
		{
			arraySource[index][charIndex] = shaderSource[index][charIndex];
		}

		arraySource[index][lineLength + 0] = '\n';
		arraySource[index][lineLength + 1] = '\0';
	}
	return;
}