#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include <vector>

class Buffer;
class Context;
class Headset;
class Pipeline;
class RenderProcess;

class Renderer final
{
public:
  Renderer();
  Renderer(const Context* context, const Headset* headset, MTLTexture_id tex_l, MTLTexture_id tex_r, uint32_t tex_w, uint32_t tex_h);
  ~Renderer();

  void render(size_t swapchainImageIndex);
  void submit(bool useSemaphores) const;

  bool isValid() const;
  VkCommandBuffer getCurrentCommandBuffer() const;
  VkSemaphore getCurrentDrawableSemaphore() const;
  VkSemaphore getCurrentPresentableSemaphore() const;
  void createTextureImage_L(const Context* context);
  void createTextureImageHax_L(const Context* context);
  void createTextureImageView_L(const Context* context);
  void createTextureSampler_L(const Context* context);

  void createTextureImage_R(const Context* context);
  void createTextureImageHax_R(const Context* context);
  void createTextureImageView_R(const Context* context);
  void createTextureSampler_R(const Context* context);

  VkCommandBuffer beginSingleTimeCommands(const Context* context);
  void endSingleTimeCommands(const Context* context, VkCommandBuffer commandBuffer);
  void transitionImageLayout(const Context* context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
  void copyBufferToImage(const Context* context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

  MTLTexture_id metal_tex_l;
  MTLTexture_id metal_tex_r;
  uint32_t metal_tex_w;
  uint32_t metal_tex_h;

private:
  bool valid = true;

  const Context* context = nullptr;
  const Headset* headset = nullptr;

  VkCommandPool commandPool = nullptr;
  VkDescriptorPool descriptorPool = nullptr;
  VkDescriptorSetLayout descriptorSetLayout = nullptr;
  std::vector<RenderProcess*> renderProcesses;
  VkPipelineLayout pipelineLayout = nullptr;
  Pipeline* rectPipeline = nullptr;
  Buffer *vertexBuffer = nullptr, *indexBuffer = nullptr;
  size_t currentRenderProcessIndex = 0u;

  VkImage textureImage_L;
  VkDeviceMemory textureImageMemory_L;
  VkImageView textureImageView_L;
  VkSampler textureSampler_L;

  VkImage textureImage_R;
  VkDeviceMemory textureImageMemory_R;
  VkImageView textureImageView_R;
  VkSampler textureSampler_R;
};