#include "GPCImageCopy.hpp"

#include "SharedImageContextIndices.hpp"
#include "vulkan_helper/engine/repo/GlobalSamplerRepo.hpp"

namespace merutilm::rff2 {
    void GPCImageCopy::renderContextRefreshed() {
        auto &sourceDesc = getDescriptor(SET_SOURCE);
        sourceDesc.get<vkh::CombinedImageSampler>(0, BINDING_SOURCE_SAMPLER)
                .setImageContextMF(wc.getSharedImageContext().getImageContextMF(
                        SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY));
        writeDescriptorMF([&sourceDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            sourceDesc.queue(queue, frameIndex, {}, {BINDING_SOURCE_SAMPLER});
        });
    }

    void GPCImageCopy::configureDescriptors(std::vector<vkh::Descriptor *> &descriptors) {
        vkh::Sampler &sampler = pickFromGlobalRepository<vkh::GlobalSamplerRepo, vkh::Sampler &>(
                VkSamplerCreateInfo{
                        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                        .magFilter = VK_FILTER_NEAREST,
                        .minFilter = VK_FILTER_NEAREST,
                        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                        .compareOp = VK_COMPARE_OP_ALWAYS,
                        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                });
        auto manager = vkh::DescriptorManager();
        manager.appendCombinedImgSampler(
                BINDING_SOURCE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                std::make_unique<vkh::CombinedImageSampler>(wc.core, sampler, true));
        appendUniqueDescriptor(SET_SOURCE, descriptors, std::move(manager));
    }
} // namespace merutilm::rff2
