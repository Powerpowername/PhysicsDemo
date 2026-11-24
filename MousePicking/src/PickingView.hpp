#pragma once

class PickingWorld;
namespace graphics { class Workbench; }

class PickingView
{
public:
    void configure(graphics::Workbench& app, PickingWorld& world);

private:
    float grabDepth = 0.0f; // Distance along the camera's forward axis, in metres.
};
