#include "Renderer.h"

#include "Buffer.h"
#include "Context.h"
#include "Headset.h"
#include "Pipeline.h"
#include "RenderProcess.h"
#include "RenderTarget.h"
#include "Util.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <array>

namespace
{
constexpr size_t numFramesInFlight = 2u;

struct Vertex final
{
  glm::vec3 position;
  glm::vec3 color;
};

constexpr std::array vertices = {
  // Grid
  Vertex{{ -20.0f, 0.0f, -20.0f }, { 1.0f, 0.0f, 0.0f }}, Vertex{{ -20.0f, 0.0f, +20.0f }, { 0.0f, 1.0f, 0.0f }},
  Vertex{{ +20.0f, 0.0f, -20.0f }, { 0.0f, 0.0f, 1.0f }}, Vertex{{ +20.0f, 0.0f, +20.0f }, { 1.0f, 0.0f, 1.0f }},

  // Cube front left
  Vertex{{ -1.0f, 0.0f, -3.0f }, { 0.0f, 0.0f, 1.0f }}, Vertex{{ -1.0f, 1.4f, -3.0f }, { 0.0f, 0.0f, 1.0f }},
  Vertex{{ +0.0f, 0.0f, -2.0f }, { 0.0f, 0.0f, 1.0f }}, Vertex{{ +0.0f, 1.4f, -2.0f }, { 0.0f, 0.0f, 1.0f }},

  // Cube front right
  Vertex{{ +0.0f, 0.0f, -2.0f }, { 0.6f, 0.0f, 0.0f }}, Vertex{{ +0.0f, 1.4f, -2.0f }, { 0.6f, 0.0f, 0.0f }},
  Vertex{{ +1.0f, 0.0f, -3.0f }, { 0.6f, 0.0f, 0.0f }}, Vertex{{ +1.0f, 1.4f, -3.0f }, { 0.6f, 0.0f, 0.0f }},

  // Cube back left
  Vertex{{ -1.0f, 0.0f, -3.0f }, { 0.4f, 0.4f, 0.4f }}, Vertex{{ -1.0f, 1.4f, -3.0f }, { 0.4f, 0.4f, 0.4f }},
  Vertex{{ +0.0f, 0.0f, -4.0f }, { 0.4f, 0.4f, 0.4f }}, Vertex{{ +0.0f, 1.4f, -4.0f }, { 0.4f, 0.4f, 0.4f }},

  // Cube back right
  Vertex{{ +0.0f, 0.0f, -4.0f }, { 0.2f, 0.2f, 0.2f }}, Vertex{{ +0.0f, 1.4f, -4.0f }, { 0.2f, 0.2f, 0.2f }},
  Vertex{{ +1.0f, 0.0f, -3.0f }, { 0.2f, 0.2f, 0.2f }}, Vertex{{ +1.0f, 1.4f, -3.0f }, { 0.2f, 0.2f, 0.2f }},

  // Cube top
  Vertex{{ -1.0f, 1.4f, -3.0f }, { 0.0f, 1.0f, 0.0f }}, Vertex{{ +0.0f, 1.4f, -4.0f }, { 0.0f, 1.0f, 0.0f }},
  Vertex{{ +1.0f, 1.4f, -3.0f }, { 0.0f, 1.0f, 0.0f }}, Vertex{{ +0.0f, 1.4f, -2.0f }, { 0.0f, 1.0f, 0.0f }},

  // Cube bottom
  Vertex{{ -1.0f, 0.0f, -3.0f }, { 0.0f, 0.2f, 0.0f }}, Vertex{{ +0.0f, 0.0f, -4.0f }, { 0.0f, 0.2f, 0.0f }},
  Vertex{{ +1.0f, 0.0f, -3.0f }, { 0.0f, 0.2f, 0.0f }}, Vertex{{ +0.0f, 0.0f, -2.0f }, { 0.0f, 0.2f, 0.0f }}
};

constexpr std::array<uint16_t, 42u> indices = { 0u,  1u,  2u,  1u,  2u,  3u,  4u,  5u,  6u,  5u,  6u,  7u,
                                                8u,  9u,  10u, 9u,  10u, 11u, 12u, 13u, 14u, 13u, 14u, 15u,
                                                16u, 17u, 18u, 17u, 18u, 19u, 20u, 21u, 22u, 20u, 22u, 23u,
                                                24u, 25u, 26u, 24u, 26u, 27u };
} // namespace

Renderer::Renderer(){}

