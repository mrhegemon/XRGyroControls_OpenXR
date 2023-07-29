#pragma once

#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

class Buffer;

class RenderProcess final
{
public:
  RenderProcess(VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkCommandPool commandPool);
  ~RenderProcess();

  bool isValid() const;
  VkCommandBuffer getCommandBuffer() const;
  VkSemaphore getDrawableSemaphore() const;
  VkSemaphore getPresentableSemaphore() const;
  VkFence getBusyFence() const;

private:
  bool valid = true;

  VkDevice device = nullptr;
  VkCommandBuffer commandBuffer = nullptr;
  VkSemaphore drawableSemaphore = nullptr, presentableSemaphore = nullptr;
  VkFence busyFence = nullptr;
};