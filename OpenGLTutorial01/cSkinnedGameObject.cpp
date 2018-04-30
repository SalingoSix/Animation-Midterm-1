#include "cSkinnedGameObject.h"
#include "cShaderProgram.h"

#define _USE_MATH_DEFINES
#include <stack>
#include <math.h>
#include <glm\gtc\matrix_transform.hpp>

cSkinnedGameObject::cSkinnedGameObject(std::string modelName, std::string modelDir, glm::vec3 position, glm::vec3 scale, glm::vec3 orientationEuler, std::vector<std::string> charAnimations)
{
	this->Model = new cSkinnedMesh(modelDir.c_str());

	this->Position = position;
	this->Scale = scale;
	this->OrientationQuat = glm::quat(orientationEuler);
	this->OrientationEuler.x = glm::radians(orientationEuler.x);
	this->OrientationEuler.y = glm::radians(orientationEuler.y);
	this->OrientationEuler.z = glm::radians(orientationEuler.z);

	this->Forward = glm::vec3(0.0f, 0.0f, 0.0f);
	this->Forward.z = cos(this->OrientationEuler.y);
	this->Forward.x = sin(this->OrientationEuler.y);
	this->Forward = glm::normalize(this->Forward);

	this->defaultAnimState = new sAnimationState();
	this->defaultAnimState->defaultAnimation.name = modelDir;
	this->defaultAnimState->defaultAnimation.frameStepTime = 0.005f;
	this->defaultAnimState->defaultAnimation.totalTime = this->Model->GetDuration();

	this->curAnimState = new sAnimationState();
	this->curAnimState->defaultAnimation.name = modelDir;
	this->curAnimState->defaultAnimation.frameStepTime = 0.01f;
	this->curAnimState->defaultAnimation.totalTime = this->Model->GetDuration();

	this->vecCharacterAnimations = charAnimations;

	for (int i = 0; i != this->vecCharacterAnimations.size(); i++)
	{
		this->Model->LoadMeshAnimation(this->vecCharacterAnimations[i]);
	}

	//Fun stuff to let us know when a punch has landed
	punchDistance = 0.0f;
	blowLanded = false;
	FightingSpirit = 100;
	deathAnimEnd = false;

	this->Speed = 4.0f;
	this->animToPlay = this->defaultAnimState->defaultAnimation.name;
	oneLoopEvent = false;
	oneFrameStandWalkRun = 0;
}