Renderer::Renderer(const Context* context, const Headset* headset) : context(context), headset(headset)
{
  const VkPhysicalDevice vkPhysicalDevice = context->getVkPhysicalDevice();
  const VkDevice vkDevice = context->getVkDevice();

  // Create a command pool
  VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolCreateInfo.queueFamilyIndex = context->getVkDrawQueueFamilyIndex();
  if (vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  // Create a descriptor pool
  VkDescriptorPoolSize descriptorPoolSize;
  descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSize.descriptorCount = 16u;

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
  descriptorPoolCreateInfo.poolSizeCount = 16u;
  descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
  descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(numFramesInFlight);
  if (vkCreateDescriptorPool(vkDevice, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  // Create a descriptor set layout
  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
  descriptorSetLayoutBinding.binding = 0u;
  descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorSetLayoutBinding.descriptorCount = 1u;
  descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  descriptorSetLayoutCreateInfo.bindingCount = 1u;
  descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;
  if (vkCreateDescriptorSetLayout(vkDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) !=
      VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  // Create a pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutCreateInfo.setLayoutCount = 1u;
  if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    util::error(Error::GenericVulkan);
    valid = false;
    return;
  }

  // Create a render process for each frame in flight
  renderProcesses.resize(numFramesInFlight);
  for (RenderProcess*& renderProcess : renderProcesses)
  {
    renderProcess = new RenderProcess(vkDevice, vkPhysicalDevice, commandPool, descriptorPool, descriptorSetLayout);
    if (!renderProcess->isValid())
    {
      valid = false;
      return;
    }
  }

  // Create the grid pipeline
  VkVertexInputBindingDescription vertexInputBindingDescription;
  vertexInputBindingDescription.binding = 0u;
  vertexInputBindingDescription.stride = sizeof(Vertex);
  vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription vertexInputAttributeDescriptionPosition;
  vertexInputAttributeDescriptionPosition.binding = 0u;
  vertexInputAttributeDescriptionPosition.location = 0u;
  vertexInputAttributeDescriptionPosition.format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescriptionPosition.offset = offsetof(Vertex, position);

  VkVertexInputAttributeDescription vertexInputAttributeDescriptionColor;
  vertexInputAttributeDescriptionColor.binding = 0u;
  vertexInputAttributeDescriptionColor.location = 1u;
  vertexInputAttributeDescriptionColor.format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescriptionColor.offset = offsetof(Vertex, color);

  gridPipeline = new Pipeline(vkDevice, pipelineLayout, headset->getRenderPass(), "/Users/maxamillion/workspace/XRGyroControls_OpenXR/shaders/Basic.vert.spv",
                              "/Users/maxamillion/workspace/XRGyroControls_OpenXR/shaders/Grid.frag.spv", { vertexInputBindingDescription },
                              { vertexInputAttributeDescriptionPosition, vertexInputAttributeDescriptionColor });
  if (!gridPipeline->isValid())
  {
    valid = false;
    return;
  }

  // Create the cube pipeline
  cubePipeline = new Pipeline(vkDevice, pipelineLayout, headset->getRenderPass(), "/Users/maxamillion/workspace/XRGyroControls_OpenXR/shaders/Basic.vert.spv",
                              "/Users/maxamillion/workspace/XRGyroControls_OpenXR/shaders/Cube.frag.spv", { vertexInputBindingDescription },
                              { vertexInputAttributeDescriptionPosition, vertexInputAttributeDescriptionColor });
  if (!cubePipeline->isValid())
  {
    valid = false;
    return;
  }

  /*for (int i = 0; i < 64; i++) {
    char tmp[256];
    snprintf(tmp, 256, "/Users/maxamillion/workspace/XRGyroControls_OpenXR/shaders/Pt%u.vert.spv", i);
    // Create the cube pipeline
    trackedPipeline[i] = new Pipeline(vkDevice, pipelineLayout, headset->getRenderPass(), tmp,
                                "/Users/maxamillion/workspace/XRGyroControls_OpenXR/shaders/Cube.frag.spv", { vertexInputBindingDescription },
                                { vertexInputAttributeDescriptionPosition, vertexInputAttributeDescriptionColor });
    if (!trackedPipeline[i]->isValid())
    {
      valid = false;
      return;
    }
  }*/

  // Create a vertex buffer
  {
    // Create a staging buffer and fill it with the vertex data
    constexpr size_t size = sizeof(vertices);
    Buffer* stagingBuffer = new Buffer(vkDevice, vkPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       static_cast<VkDeviceSize>(size), static_cast<const void*>(vertices.data()));
    if (!stagingBuffer->isValid())
    {
      valid = false;
      return;
    }

    // Create an empty target buffer
    vertexBuffer =
      new Buffer(vkDevice, vkPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, static_cast<VkDeviceSize>(size));
    if (!vertexBuffer->isValid())
    {
      valid = false;
      return;
    }

    // Copy from the staging to the target buffer
    if (!stagingBuffer->copyTo(*vertexBuffer, renderProcesses.at(0u)->getCommandBuffer(), context->getVkDrawQueue()))
    {
      valid = false;
      return;
    }

    // Clean up the staging buffer
    delete stagingBuffer;
  }

  // Create an index buffer
  {
    // Create a staging buffer and fill it with the index data
    constexpr size_t size = sizeof(indices);
    Buffer* stagingBuffer = new Buffer(vkDevice, vkPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       static_cast<VkDeviceSize>(size), static_cast<const void*>(indices.data()));
    if (!stagingBuffer->isValid())
    {
      valid = false;
      return;
    }

    // Create an empty target buffer
    indexBuffer =
      new Buffer(vkDevice, vkPhysicalDevice, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, static_cast<VkDeviceSize>(size));
    if (!indexBuffer->isValid())
    {
      valid = false;
      return;
    }

    // Copy from the staging to the target buffer
    if (!stagingBuffer->copyTo(*indexBuffer, renderProcesses.at(0u)->getCommandBuffer(), context->getVkDrawQueue()))
    {
      valid = false;
      return;
    }

    // Clean up the staging buffer
    delete stagingBuffer;
  }
}

Renderer::~Renderer()
{
  delete indexBuffer;
  delete vertexBuffer;
  delete cubePipeline;
  delete gridPipeline;

  const VkDevice vkDevice = context->getVkDevice();

  vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, nullptr);
  vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);

  for (const RenderProcess* renderProcess : renderProcesses)
  {
    delete renderProcess;
  }

  vkDestroyCommandPool(vkDevice, commandPool, nullptr);
}

void Renderer::render(size_t swapchainImageIndex)
{
  currentRenderProcessIndex = (currentRenderProcessIndex + 1u) % renderProcesses.size();

  RenderProcess* renderProcess = renderProcesses.at(currentRenderProcessIndex);

  const VkFence busyFence = renderProcess->getBusyFence();
  if (vkResetFences(context->getVkDevice(), 1u, &busyFence) != VK_SUCCESS)
  {
    return;
  }

  const VkCommandBuffer commandBuffer = renderProcess->getCommandBuffer();

  if (vkResetCommandBuffer(commandBuffer, 0u) != VK_SUCCESS)
  {
    return;
  }

  VkCommandBufferBeginInfo commandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
  {
    return;
  }

  // Update the uniform buffer data
  float handScaleAll1 = 0.1;
  float handScaleAll2 = 0.01;
  glm::vec3 handScale1 = glm::vec3(handScaleAll1 * (1.0 / 2.0), handScaleAll1 * (1.0 / 2.0), handScaleAll1 * (1.0 / 2.0));
  glm::vec3 handScale2 = glm::vec3(handScaleAll2 * (1.0 / 2.0), handScaleAll2 * (1.0 / 2.0), handScaleAll2 * (1.0 / 2.0));


  /*for (int i = 0; i < 64; i++)
  {
    glm::mat4 trans1 = glm::translate(glm::mat4(1.0f), { 0.0f, -1.4f/2.0, 2.0f });
    glm::mat4 scale1 = glm::scale(glm::mat4(1.0f), i < 2 ? handScale1 : handScale2);
    glm::mat4 trans2_l = glm::translate(glm::mat4(1.0f), { headset->tracked_locations[i].pose.position.x, headset->tracked_locations[i].pose.position.y, headset->tracked_locations[i].pose.position.z });
    glm::quat rot_l_q = glm::quat(headset->tracked_locations[i].pose.orientation.w, headset->tracked_locations[i].pose.orientation.x, headset->tracked_locations[i].pose.orientation.y, headset->tracked_locations[i].pose.orientation.z);
    glm::mat4 rot_l = glm::toMat4(rot_l_q);
    glm::mat4 realMat_l = trans2_l * rot_l * scale1 * trans1;

    renderProcess->uniformBufferData.tracked_points[i] = realMat_l;
  }*/

  renderProcess->uniformBufferData.world = glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, 0.0f });
  for (size_t eyeIndex = 0u; eyeIndex < headset->getEyeCount(); ++eyeIndex)
  {
    renderProcess->uniformBufferData.viewProjection[eyeIndex] =
      headset->getEyeProjectionMatrix(eyeIndex) * headset->getEyeViewMatrix(eyeIndex);
  }

  if (!renderProcess->updateUniformBufferData())
  {
    return;
  }

  const std::array clearValues = { VkClearValue({ 1.00f, 0.01f, 0.01f, 1.0f }), VkClearValue({ 1.0f, 0u }) };

  VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
  renderPassBeginInfo.renderPass = headset->getRenderPass();
  renderPassBeginInfo.framebuffer = headset->getRenderTarget(swapchainImageIndex)->getFramebuffer();
  renderPassBeginInfo.renderArea.offset = { 0, 0 };
  renderPassBeginInfo.renderArea.extent = headset->getEyeResolution(0u);
  renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassBeginInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  // Set the viewport
  VkViewport viewport;
  viewport.x = static_cast<float>(renderPassBeginInfo.renderArea.offset.x);
  viewport.y = static_cast<float>(renderPassBeginInfo.renderArea.offset.y);
  viewport.width = static_cast<float>(renderPassBeginInfo.renderArea.extent.width);
  viewport.height = static_cast<float>(renderPassBeginInfo.renderArea.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0u, 1u, &viewport);

  // Set the scissor
  VkRect2D scissor;
  scissor.offset = renderPassBeginInfo.renderArea.offset;
  scissor.extent = renderPassBeginInfo.renderArea.extent;
  vkCmdSetScissor(commandBuffer, 0u, 1u, &scissor);

  // Bind the vertex buffer
  const VkDeviceSize offset = 0u;
  const VkBuffer buffer = vertexBuffer->getVkBuffer();
  vkCmdBindVertexBuffers(commandBuffer, 0u, 1u, &buffer, &offset);

  // Bind the index buffer
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getVkBuffer(), 0u, VK_INDEX_TYPE_UINT16);

  // Bind the uniform buffer
  const VkDescriptorSet descriptorSet = renderProcess->getDescriptorSet();
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, 1u, &descriptorSet, 0u,
                          nullptr);

  // Draw the grid
  gridPipeline->bind(commandBuffer);
  vkCmdDrawIndexed(commandBuffer, 6u, 1u, 0u, 0u, 0u);

  // Draw the cube
  cubePipeline->bind(commandBuffer);
  vkCmdDrawIndexed(commandBuffer, 36u, 1u, 6u, 0u, 0u);

  // Draw the lhand cube
  /*for (int i = 0; i < 64; i++) {
    trackedPipeline[i]->bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, 36u, 1u, 6u, 0u, 0u);
  }*/

  vkCmdEndRenderPass(commandBuffer);
}

