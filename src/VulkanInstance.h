#pragma once
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

#include "Primative.h"

#include <vector>

class VulkanInstance;

//Create with VulkanInstance.CreateIndexBuffer()
struct IndexBuffer {
	void update(std::vector<unsigned short>& indicies);
	void destroy();
	VkBuffer buffer;
	VkDeviceMemory deviceMemory;
	VulkanInstance* vk;
};
//Create with VulkanInstance.CreateVertexBuffer()
struct VertexBuffer {
	void update(std::vector<void*>& verticies);
	void destroy();
	VkBuffer buffer;
	VkDeviceMemory deviceMemory;
	VulkanInstance* vk;
};

class VulkanInstance {
public:
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT callback;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	std::vector<uint32_t> queueIndicies;
	VkQueue graphicsQueue;
	VkSurfaceKHR surface;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	VkCommandBuffer drawCmd;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> sentFrameFences;
	size_t currentFrame = 0;
	bool framebufferResized = false;

	std::vector<VertexBuffer> vertexBuffers;
	std::vector<IndexBuffer> indexBuffers;

	VkImage outputImage;
	VkDeviceMemory outputImageMemory;
	VkImageView outputImageView;
	VkSampler outputTextureSampler;

	VkPipelineLayout compPipelineLayout;
	VkPipeline compPipeline;
	VkCommandBuffer compCmd;
	VkDescriptorSetLayout compDescriptorSetLayout;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	void Init();

	void Cleanup();
	
	void drawFrame();

	void beginSetCmdBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

	void endSetCmdBuffer(VkCommandBuffer buffer);

	VkCommandBuffer beginSingleTimeCommands();

	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	VertexBuffer* CreateVertexBuffer(std::vector<primative::IVertexData> verticies);

	IndexBuffer* CreateIndexBuffer(std::vector<unsigned short> indicies);

private:
	void InitVulkan();

	void InitWindow();

	void CleanupSwapchain();

	void recreateSwapchain();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	std::vector<const char*> ValidateExtensions();

	void CreateInstance();

	bool CheckValidationLayerSupport();

	std::vector<const char*> GetRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator);

	void SetupDebugCallback();

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	int RateDeviceSuitability(VkPhysicalDevice device);

	void PickPhysicalDevice();

	struct QueueFamilyIndices;

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	void CreateLogicalDevice();

	void CreateSurface();

	struct SwapChainSupportDetails;

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availibleFormats);

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availiblePresentmodes);

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capacities);

	void CreateSwapChain();

	void CreateImageViews();

	void CreateRenderPass();

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& imageMemory);

	VkImageView createImageView(VkImage image, VkFormat format);

	void createTextureSampler();

	VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode);

	void CreatePipeline();

	void CreateComputePipeline();
	
	void CreateDescriptorPool();

	void CreateDescriptorSets();

	void CreateFramebuffers();

	void CreateCommandPool();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void CreateCommandBuffers();

	void resetCmdBuffer(size_t index);

	void CreateSynchronizers();
};