void cSkinnedGameObject::Draw(cShaderProgram Shader, float deltaTime, bool &impact, glm::vec3 &impactPoint, bool &opponentLock)
{
	float curFrameTime = 0.0f;

	
	if (this->defaultAnimState->defaultAnimation.name == this->animToPlay)
	{	//If no animation is set, the default is played, which is a t-pose
		this->defaultAnimState->defaultAnimation.IncrementTime();
		curFrameTime = this->defaultAnimState->defaultAnimation.currentTime;
	}
	else if (this->animToPlay == "assets/modelsFBX/RPG-Character_Unarmed-Idle(FBX2013).FBX")
	{	//During their idle pose, characters are to remain frozen. We arbitrarily pick a frame in the idle phase. Any one would do.
		curFrameTime = 0.0f;
		punchDistance = 0.0f;
		blowLanded = false;
	}
	else if (this->animToPlay == "assets/modelsFBX/RPG-Character_Unarmed-Death1(FBX2013).FBX")
	{
		//So for whatever reason, the death animation's duration is longer than it should be. But this number works.
		this->curAnimState->defaultAnimation.totalTime = 0.998f;
		this->curAnimState->defaultAnimation.name = this->animToPlay;
		this->curAnimState->defaultAnimation.IncrementTime();
		if (this->curAnimState->defaultAnimation.currentTime == 0.0f || deathAnimEnd)
		{
			curFrameTime = this->curAnimState->defaultAnimation.totalTime;
			deathAnimEnd = true;
		}
		else
		{
			curFrameTime = this->curAnimState->defaultAnimation.currentTime;
		}
	}
	else
	{
		this->curAnimState->defaultAnimation.totalTime = this->Model->MapAnimationNameToScene[this->animToPlay]->mAnimations[0]->mDuration /
			this->Model->MapAnimationNameToScene[this->animToPlay]->mAnimations[0]->mTicksPerSecond;
		this->curAnimState->defaultAnimation.name = this->animToPlay;
		this->curAnimState->defaultAnimation.IncrementTime();
		curFrameTime = this->curAnimState->defaultAnimation.currentTime;
	}

	std::vector<glm::mat4> vecFinalTransformation;
	std::vector<glm::mat4> vecOffsets;

	this->Model->BoneTransform(curFrameTime, animToPlay, vecFinalTransformation, this->vecBoneTransformation, vecOffsets);

	if (!blowLanded && animToPlay == "assets/modelsFBX/RPG-Character_Unarmed-Attack-L3(FBX2013).FBX")
	{
		glm::vec3 pelvisLoc = glm::vec3(vecFinalTransformation[8][3].x, vecFinalTransformation[8][3].y, vecFinalTransformation[8][3].z);
		glm::vec3 leftFistLoc = glm::vec3(vecFinalTransformation[14][3].x, vecFinalTransformation[14][3].y, vecFinalTransformation[14][3].z);

		float theDistance = glm::distance(pelvisLoc, leftFistLoc);
		if (theDistance > 230.0f && theDistance < this->punchDistance)
		{
			blowLanded = true;
			impact = true;
			opponentLock = true;

			int damageValue = rand() % 6 + 5;
			Opponent->FightingSpirit -= damageValue;
			if (Opponent->FightingSpirit < 0)
			{
				Opponent->FightingSpirit = 0;
			}
			if (Opponent->FightingSpirit == 0)
			{
				Opponent->curAnimState->defaultAnimation.currentTime = 0.0f;
				Opponent->animToPlay = "assets/modelsFBX/RPG-Character_Unarmed-Death1(FBX2013).FBX";
			}
			else
			{
				Opponent->oneLoopEvent = true;
				Opponent->curAnimState->defaultAnimation.currentTime = 0.0f;
				Opponent->animToPlay = "assets/modelsFBX/RPG-Character_Unarmed-Stunned(FBX2013).FBX";
			}

			glm::mat4 leftFistMatrix = vecBoneTransformation[14];
			float myScale = this->Scale.x;	//xyz of Scale vector should all be the same anyway
			leftFistMatrix = glm::scale(leftFistMatrix, glm::vec3(myScale));


			glm::vec4 characterPosition = glm::vec4(this->Position, 1.0f);
			glm::vec4 fistLocation = leftFistMatrix * characterPosition;
			fistLocation *= myScale;
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::rotate(model, this->OrientationEuler.y, glm::vec3(0.0f, 1.0f, 0.0f));
			fistLocation = model * fistLocation;

			//fistLocation *= myScale;
			impactPoint = glm::vec3(fistLocation.x, fistLocation.y, fistLocation.z);
		}
		this->punchDistance = theDistance;
	}

	else if (!blowLanded && animToPlay == "assets/modelsFBX/RPG-Character_Unarmed-Attack-R3(FBX2013).FBX")
	{
		//If we use the y value of the positions, the punch "connects" too early
		glm::vec3 pelvisLoc = glm::vec3(vecFinalTransformation[8][3].x, 0.0f, vecFinalTransformation[8][3].z);
		glm::vec3 rightFistLoc = glm::vec3(vecFinalTransformation[21][3].x, 0.0f, vecFinalTransformation[21][3].z);

		float theDistance = glm::distance(pelvisLoc, rightFistLoc);
		if (theDistance > 200.0f && theDistance < this->punchDistance)
		{
			blowLanded = true;
			impact = true;
			opponentLock = true;

			int damageValue = rand() % 6 + 5;
			Opponent->FightingSpirit -= damageValue;
			if (Opponent->FightingSpirit < 0)
			{
				Opponent->FightingSpirit = 0;
			}
			if (Opponent->FightingSpirit == 0)
			{
				Opponent->curAnimState->defaultAnimation.currentTime = 0.0f;
				Opponent->animToPlay = "assets/modelsFBX/RPG-Character_Unarmed-Death1(FBX2013).FBX";
			}
			else
			{
				Opponent->oneLoopEvent = true;
				Opponent->curAnimState->defaultAnimation.currentTime = 0.0f;
				Opponent->animToPlay = "assets/modelsFBX/RPG-Character_Unarmed-Stunned(FBX2013).FBX";
			}

			glm::mat4 rightFistMatrix = vecBoneTransformation[21];
			rightFistMatrix = glm::scale(rightFistMatrix, this->Scale);
			

			glm::vec4 characterPosition = glm::vec4(this->Position, 1.0f);
			glm::vec4 fistLocation = rightFistMatrix * characterPosition;
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::rotate(model, this->OrientationEuler.y, glm::vec3(0.0f, 1.0f, 0.0f));
			fistLocation = model * fistLocation;

			fistLocation *= this->Scale.x;	//xyz of Scale vector should all be the same anyway
			impactPoint = glm::vec3(fistLocation.x, fistLocation.y, fistLocation.z);
		}
		this->punchDistance = theDistance;
	}

	GLuint numBonesUsed = static_cast<GLuint>(vecFinalTransformation.size());
	Shader.useProgram();
	Shader.setInt("numBonesUsed", numBonesUsed);
	glm::mat4* boneMatrixArray = &(vecFinalTransformation[0]);
	Shader.setMat4("bones", numBonesUsed, boneMatrixArray);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, this->Position);
	model = glm::rotate(model, this->OrientationEuler.x, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, this->OrientationEuler.y, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, this->OrientationEuler.z, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, this->Scale);
	Shader.setMat4("model", model);

	this->Model->Draw(Shader);

	if (this->oneLoopEvent)
	{
		if (curFrameTime == 0)
		{
			this->animToPlay = "assets/modelsFBX/RPG-Character_Unarmed-Idle(FBX2013).FBX";
			oneLoopEvent = false;
		}
	}
}