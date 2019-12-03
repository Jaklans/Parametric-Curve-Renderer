#pragma once
//Testing and development based on
//https://vulkan-tutorial.com/

#include "VulkanInstance.h"

//Standard Libraries
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <fstream>

#include "Settings.h"

//Defines how many frames can be sent
//for render before CPU must wait
const size_t MAX_FRAMES_SENT = 3;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};
const std::vector<const char*> requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


#ifdef _DEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif


static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::string message("Failed to open file ");
		message.append(filename);
		throw std::runtime_error(message);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}


	void VulkanInstance::Init() {
		//Extrodinarily frustrating, but this fixes a runtime error
		commandBuffers.resize(3);

		glfwInit();

		InitWindow();

		InitVulkan();
	}



	void VulkanInstance::InitWindow() {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void VulkanInstance::InitVulkan() {
		CreateInstance();
		SetupDebugCallback();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreatePipeline();
		CreateFramebuffers();
		CreateCommandPool();

CreateComputePipeline();
createImage(WIDTH, HEIGHT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, outputImage, outputImageMemory);

		CreateCommandBuffers();
		CreateSynchronizers();

		// App Specific
		
		
		outputImageView = createImageView(outputImage, VK_FORMAT_R8G8B8A8_UNORM);
		createTextureSampler();
		CreateDescriptorPool();

		//createImage(WIDTH, HEIGHT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT, computeTarget, computeTargetMemory);
	}

	void VulkanInstance::drawFrame(bool firstFrame) {
		vkWaitForFences(device, 1, &sentFrameFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &sentFrameFences[currentFrame]);

		uint32_t imageIndex;
		VkResult result = VK_RESULT_MAX_ENUM;
		while (result != VK_SUCCESS) {
			result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
				recreateSwapchain();
				framebufferResized = false;
			}
			else if (result != VK_SUCCESS) {
				throw std::runtime_error("Failed to acquire Swap Chain Image");
			}
		}

		//Redefine Command Buffers (unfortunate)
		if(firstFrame) resetCmdBuffer(currentFrame, VK_IMAGE_LAYOUT_UNDEFINED);
		else resetCmdBuffer(currentFrame);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		int i = vkQueueSubmit(graphicsQueue, 1, &submitInfo, sentFrameFences[currentFrame]);
		if (i != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit Draw Command Buffer");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR(presentQueue, &presentInfo);

		if (currentFrame++ >= MAX_FRAMES_SENT - 1) {
			currentFrame = 0;
		}
	}

	void VulkanInstance::Cleanup() {
		for (size_t i = 0; i < MAX_FRAMES_SENT; i++) {
			vkWaitForFences(device, MAX_FRAMES_SENT, sentFrameFences.data(), VK_TRUE, 5000000000);
		}

		CleanupSwapchain();
		
		vkDestroyPipeline(device, compPipeline, nullptr);
		vkDestroyPipelineLayout(device, compPipelineLayout, nullptr);

		for (VertexBuffer vb : vertexBuffers) {
			vb.destroy();
		}
		for (IndexBuffer ib : indexBuffers) {
			ib.destroy();
		}

		vkDestroySampler(device, outputTextureSampler, nullptr);
		vkDestroyImageView(device, outputImageView, nullptr);
		vkDestroyImage(device, outputImage, nullptr);
		vkFreeMemory(device, outputImageMemory, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_SENT; i++) {
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, sentFrameFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
		}

		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VulkanInstance::CleanupSwapchain() {
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		//vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void VulkanInstance::recreateSwapchain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		CleanupSwapchain();

		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreatePipeline();
		CreateFramebuffers();
		CreateCommandBuffers();
		//CreateDescriptorPool();
	}

	void VulkanInstance::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<VulkanInstance*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	std::vector<const char*> VulkanInstance::ValidateExtensions() {
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		VkExtensionProperties* extensions = new VkExtensionProperties[extensionCount];

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

		std::cout << "availible extensions: \n";

		for (int i = extensionCount - 1; i >= 0; i--) {
			std::cout << extensions[i].extensionName << '\n';
		}

		std::cout << '\n';

		std::vector<const char*> required = GetRequiredExtensions();

		for (uint32_t i = 0; i < required.size(); i++) {
			bool printMissing = true;
			for (uint32_t j = 0; j < extensionCount; j++) {
				if (!strcmp(required[i], extensions[j].extensionName)) {
					printMissing = false;
					break;
				}
			}
			if (printMissing) {
				std::cout << "missing extension: " << required[i] << '\n';
			}
		}
		delete []extensions;
		return required;
	}

	void VulkanInstance::CreateInstance() {
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		if (enableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("Validation layers requested were not availible");
		}

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		std::vector<const char *> requiredExtensions = ValidateExtensions();

		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create instance!");
		}
	}

	bool VulkanInstance::CheckValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	std::vector<const char*> VulkanInstance::GetRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	void VulkanInstance::SetupDebugCallback() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;

		VkResult result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback);
		if (result != VK_SUCCESS) {
			std::cout << "Error: " << result << std::endl;
			throw std::runtime_error("Failed to set up debug callback! ");
		}
	}

	bool VulkanInstance::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	struct VulkanInstance::SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities = {};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct VulkanInstance::QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	int VulkanInstance::RateDeviceSuitability(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		int score = 0;

		//Discrete processers are preferred
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		//Maximum resolution may be a good indicator
		score += deviceProperties.limits.maxImageDimension2D;

		//Device must meet all hard requirements for consideration
		if (CheckDeviceExtensionSupport(device)) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);

			if (!swapChainSupport.formats.empty() &&
				!swapChainSupport.presentModes.empty() &&
				deviceFeatures.geometryShader &&
				FindQueueFamilies(device).isComplete()) {
				return score;
			}
		}

		return 0;
	}

	void VulkanInstance::PickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("No GPUs with Vulkan support were found");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		physicalDevice = 0;
		int primarySuitibility = 0;
		int primaryIndex = 0;

		for (uint32_t i = 0; i < deviceCount; i++) {
			int deviceSuitibility = RateDeviceSuitability(devices[i]);
			if (deviceSuitibility > 0 && (!physicalDevice || deviceSuitibility > primarySuitibility)) {
				physicalDevice = devices[i];
				primarySuitibility = deviceSuitibility;
				primaryIndex = i;
			}
		}

		if (!(primarySuitibility > 0)) {
			throw std::runtime_error("Failed to find a suitable GPU!");
		}

		std::cout << "Active Graphics Devices: " << '\n';

		for (uint32_t i = 0; i < deviceCount; i++) {
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
			std::cout << deviceProperties.deviceName << (primaryIndex == i ? " (Primary)\n" : "\n");
		}
	}

	VulkanInstance::QueueFamilyIndices VulkanInstance::FindQueueFamilies(VkPhysicalDevice device) { //TODO: Prioritize queues that can do both
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (queueFamilies[i].queueCount > 0 && presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
		}

		return indices;
	}

	void VulkanInstance::CreateLogicalDevice() {
		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		queueIndicies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : queueIndicies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		createInfo.queueCreateInfoCount = queueIndicies[0] == queueIndicies[1] ? 1 : static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Logical Device");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void VulkanInstance::CreateSurface() {
		VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface");
		}
	}

	VulkanInstance::SwapChainSupportDetails VulkanInstance::QuerySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR VulkanInstance::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availibleFormats) {
		if (availibleFormats.size() == 1 && availibleFormats[0].format == VK_FORMAT_UNDEFINED) {
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const auto& availibleFormat : availibleFormats) {
			if (availibleFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availibleFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availibleFormat;
			}
		}

		return availibleFormats[0];
	}

	VkPresentModeKHR VulkanInstance::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availiblePresentmodes) {
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& availiblePresentmode : availiblePresentmodes) {
			if (availiblePresentmode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availiblePresentmode;

			if (availiblePresentmode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}

		return bestMode;
	}

	VkExtent2D VulkanInstance::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capacities) {
		if (capacities.currentExtent.width != UINT32_MAX) {
			return capacities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			if (actualExtent.width < capacities.minImageExtent.width) {
				actualExtent.width = capacities.minImageExtent.width;
			}
			else if (actualExtent.width > capacities.maxImageExtent.width) {
				actualExtent.width = capacities.minImageExtent.width;
			}
			if (actualExtent.height < capacities.minImageExtent.height) {
				actualExtent.height = capacities.minImageExtent.height;
			}
			else if (actualExtent.height > capacities.maxImageExtent.height) {
				actualExtent.height = capacities.minImageExtent.height;
			}

			return actualExtent;
		}
	}

	void VulkanInstance::CreateSwapChain() {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		swapChainExtent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
			createInfo.queueFamilyIndexCount = 2;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Swapchain");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
	}

	void VulkanInstance::CreateImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (int i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
		}
	}

	void VulkanInstance::CreateRenderPass() {
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Render Pass");
		}
	}

	void VulkanInstance::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& imageMemory){
		VkImageCreateInfo imageInfo = {};

		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Image");
		}

		VkMemoryRequirements memReq;
		vkGetImageMemoryRequirements(device, image, &memReq);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate Image Memory!");
		}

		vkBindImageMemory(device, image, imageMemory, 0);
	}

	VkImageView VulkanInstance::createImageView(VkImage image, VkFormat format) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view");
		}

		return imageView;
	}

	void VulkanInstance::createTextureSampler() {
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		//samplerInfo.maxAnisotropy = 16;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &outputTextureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}

	VkShaderModule VulkanInstance::CreateShaderModule(const std::vector<char>& shaderCode) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Shader Module");
		}

		return shaderModule;
	}

	void VulkanInstance::CreatePipeline() {
		auto vertShaderCode = readFile("assets/shaders/clipSpaceTri.vert.spv");
		auto fragShaderCode = readFile("assets/shaders/renderTexture.frag.spv");

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {}; {
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShaderModule;
			vertShaderStageInfo.pName = "main";
		}

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {}; {
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";
		}

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = primative::Vertex::getBindingDescription();
		auto attributeDescriptions = primative::Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {}; {
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 0;
			vertexInputInfo.pVertexBindingDescriptions = nullptr;//&bindingDescription;
			vertexInputInfo.vertexAttributeDescriptionCount = 0;//static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = nullptr; // attributeDescriptions.data();
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {}; {
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;
		}

		VkViewport viewport = {}; {
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)swapChainExtent.width;
			viewport.height = (float)swapChainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
		}

		VkRect2D scissor = {}; {
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent;
		}

		VkPipelineViewportStateCreateInfo viewportState = {}; {
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;
		}

		VkPipelineRasterizationStateCreateInfo rasterizer = {}; {
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;
			rasterizer.depthBiasConstantFactor = 0.0f;
			rasterizer.depthBiasClamp = 0.0f;
			rasterizer.depthBiasSlopeFactor = 0.0f;
		}

		VkPipelineMultisampleStateCreateInfo multisampling = {}; {
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.minSampleShading = 1.0f;
			multisampling.pSampleMask = nullptr;
			multisampling.alphaToCoverageEnable = VK_FALSE;
			multisampling.alphaToOneEnable = VK_FALSE;
		}

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; {
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending = {}; {
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;
		}

		//Pipeline Layout
		{
			//VkPushConstantRange pushConstantRange = primative::Uniform::getPushConstantRange();

			VkDescriptorSetLayoutBinding layoutDescriptors[1];

			//Sampler Layout Binding
			layoutDescriptors[0] = {};
			layoutDescriptors[0].binding = 0;
			layoutDescriptors[0].descriptorCount = 1;
			layoutDescriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutDescriptors[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutDescriptors[0].pImmutableSamplers = nullptr;

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 1;
			layoutInfo.pBindings = layoutDescriptors;

			if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create descriptor set layout");
			}

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr; //&pushConstantRange;

			if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Pipeline Layout");
			}
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {}; {
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = nullptr;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = nullptr;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;
		}

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device, vertShaderModule, nullptr);
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
	}

	void VulkanInstance::CreateFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer");
			}
		}
	}

	void VulkanInstance::CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		//Modified to alow reseting
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Command Pool");
		}
	}

	uint32_t VulkanInstance::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void VulkanInstance::CreateCommandBuffers() {
		//Create Main Draw and Compute Command Buffer
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 2;

		if (vkAllocateCommandBuffers(device, &allocInfo, &drawCmd) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Command Buffers");
		}

		compCmd = *(&drawCmd + 1);

		beginSetCmdBuffer(compCmd, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
		vkCmdBindPipeline(compCmd, VK_PIPELINE_BIND_POINT_COMPUTE, compPipeline);
		endSetCmdBuffer(compCmd);

		beginSetCmdBuffer(drawCmd);
		endSetCmdBuffer(drawCmd);

		//Create command buffers for three swapchains
		//(They will only clear the screen and call secondary buffers)
		commandBuffers.resize(swapChainFramebuffers.size());

		allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Command Buffers");
		}

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			resetCmdBuffer(i);
		}
	}

	void VulkanInstance::resetCmdBuffer(size_t index, VkImageLayout initialLayoutTEMP) {
		vkResetCommandBuffer(commandBuffers[index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffers[index], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording Command Buffer");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[index];
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0.0f,0.0f,0.0f,1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
vkCmdExecuteCommands(commandBuffers[index], 1, &compCmd);
VkImageMemoryBarrier imageMemoryBarrier = {};
imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
imageMemoryBarrier.oldLayout = initialLayoutTEMP;
imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
imageMemoryBarrier.image = outputImage;
imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
vkCmdPipelineBarrier(
	commandBuffers[index],
	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	0,
	0, nullptr,
	0, nullptr,
	1, &imageMemoryBarrier);

		vkCmdBeginRenderPass(commandBuffers[index], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		//vkCmdBindPipeline(commandBuffers[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);


		
		vkCmdExecuteCommands(commandBuffers[index], 1, &drawCmd);

		vkCmdEndRenderPass(commandBuffers[index]);

		if (vkEndCommandBuffer(commandBuffers[index]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record Command Buffer!");
		}
	}

	void VulkanInstance::beginSetCmdBuffer(VkCommandBuffer cmdBuffer, VkCommandBufferUsageFlags flags) {
		vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		VkCommandBufferInheritanceInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		info.renderPass = renderPass;

		VkCommandBufferBeginInfo bufferBeginInfo = {};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bufferBeginInfo.pInheritanceInfo = &info;
		bufferBeginInfo.flags = flags;

		vkBeginCommandBuffer(cmdBuffer, &bufferBeginInfo);
		//vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}

	void VulkanInstance::endSetCmdBuffer(VkCommandBuffer cmdBuffer) {
		vkEndCommandBuffer(cmdBuffer);
	}

	VkCommandBuffer VulkanInstance::beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void VulkanInstance::endSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void VulkanInstance::CreateSynchronizers() {
		imageAvailableSemaphores.resize(MAX_FRAMES_SENT);
		renderFinishedSemaphores.resize(MAX_FRAMES_SENT);
		sentFrameFences.resize(MAX_FRAMES_SENT);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_SENT; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &sentFrameFences[i])) {
				throw std::runtime_error("Failed to create Synchronizers");
			}
		}
	}

	VertexBuffer* VulkanInstance::CreateVertexBuffer(std::vector<primative::IVertexData> verticies) { // Consider updating this to use Staging Buffers
		vertexBuffers.push_back(VertexBuffer());
		VertexBuffer* vb = &vertexBuffers[vertexBuffers.size() - 1];

		vb->vk = this;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(verticies[0]) * verticies.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &vb->buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, vb->buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex =
			findMemoryType(
				memRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &vb->deviceMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(device, vb->buffer, vb->deviceMemory, 0);

		void* data;
		vkMapMemory(device, vb->deviceMemory, 0, bufferInfo.size, 0, &data);
		memcpy(data, verticies.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(device, vb->deviceMemory);

		return vb;
	}

	IndexBuffer* VulkanInstance::CreateIndexBuffer(std::vector<unsigned short> indicies) { // Consider updating this to use Staging Buffers
		indexBuffers.push_back(IndexBuffer());
		IndexBuffer* ib = &indexBuffers[indexBuffers.size() - 1];

		ib->vk = this;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(unsigned short) * indicies.size();
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &ib->buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create index buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, ib->buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex =
			findMemoryType(
				memRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &ib->deviceMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate index buffer memory!");
		}

		vkBindBufferMemory(device, ib->buffer, ib->deviceMemory, 0);

		void* data;
		vkMapMemory(device, ib->deviceMemory, 0, bufferInfo.size, 0, &data);
		memcpy(data, indicies.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(device, ib->deviceMemory);

		return ib;
	}

	void IndexBuffer::update(std::vector<unsigned short>& indicies){
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(unsigned short) * indicies.size();
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		void* data;
		vkMapMemory(vk->device, deviceMemory, 0, bufferInfo.size, 0, &data);
		memcpy(data, indicies.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(vk->device, deviceMemory);
	}

	void IndexBuffer::destroy(){
		if (buffer != 0) {
			vkDestroyBuffer(vk->device, buffer, nullptr);
			buffer = 0;
		}
		if (deviceMemory != 0) {
			vkFreeMemory(vk->device, deviceMemory, nullptr);
			deviceMemory = 0;
		}
	}

	void VertexBuffer::update(std::vector<void*>& indicies) {
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(unsigned short) * indicies.size();
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		void* data;
		vkMapMemory(vk->device, deviceMemory, 0, bufferInfo.size, 0, &data);
		memcpy(data, indicies.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(vk->device, deviceMemory);
	}
	
	void VertexBuffer::destroy() {
		if (buffer != 0 ) {
			vkDestroyBuffer(vk->device, buffer, nullptr);
			buffer = 0;
		}
		if (deviceMemory != 0) {
			vkFreeMemory(vk->device, deviceMemory, nullptr);
			deviceMemory = 0;
		}
	} 


	void VulkanInstance::CreateComputePipeline() {
		auto compShaderCode = readFile("assets/shaders/raycast.comp.spv");
		VkShaderModule compShaderModule = CreateShaderModule(compShaderCode);

		VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = compShaderModule;
		compShaderStageInfo.pName = "main";

		VkDescriptorSetLayoutBinding layoutBindings[2] = {};

		layoutBindings[0] = {};
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBindings[0].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = layoutBindings;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &compDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &compDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &compPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Pipeline Layout");
		}

		VkComputePipelineCreateInfo compPipelineInfo = {};
		compPipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		compPipelineInfo.flags = 0;
		compPipelineInfo.stage = compShaderStageInfo;
		compPipelineInfo.layout = compPipelineLayout;
		compPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		compPipelineInfo.basePipelineIndex = -1;

		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compPipelineInfo, nullptr, &compPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline!");
		}

		vkDestroyShaderModule(device, compShaderModule, nullptr);
	}
	
	void VulkanInstance::CreateDescriptorPool() {
		VkDescriptorPoolSize poolSizes[2];
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[1].descriptorCount = 1;

		
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size() + 1);

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}

		std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);

		layouts.push_back(compDescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size() + 1);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(swapChainImages.size() + 1);
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo.imageView = outputImageView;
		imageInfo.sampler = outputTextureSampler;

		VkWriteDescriptorSet samplerWrite = {};
		samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		samplerWrite.dstBinding = 0;
		samplerWrite.dstArrayElement = 0;
		samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerWrite.descriptorCount = 1;
		samplerWrite.pImageInfo = &imageInfo;

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			samplerWrite.dstSet = descriptorSets[i];
			vkUpdateDescriptorSets(device, 1, &samplerWrite, 0, nullptr);
		}
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		samplerWrite.dstSet = descriptorSets[swapChainImages.size()];
		vkUpdateDescriptorSets(device, 1, &samplerWrite, 0, nullptr);
	}