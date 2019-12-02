//Testing and development based on
//https://vulkan-tutorial.com/
//Drawing/Framebuffers

/*TODO:
	
*/

#pragma once
#include <iostream>
#include <stdexcept>
#include "VulkanInstance.h"
#include "Application.h"

int main() {//Initialize Vulkan Instance
		VulkanInstance vk;
		vk.Init();
		Application app(vk);
		app.MainLoop();
	try {
		
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		system("PAUSE");
		return EXIT_FAILURE;
	}
	system("PAUSE");
	return EXIT_SUCCESS;
}