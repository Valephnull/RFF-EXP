//
// Copies the final post-process image between the renderer's ping-pong targets.
//

#pragma once

#include "vulkan_helper/engine/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"

namespace merutilm::rff2 {
    struct GPCImageCopy final : public vkh::GeneralPostProcessGraphicsPipelineConfigurator {
        static constexpr uint32_t SET_SOURCE = 0;
        static constexpr uint32_t BINDING_SOURCE_SAMPLER = 0;

        explicit GPCImageCopy(vkh::Engine &engine, vkh::WindowContext &wc) :
            GeneralPostProcessGraphicsPipelineConfigurator(engine, wc, "vk_image_copy.frag") {}

        void updateQueue(vkh::DescriptorUpdateQueue &, uint32_t) override {}
        void pipelineInitialized() override {}
        void renderContextRefreshed() override;

    protected:
        void configurePushConstant(vkh::PipelineLayoutManager &) override {}
        void configureDescriptors(std::vector<vkh::Descriptor *> &descriptors) override;
    };
} // namespace merutilm::rff2
