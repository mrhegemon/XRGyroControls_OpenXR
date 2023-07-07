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
  glm::vec2 uv;
};

constexpr std::array vertices = {
  // Cube front left
  Vertex{{ -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {0.0f, 0.0f}}, Vertex{{ -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {0.0f, 1.0f}},
  Vertex{{ +1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {1.0f, 0.0f}}, Vertex{{ +1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f}},
};

constexpr std::array<uint16_t, 6u> indices = { 0u,  1u,  2u,  1u,  2u,  3u};
} // namespace

Renderer::Renderer(){}

uint32_t findMemoryType(const Context* context, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  const VkPhysicalDevice vkPhysicalDevice = context->getVkPhysicalDevice();
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void createBuffer(const Context* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
  const VkDevice vkDevice = context->getVkDevice();

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
}

void createImage(const Context* context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
  const VkDevice vkDevice = context->getVkDevice();

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(vkDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(vkDevice, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(vkDevice, image, imageMemory, 0);
}

void createImageFromMetal(const Context* context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, MTLTexture_id tex_id) {
  const VkDevice vkDevice = context->getVkDevice();

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImportMetalTextureInfoEXT metalInfo{};
  metalInfo.sType = VK_STRUCTURE_TYPE_IMPORT_METAL_TEXTURE_INFO_EXT;
  metalInfo.pNext = nullptr;
  metalInfo.plane = VK_IMAGE_ASPECT_COLOR_BIT;
  metalInfo.mtlTexture = tex_id;

  imageInfo.pNext = &metalInfo;

  if (vkCreateImage(vkDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(vkDevice, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(vkDevice, image, imageMemory, 0);
}

VkCommandBuffer Renderer::beginSingleTimeCommands(const Context* context) {
    const VkDevice device = context->getVkDevice();
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Renderer::endSingleTimeCommands(const Context* context, VkCommandBuffer commandBuffer) {
  const VkDevice device = context->getVkDevice();

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(context->getVkDrawQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->getVkDrawQueue());

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Renderer::transitionImageLayout(const Context* context, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    const VkDevice device = context->getVkDevice();
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(context);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(context, commandBuffer);
}

void Renderer::copyBufferToImage(const Context* context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(context);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(context, commandBuffer);
}

void Renderer::createTextureImage_L(const Context* context) {
  const VkDevice vkDevice = context->getVkDevice();

  int texWidth = 32;
  int texHeight = 32;
  int texChannels = 4;
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    unsigned char* data_as_u8 = static_cast<unsigned char*>(data);
    for (int i = 0; i < texWidth * texHeight; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        unsigned char val = 0;
        if (j == 3) val = 0xFF;
        if (j == 0) val = i & 0xFF;

        if (i & 1 && j != 3) val = 0x0;

        data_as_u8[(i*4)+j] = val;
      }
    }
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createImage(context, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_L, textureImageMemory_L);

  transitionImageLayout(context, textureImage_L, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(context, stagingBuffer, textureImage_L, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  transitionImageLayout(context, textureImage_L, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void Renderer::createTextureImageHax_L(const Context* context) {
  const VkDevice vkDevice = context->getVkDevice();

  int texWidth = metal_tex_w;
  int texHeight = metal_tex_h;
  int texChannels = 4;
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    unsigned char* data_as_u8 = static_cast<unsigned char*>(data);
    for (int i = 0; i < texWidth * texHeight; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        unsigned char val = 0;
        if (j == 3) val = 0xFF;
        if (j == 0) val = i & 0xFF;

        if (i & 1 && j != 3) val = 0x0;

        data_as_u8[(i*4)+j] = val;
      }
    }
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createImageFromMetal(context, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_L, textureImageMemory_L, metal_tex_l);

  transitionImageLayout(context, textureImage_L, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(context, stagingBuffer, textureImage_L, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  transitionImageLayout(context, textureImage_L, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void Renderer::createTextureImage_R(const Context* context) {
  const VkDevice vkDevice = context->getVkDevice();

  int texWidth = metal_tex_w;
  int texHeight = metal_tex_h;
  int texChannels = 4;
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    unsigned char* data_as_u8 = static_cast<unsigned char*>(data);
    for (int i = 0; i < texWidth * texHeight; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        unsigned char val = 0;
        if (j == 3) val = 0xFF;
        if (j == 1) val = i & 0xFF;

        if (i & 1 && j != 3) val = 0x0;

        data_as_u8[(i*4)+j] = val;
      }
    }
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createImage(context, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_R, textureImageMemory_R);

  transitionImageLayout(context, textureImage_R, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(context, stagingBuffer, textureImage_R, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  transitionImageLayout(context, textureImage_R, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void Renderer::createTextureImageHax_R(const Context* context) {
  const VkDevice vkDevice = context->getVkDevice();

  int texWidth = 32;
  int texHeight = 32;
  int texChannels = 4;
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    unsigned char* data_as_u8 = static_cast<unsigned char*>(data);
    for (int i = 0; i < texWidth * texHeight; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        unsigned char val = 0;
        if (j == 3) val = 0xFF;
        if (j == 1) val = i & 0xFF;

        if (i & 1 && j != 3) val = 0x0;

        data_as_u8[(i*4)+j] = val;
      }
    }
  vkUnmapMemory(vkDevice, stagingBufferMemory);

  createImageFromMetal(context, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_R, textureImageMemory_R, metal_tex_r);

  transitionImageLayout(context, textureImage_R, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(context, stagingBuffer, textureImage_R, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  transitionImageLayout(context, textureImage_R, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
  vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

VkImageView createImageView(const Context* context, VkImage image, VkFormat format) {
  const VkDevice vkDevice = context->getVkDevice();
  VkImageViewCreateInfo viewInfo{};
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
  if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture image view!");
  }

  return imageView;
}

void Renderer::createTextureImageView_L(const Context* context) {
    textureImageView_L = createImageView(context, textureImage_L, VK_FORMAT_R8G8B8A8_SRGB);
}

void Renderer::createTextureImageView_R(const Context* context) {
    textureImageView_R = createImageView(context, textureImage_R, VK_FORMAT_R8G8B8A8_SRGB);
}

void Renderer::createTextureSampler_L(const Context* context) {
  const VkDevice vkDevice = context->getVkDevice();
  const VkPhysicalDevice vkPhysicalDevice = context->getVkPhysicalDevice();
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(vkPhysicalDevice, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &textureSampler_L) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void Renderer::createTextureSampler_R(const Context* context) {
  const VkDevice vkDevice = context->getVkDevice();
  const VkPhysicalDevice vkPhysicalDevice = context->getVkPhysicalDevice();
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(vkPhysicalDevice, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &textureSampler_R) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

Renderer::Renderer(const Context* context, const Headset* headset, MTLTexture_id tex_l, MTLTexture_id tex_r, uint32_t tex_w, uint32_t tex_h) : context(context), headset(headset)
{
  const VkPhysicalDevice vkPhysicalDevice = context->getVkPhysicalDevice();
  const VkDevice vkDevice = context->getVkDevice();
  metal_tex_l = tex_l;
  metal_tex_r = tex_r;
  metal_tex_w = tex_w;
  metal_tex_h = tex_h;

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

  createTextureImageHax_L(context);
  createTextureImageView_L(context);
  createTextureSampler_L(context);

  createTextureImageHax_R(context);
  createTextureImageView_R(context);
  createTextureSampler_R(context);

  std::array<VkDescriptorPoolSize, 3> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(numFramesInFlight);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(numFramesInFlight);
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[2].descriptorCount = static_cast<uint32_t>(numFramesInFlight);

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
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

  VkDescriptorSetLayoutBinding samplerLayoutBinding_L{};
  samplerLayoutBinding_L.binding = 1u;
  samplerLayoutBinding_L.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding_L.descriptorCount = 1u;
  samplerLayoutBinding_L.pImmutableSamplers = nullptr;
  samplerLayoutBinding_L.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding_R{};
  samplerLayoutBinding_R.binding = 2u;
  samplerLayoutBinding_R.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding_R.descriptorCount = 1u;
  samplerLayoutBinding_R.pImmutableSamplers = nullptr;
  samplerLayoutBinding_R.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 3> bindings = {descriptorSetLayoutBinding, samplerLayoutBinding_L, samplerLayoutBinding_R};

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();
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
    renderProcess = new RenderProcess(vkDevice, vkPhysicalDevice, commandPool, descriptorPool, descriptorSetLayout, textureImageView_L, textureSampler_L, textureImageView_R, textureSampler_R);
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

  VkVertexInputAttributeDescription vertexInputAttributeDescriptionUV;
  vertexInputAttributeDescriptionUV.binding = 0u;
  vertexInputAttributeDescriptionUV.location = 2u;
  vertexInputAttributeDescriptionUV.format = VK_FORMAT_R32G32_SFLOAT;
  vertexInputAttributeDescriptionUV.offset = offsetof(Vertex, uv);

  gridPipeline = new Pipeline(vkDevice, pipelineLayout, headset->getRenderPass(), "/Users/maxamillion/workspace/XRGyroControls_OpenXR_2/shaders/Basic.vert.spv",
                              "/Users/maxamillion/workspace/XRGyroControls_OpenXR_2/shaders/Grid.frag.spv", { vertexInputBindingDescription },
                              { vertexInputAttributeDescriptionPosition, vertexInputAttributeDescriptionColor, vertexInputAttributeDescriptionUV });
  if (!gridPipeline->isValid())
  {
    valid = false;
    return;
  }

  // Create the cube pipeline
  cubePipeline = new Pipeline(vkDevice, pipelineLayout, headset->getRenderPass(), "/Users/maxamillion/workspace/XRGyroControls_OpenXR_2/shaders/Basic.vert.spv",
                              "/Users/maxamillion/workspace/XRGyroControls_OpenXR_2/shaders/Cube.frag.spv", { vertexInputBindingDescription },
                              { vertexInputAttributeDescriptionPosition, vertexInputAttributeDescriptionColor, vertexInputAttributeDescriptionUV });
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
                                { vertexInputAttributeDescriptionPosition, vertexInputAttributeDescriptionColor, vertexInputAttributeDescriptionUV });
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

  vkDestroySampler(vkDevice, textureSampler_L, nullptr);
  vkDestroyImageView(vkDevice, textureImageView_L, nullptr);
  vkDestroyImage(vkDevice, textureImage_L, nullptr);
  vkFreeMemory(vkDevice, textureImageMemory_L, nullptr);

  vkDestroySampler(vkDevice, textureSampler_R, nullptr);
  vkDestroyImageView(vkDevice, textureImageView_R, nullptr);
  vkDestroyImage(vkDevice, textureImage_R, nullptr);
  vkFreeMemory(vkDevice, textureImageMemory_R, nullptr);

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

  const std::array clearValues = { VkClearValue({ 0.01f, 0.01f, 0.01f, 1.0f }), VkClearValue({ 1.0f, 0u }) };

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
  //gridPipeline->bind(commandBuffer);
  //vkCmdDrawIndexed(commandBuffer, 6u, 1u, 0u, 0u, 0u);

  // Draw the cube
  cubePipeline->bind(commandBuffer);
  vkCmdDrawIndexed(commandBuffer, 6u, 1u, 0u, 0u, 0u);

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