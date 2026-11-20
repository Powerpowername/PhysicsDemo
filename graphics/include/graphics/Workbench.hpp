#pragma once

#include <graphics/SceneRenderer.hpp>

#include <functional>
#include <string>

namespace graphics
{
class Workbench
{
public:
    Workbench(std::string title, float fixedStep);

    Camera camera;
    std::function<void()> step;  // Required: advance the simulation by one fixed step.
    std::function<void()> reset;
    std::function<void()> controls;
    std::function<void()> inspector;
    std::function<void(SceneRenderer&)> drawScene;

    // Owns the window until it closes. Captured state must outlive this call.
    void run() const;

private:
    std::string title_;
    float fixedStep_;
};
}
