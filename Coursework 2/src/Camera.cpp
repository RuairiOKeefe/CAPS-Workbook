#include "Camera.h"
#include <iostream>
#include <glm\gtx\string_cast.hpp>
void Camera::Update(float deltaTime)
{
	// Calculate the forward direction - spherical coordinates to Cartesian
	glm::vec3 forward(cosf(pitch) * -sinf(yaw), sinf(pitch), -cosf(yaw) * cosf(pitch));
	// Normalize forward
	forward = glm::normalize(forward);

	// Calculate standard right.  Rotate right vector by yaw
	glm::vec3 right = glm::vec3(glm::eulerAngleY(yaw) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	// Normalize right
	right = glm::normalize(right);

	// Up vector is up rotated by pitch
	up = glm::cross(right, forward);
	// Normalize up
	up = glm::normalize(up);

	// We can now update position based on forward, up and right
	glm::vec3 trans = translation.x * right;
	trans += translation.y * up;
	trans += translation.z * forward;
	position += trans;

	// Target vector is just our position vector plus forward vector
	target = position + forward;
	// Set the translation vector to zero for the next frame
	translation = glm::vec3(0.0f, 0.0f, 0.0f);
	// We can now calculate the view matrix
	view = glm::lookAt(position, target, up);
}