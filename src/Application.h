#pragma once
#include "VulkanInstance.h"

class Application {
	VulkanInstance vk;

	glm::vec3 camPos;
	glm::vec3 camForward;
	float camSpeed;
	float camHorizontal = 0.0f;
	float camVertical = 0.0f;
	float mouseSpeed = 1.0f;

public:
	Application(VulkanInstance vkInstance);
	void MainLoop();
	void Update();
	void Render();
};