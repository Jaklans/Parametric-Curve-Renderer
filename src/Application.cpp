#pragma once
#define GLM_EXT_INCLUDED
#include "Application.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Settings.h"


Application::Application(VulkanInstance vkInstance) {
	vk = vkInstance;



//Project Specific Initialization

	glm::vec4 in (0, 0, 1, 0);
	glm::mat4 mat = glm::rotate(glm::rotate(glm::mat4(1.0f), 25.0f * glm::pi<float>() /180, glm::vec3(0.0f, 1.0f, 0.0f)), 25.0f * glm::pi<float>() / 180, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec4 result = mat * in;
	


	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.image = vk.outputImage;
	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vk.beginSetCmdBuffer(vk.compCmd, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
	vkCmdBindPipeline(vk.compCmd, VK_PIPELINE_BIND_POINT_COMPUTE, vk.compPipeline);
	vkCmdBindDescriptorSets(
		vk.compCmd, VK_PIPELINE_BIND_POINT_COMPUTE, 
		vk.compPipelineLayout, 
		0, 1, 
		&vk.descriptorSets[vk.swapChainImages.size()],
		0, 0);

	vkCmdDispatch(vk.compCmd, WIDTH, HEIGHT, 1);
	vk.endSetCmdBuffer(vk.compCmd);

	vk.beginSetCmdBuffer(vk.drawCmd);

	//vkCmdPipelineBarrier(
	//	vk.drawCmd,
	//	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	//	VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	//	0,
	//	0, nullptr,
	//	0, nullptr,
	//	1, &imageMemoryBarrier);

	vkCmdBindDescriptorSets(
		vk.drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		vk.pipelineLayout,
		0, 1,
		&vk.descriptorSets[0],
		0, 0);

	
	vkCmdBindPipeline(vk.drawCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphicsPipeline);
	vkCmdDraw(vk.drawCmd, 3, 1, 0, 0);
	vk.endSetCmdBuffer(vk.drawCmd);
}

void Application::MainLoop() {
	while (!glfwWindowShouldClose(vk.window)) {
		glfwPollEvents();
		Update();
		Render();
	}
}

void Application::Update() {
}

bool ff = true;
void Application::Render() {
	vk.drawFrame(ff);
	ff = false;
}
