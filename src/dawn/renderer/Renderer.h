/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.me.uk)
 */
#pragma once

#include "core/Timer.h"
#include "renderer/rhi/RHIRenderer.h"

namespace dw {
class DW_API Renderer : public Module {
public:
    DW_OBJECT(Renderer);

    Renderer(Context* ctx);
    ~Renderer() = default;

    /// Render a single frame.
    bool frame();

    /// Get the renderer hardware interface.
    rhi::RHIRenderer* rhi() const;

    /// Access the last frame time
    double frameTime() const;

    /// Access the frames per second metric.
    int framesPerSecond() const;

private:
    UniquePtr<rhi::RHIRenderer> rhi_;

    double frame_time_;
    int frames_per_second_;
    int frame_counter_;
    time::TimePoint last_fps_update_;
};
}  // namespace dw
