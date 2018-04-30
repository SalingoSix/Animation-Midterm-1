#ifndef _HG_cSkinnedGameObject_
#define _HG_cSkinnedGameObject_

#include <glm\vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm\gtc\quaternion.hpp>
#include <glm\gtx\quaternion.hpp>
#include <string>
#include <stdlib.h>
#include <time.h>

#include "cSkinnedMesh.h"
#include "sAnimationState.h"

class cSkinnedGameObject
{
public:

	enum Direction
	{
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT
	};

	glm::vec3 Position, Scale;
	glm::vec3 Forward;
	glm::quat OrientationQuat;
	glm::vec3 OrientationEuler;

	cSkinnedGameObject(std::string modelName, std::string modelDir, glm::vec3 position, glm::vec3 scale, glm::vec3 orientationEuler, std::vector<std::string> charAnimations);

	void Draw(cShaderProgram Shader, float deltaTime, bool &impact, glm::vec3 &impactPoint, bool &opponentLock);

	std::vector<std::string> vecCharacterAnimations;
	sAnimationState* defaultAnimState, *curAnimState;
	std::string animToPlay;
	float Speed;
	float punchDistance;

	cSkinnedMesh * Model;

	cSkinnedGameObject* Opponent;

	//Okay, it's just a health variable
	int FightingSpirit;
	bool deathAnimEnd;

	bool oneLoopEvent;
	int oneFrameStandWalkRun;
private:
	std::vector<glm::mat4> vecBoneTransformation;
	bool blowLanded;
};
#endif