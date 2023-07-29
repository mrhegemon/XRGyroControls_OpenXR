#include "RenderProcess.h"

#include "Buffer.h"
#include "Util.h"

RenderProcess::RenderProcess(VkDevice device,
                             VkPhysicalDevice physicalDevice,
                             VkCommandPool commandPool)
: device(device)
{

  // Allocate a command buffer
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocateInfo.commandBufferCount = 1u;
  if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  // Create semaphores
  VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
  if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &drawableSemaphore) != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentableSemaphore) != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  // Create a memory fence
  VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Make sure the fence starts off signaled
  if (vkCreateFence(device, &fenceCreateInfo, nullptr, &busyFence) != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }
}

RenderProcess::~RenderProcess()
{
  vkDestroyFence(device, busyFence, nullptr);
}

bool RenderProcess::isValid() const
{
  return valid;
}

VkCommandBuffer RenderProcess::getCommandBuffer() const
{
  return commandBuffer;
}

VkSemaphore RenderProcess::getDrawableSemaphore() const
{
  return drawableSemaphore;
}

VkSemaphore RenderProcess::getPresentableSemaphore() const
{
  return presentableSemaphore;
}

VkFence RenderProcess::getBusyFence() const
{
  return busyFence;
}