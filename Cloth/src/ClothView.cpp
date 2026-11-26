#include "ClothView.hpp"
#include "ClothWorld.hpp"

#include <graphics/Primitives.hpp>
#include <graphics/Workbench.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

namespace
{
graphics::MeshData unitRightTriangle()
{
    return {{{{0, 0, 0}, {0, 0, 1}},
             {{1, 0, 0}, {0, 0, 1}},
             {{0, 1, 0}, {0, 0, 1}}},
            {0, 1, 2}};
}

// A rigid transform and a non-uniform scale can draw any right triangle. Split the
// cloth triangle at the foot of its altitude so both halves use the same static mesh.
void drawTriangle(graphics::SceneRenderer& renderer, graphics::MeshHandle mesh,
                  glm::vec3 a, glm::vec3 b, glm::vec3 c, const graphics::Material& material)
{
    // The altitude meets the longest side inside the triangle (including its ends).
    const glm::vec3 corners[] = {a, b, c};
    const float sides[] = {glm::distance(a, b), glm::distance(b, c), glm::distance(c, a)};
    int base = sides[1] > sides[0] ? 1 : 0;
    if (sides[2] > sides[base]) base = 2;
    a = corners[base];
    b = corners[(base + 1) % 3];
    c = corners[(base + 2) % 3];

    const float width = sides[base];
    if (width < 1e-6f) return;
    const glm::vec3 x = (b - a) / width;
    const float left = glm::dot(c - a, x);
    const glm::vec3 foot = a + left * x;
    const float height = glm::distance(c, foot);
    if (height < 1e-6f) return;
    const glm::vec3 y = (c - foot) / height;
    const auto rotation = glm::quat_cast(glm::mat3{x, y, glm::cross(x, y)});
    if (left > 1e-6f)
        renderer.drawMesh(mesh, {foot, rotation, {-left, height, 1}}, material);
    if (width - left > 1e-6f)
        renderer.drawMesh(mesh, {foot, rotation, {width - left, height, 1}}, material);
}
}

void configureClothView(graphics::Workbench& app, ClothWorld& world)
{
    app.camera = {{2.0f, 2.7f, 5.1f}, {0.0f, 1.9f, 0.15f}};
    app.controls = [&world]
    {
        ImGui::SeparatorText("Wind");
        bool enabled = world.windEnabled();
        if (ImGui::Checkbox("Enable wind", &enabled)) world.setWindEnabled(enabled);
        ImGui::TextWrapped("The top edge is pinned. Turn off the wind to let the cloth settle.");
    };
    app.drawScene = [&world, mesh = graphics::MeshHandle{}, rail = graphics::MeshHandle{}](graphics::SceneRenderer& renderer) mutable
    {
        if (!mesh) mesh = renderer.createMesh(unitRightTriangle());
        if (!rail) rail = renderer.createMesh(graphics::primitives::box());
        const auto& leftPin = world.position(0, 0);
        const auto& rightPin = world.position(world.columns() - 1, 0);
        graphics::Transform support;
        support.position = (leftPin + rightPin) * 0.5f;
        support.scale = {0.5f * (rightPin.x - leftPin.x) + 0.07f, 0.035f, 0.04f};
        renderer.drawMesh(rail, support, {{0.65f, 0.45f, 0.28f, 1.0f}});
        const graphics::Material clothColors[] = {
            {{0.43f, 0.68f, 0.67f, 1.0f}},
            {{0.69f, 0.79f, 0.70f, 1.0f}},
        };
        for (std::size_t row = 0; row + 1 < world.rows(); ++row)
        {
            for (std::size_t column = 0; column + 1 < world.columns(); ++column)
            {
                const auto color = clothColors[(column / 3 + row / 3) % 2];
                const auto p00 = world.position(column, row);
                const auto p10 = world.position(column + 1, row);
                const auto p01 = world.position(column, row + 1);
                const auto p11 = world.position(column + 1, row + 1);
                drawTriangle(renderer, mesh, p00, p01, p11, color);
                drawTriangle(renderer, mesh, p00, p11, p10, color);
            }
        }
    };
}