void Renderer::submit(bool useSemaphores) const
{
  const RenderProcess* renderProcess = renderProcesses.at(currentRenderProcessIndex);
  const VkCommandBuffer commandBuffer = renderProcess->getCommandBuffer();
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    return;
  }

  const VkPipelineStageFlags waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  const VkSemaphore drawableSemaphore = renderProcess->getDrawableSemaphore();
  const VkSemaphore presentableSemaphore = renderProcess->getPresentableSemaphore();
  const VkFence busyFence = renderProcess->getBusyFence();

  VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
  submitInfo.pWaitDstStageMask = &waitStages;
  submitInfo.commandBufferCount = 1u;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (useSemaphores)
  {
    submitInfo.waitSemaphoreCount = 1u;
    submitInfo.pWaitSemaphores = &drawableSemaphore;
    submitInfo.signalSemaphoreCount = 1u;
    submitInfo.pSignalSemaphores = &presentableSemaphore;
  }

  if (vkQueueSubmit(context->getVkDrawQueue(), 1u, &submitInfo, busyFence) != VK_SUCCESS)
  {
    return;
  }
}

bool Renderer::isValid() const
{
  return valid;
}

VkCommandBuffer Renderer::getCurrentCommandBuffer() const
{
  return renderProcesses.at(currentRenderProcessIndex)->getCommandBuffer();
}

VkSemaphore Renderer::getCurrentDrawableSemaphore() const
{
  return renderProcesses.at(currentRenderProcessIndex)->getDrawableSemaphore();
}

VkSemaphore Renderer::getCurrentPresentableSemaphore() const
{
  return renderProcesses.at(currentRenderProcessIndex)->getPresentableSemaphore();
}