#pragma once

#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

class Buffer;

class RenderProcess final
{
public:
  RenderProcess(VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkCommandPool commandPool,
                VkDescriptorPool descriptorPool,
                VkDescriptorSetLayout descriptorSetLayout,
                VkImageView textureImageView,
                VkSampler textureSampler,
                VkImageView textureImageView2,
                VkSampler textureSampler2);
  ~RenderProcess();

  struct UniformBufferData final
  {
    glm::mat4 world;
    glm::mat4 viewProjection[2];
  } uniformBufferData;

  bool isValid() const;
  VkCommandBuffer getCommandBuffer() const;
  VkSemaphore getDrawableSemaphore() const;
  VkSemaphore getPresentableSemaphore() const;
  VkFence getBusyFence() const;
  VkDescriptorSet getDescriptorSet() const;

  bool updateUniformBufferData() const;

private:
  bool valid = true;

  VkDevice device = nullptr;
  VkCommandBuffer commandBuffer = nullptr;
  VkSemaphore drawableSemaphore = nullptr, presentableSemaphore = nullptr;
  VkFence busyFence = nullptr;
  Buffer* uniformBuffer = nullptr;
  VkDescriptorSet descriptorSet = nullptr;
};