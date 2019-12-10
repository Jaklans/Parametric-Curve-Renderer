#pragma once
#define GLM_EXT_INCLUDED
#include "Application.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Settings.h"


Application::Application(VulkanInstance vkInstance) {
	vk = vkInstance;

//Project Specific Initialization
	camPos = { 0.0f, 0.0f, -5.0f };
	camForward = { 0.0f, 0.0f, 1.0f };
	camSpeed = .05f;

	vk.beginSetCmdBuffer(vk.drawCmd);

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
	if (glfwGetKey(vk.window, GLFW_KEY_UP) == GLFW_PRESS) {
		camPos += camForward * camSpeed;
	}
	if (glfwGetKey(vk.window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		camPos -= camForward * camSpeed;
	}
	if (glfwGetKey(vk.window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		camPos += glm::cross(camForward, { 0.0f, 1.0f, 0.0f }) * camSpeed;
	}
	if (glfwGetKey(vk.window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		camPos -= glm::cross(camForward, { 0.0f, 1.0f, 0.0f }) * camSpeed;
	}
}

void Application::Render() {
	vk.beginSetCmdBuffer(vk.compCmd, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
	vkCmdBindPipeline(vk.compCmd, VK_PIPELINE_BIND_POINT_COMPUTE, vk.compPipeline);
	vkCmdBindDescriptorSets(
		vk.compCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
		vk.compPipelineLayout,
		0, 1,
		&vk.descriptorSets[vk.swapChainImages.size()],
		0, 0);

	glm::mat4 worldToCam = glm::translate(glm::identity<glm::mat4>(), camPos);

	vkCmdPushConstants(vk.compCmd, vk.compPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::mat4), &worldToCam);

	vkCmdDispatch(vk.compCmd, WIDTH, HEIGHT, 1);
	vk.endSetCmdBuffer(vk.compCmd);

	//Draw pipeline remains unchanged

	vk.drawFrame();
}
