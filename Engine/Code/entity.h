#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "platform.h"

class Entity
{
public:
	Entity(glm::vec3 position, u32 model)
	{
		pos = position;
		modelIdx = model;
	}
	~Entity() 
	{

	}

	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);

	u32 modelIdx;

	u32 localParamsOffset;
	u32 localParamsSize;

private:

};