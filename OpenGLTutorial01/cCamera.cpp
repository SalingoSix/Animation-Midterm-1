#include "cCamera.h"

cCamera::cCamera(glm::vec3 Position, glm::vec3 Up, float Pitch, float Yaw)
{
	position = Position;
	worldUp = Up;
	yaw = Yaw;
	pitch = Pitch;

	movementSpeed = SPEED;
	mouseSensitivity = SENSITIVITY;
	zoom = ZOOM;

	updateCameraVectors();
}

glm::mat4 cCamera::getViewMatrix()
{
	return glm::lookAt(position, position + front, up);
}

void cCamera::processKeyboard(Camera_Movement direction, float deltaTime)
{
	float velocity = movementSpeed * deltaTime;
	if (direction == FORWARD)
		position += front * velocity;
	if (direction == BACKWARD)
		position -= front * velocity;
	if (direction == LEFT)
		position -= right * velocity;
	if (direction == RIGHT)
		position += right * velocity;
}

void cCamera::processMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	if (constrainPitch)
	{
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	updateCameraVectors();
}

void cCamera::processMouseScroll(float yOffset)
{
	if (zoom >= 1.0f && zoom <= 45.0f)
		zoom -= yOffset;
	if (zoom <= 1.0f)
		zoom = 1.0f;
	if (zoom >= 45.0f)
		zoom = 45.0f;
}

void cCamera::updateCameraVectors()
{
	glm::vec3 Front;
	Front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	Front.y = sin(glm::radians(pitch));
	Front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	front = glm::normalize(Front);

	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}