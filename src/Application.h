#pragma once
#include "VulkanInstance.h"

class Application {
	VulkanInstance vk;

	glm::vec3 camPos;
	glm::vec3 camForward;
	float camSpeed;

public:
	Application(VulkanInstance vkInstance);
	void MainLoop();
	void Update();
	void Render();
};