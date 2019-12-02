#pragma once
#define GLFW_INCLUDE_VULKAN
#include <glm/glm.hpp>
#include <glfw/glfw3.h>
#include <vector>

namespace primative {
	struct IUniformData {
		static VkPushConstantRange getPushConstantRange();
	};

	struct IVertexData {
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
	};

	struct Uniform : IUniformData {
		glm::mat4 modelViewProj;
		glm::vec4 color;

		static VkPushConstantRange getPushConstantRange() {
			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(Uniform);
			pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			return pushConstantRange;
		}
	};
	
	struct Vertex : IVertexData {
		glm::vec3 pos;
		glm::vec3 normal;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		//might be issue
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, normal);

			return attributeDescriptions;
		}
	};
}