#pragma once

class JointWorld;
namespace graphics { class Workbench; }

class JointView
{
public:
    void configure(graphics::Workbench& app, JointWorld& world);

private:
    float grabDepth = 0.0f;
};
