#pragma once
#include "VulkanInstance.h"

class Application {
	VulkanInstance vk;

public:
	Application(VulkanInstance vkInstance);
	void MainLoop();
	void Update();
	void Render();
};