#include "RenderProcess.h"

#include "Buffer.h"
#include "Util.h"

RenderProcess::RenderProcess(VkDevice device,
                             VkPhysicalDevice physicalDevice,
                             VkCommandPool commandPool,
                             VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout descriptorSetLayout,
                             VkImageView textureImageView,
                             VkSampler textureSampler,
                             VkImageView textureImageView2,
                             VkSampler textureSampler2)
: device(device)
{
  // Initialize the uniform buffer data
  uniformBufferData.world = glm::mat4(1.0f);
  /*for (int i = 0; i < 64; i++)
  {
    uniformBufferData.tracked_points[i] = glm::mat4(1.0f);
  }*/
  uniformBufferData.viewProjection[0] = glm::mat4(1.0f);
  uniformBufferData.viewProjection[1] = glm::mat4(1.0f);

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

  // Create an empty uniform buffer
  constexpr VkDeviceSize uniformBufferSize = static_cast<VkDeviceSize>(sizeof(UniformBufferData));
  uniformBuffer =
    new Buffer(device, physicalDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferSize);
  if (!uniformBuffer->isValid())
  {
    valid = false;
    return;
  }

  // Allocate a descriptor set
  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
  descriptorSetAllocateInfo.descriptorPool = descriptorPool;
  descriptorSetAllocateInfo.descriptorSetCount = 1u;
  descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
  const VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet);
  if (result != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  this->UpdateImages(textureImageView, textureSampler, textureImageView2, textureSampler2);
}

void RenderProcess::UpdateImages(VkImageView textureImageView,
                             VkSampler textureSampler,
                             VkImageView textureImageView2,
                             VkSampler textureSampler2)
{
  // Associate the descriptor set with the uniform buffer
  VkDescriptorBufferInfo descriptorBufferInfo;
  descriptorBufferInfo.buffer = uniformBuffer->getVkBuffer();
  descriptorBufferInfo.offset = 0u;
  descriptorBufferInfo.range = VK_WHOLE_SIZE;

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = textureImageView;
  imageInfo.sampler = textureSampler;

  VkDescriptorImageInfo imageInfo2{};
  imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo2.imageView = textureImageView2;
  imageInfo2.sampler = textureSampler2;

  VkWriteDescriptorSet writeDescriptorSet[3];
  writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet[0].pNext = nullptr;
  writeDescriptorSet[0].dstSet = descriptorSet;
  writeDescriptorSet[0].dstBinding = 0u;
  writeDescriptorSet[0].dstArrayElement = 0u;
  writeDescriptorSet[0].descriptorCount = 1u;
  writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeDescriptorSet[0].pBufferInfo = &descriptorBufferInfo;
  writeDescriptorSet[0].pImageInfo = nullptr;
  writeDescriptorSet[0].pTexelBufferView = nullptr;

  writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet[1].pNext = nullptr;
  writeDescriptorSet[1].dstSet = descriptorSet;
  writeDescriptorSet[1].dstBinding = 1;
  writeDescriptorSet[1].dstArrayElement = 0;
  writeDescriptorSet[1].descriptorCount = 1u;
  writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet[1].pBufferInfo = nullptr;
  writeDescriptorSet[1].pImageInfo = &imageInfo;
  writeDescriptorSet[1].pTexelBufferView = nullptr;

  writeDescriptorSet[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet[2].pNext = nullptr;
  writeDescriptorSet[2].dstSet = descriptorSet;
  writeDescriptorSet[2].dstBinding = 2;
  writeDescriptorSet[2].dstArrayElement = 0;
  writeDescriptorSet[2].descriptorCount = 1u;
  writeDescriptorSet[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet[2].pBufferInfo = nullptr;
  writeDescriptorSet[2].pImageInfo = &imageInfo2;
  writeDescriptorSet[2].pTexelBufferView = nullptr;

  vkUpdateDescriptorSets(device, 3u, writeDescriptorSet, 0u, nullptr);
}

RenderProcess::~RenderProcess()
{
  delete uniformBuffer;

  vkDestroyFence(device, busyFence, nullptr);
  vkDestroySemaphore(device, presentableSemaphore, nullptr);
  vkDestroySemaphore(device, drawableSemaphore, nullptr);
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

VkDescriptorSet RenderProcess::getDescriptorSet() const
{
  return descriptorSet;
}

bool RenderProcess::updateUniformBufferData() const
{
  void* data = uniformBuffer->map();
  if (!data)
  {
    return false;
  }

  memcpy(data, &uniformBufferData, sizeof(UniformBufferData));
  uniformBuffer->unmap();

  return true;
}