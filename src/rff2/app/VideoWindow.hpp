//
// Created by Merutilm on 2025-09-06.
//

#pragma once


#include "../settings/Settings.h"
#include "RFF2.hpp"
#include "VideoBufferCache.hpp"
#include "VideoWindowRenderManager.hpp"
#include "opencv2/freetype.hpp"

namespace merutilm::rff2 {


    class VideoWindow final : public vkh::Handler{

        RFF2 &app;
        std::unique_ptr<VideoWindowRenderManager> scene = nullptr;
        const int width;
        const int height;
        bool ownsWindowContext = false;
        std::filesystem::path inputDirectory;
        std::filesystem::path outputFile;
        Settings targetSettings;
        uint32_t imageWidth = 0;
        uint32_t imageHeight = 0;
        uint32_t dynamicFrameCount = 0;


    public:
        explicit VideoWindow(RFF2 &app, int width, int height);

        ~VideoWindow() override;

        VideoWindow(const VideoWindow &) = delete;

        VideoWindow &operator=(const VideoWindow &) = delete;

        VideoWindow(VideoWindow &&) = delete;

        VideoWindow &operator=(VideoWindow &&) = delete;


        static void createVideo(RFF2 &app, const std::filesystem::path &open,
                                const std::filesystem::path &save, const Settings &settingsClone);

        [[nodiscard]] static std::shared_ptr<VideoWindow> prepareVideo(
                RFF2 &app, const std::filesystem::path &open,
                const std::filesystem::path &save, const Settings &settingsClone);

        void renderVideo();

        static cv::Mat generateFrame(const VideoBufferCache &buffer, int imgWidth,
                                     const VidAnimationSettings &animation,
                                     const cv::Ptr<cv::freetype::FreeType2> &systemFont = {});

    protected:
        void initScene(const VkExtent2D &videoExtent, const Settings &targetSettings);

        void init() override;

        void cleanup() override;
    };
} // namespace merutilm::rff2
