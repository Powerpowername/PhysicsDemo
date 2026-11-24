#include "BoxStackView.hpp"
#include "StackWorld.hpp"

#include <graphics/Primitives.hpp>
#include <graphics/Workbench.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_internal.h>


namespace
{
graphics::Transform transformOf(const physx::PxRigidDynamic& body)
{
    const auto pose = body.getGlobalPose();
    return {{pose.p.x, pose.p.y, pose.p.z}, {pose.q.w, pose.q.x, pose.q.y, pose.q.z}};
}

void launchOnClick(StackWorld& world, const graphics::Camera& camera,
                   const glm::mat4& viewProjection, const glm::ivec4& viewport)
{
    const auto& io = ImGui::GetIO();
    if (io.WantCaptureMouse || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    // Convert the press to framebuffer pixels, with the origin at the bottom left.
    const auto mouse = io.MouseClickedPos[ImGuiMouseButton_Left];
    const auto origin = ImGui::GetMainViewport()->Pos;
    const float x = (mouse.x - origin.x) * io.DisplayFramebufferScale.x;
    const float y = (io.DisplaySize.y - (mouse.y - origin.y)) * io.DisplayFramebufferScale.y;
    if (x < viewport.x || x >= viewport.x + viewport.z ||
        y <= viewport.y || y > viewport.y + viewport.w) return;

    const glm::vec2 ndc = 2.0f * (glm::vec2{x, y} - glm::vec2(viewport)) /
                          glm::vec2{viewport.z, viewport.w} - 1.0f;
    const glm::vec4 nearPoint = glm::inverse(viewProjection) * glm::vec4(ndc, -1.0f, 1.0f);
    const glm::vec3 direction = glm::normalize(glm::vec3(nearPoint) / nearPoint.w - camera.position);
    // Start just beyond the camera so the near plane doesn't cut through the ball.
    const glm::vec3 start = camera.position + direction * (2.0f * StackWorld::ballRadius);
    world.launch({start.x, start.y, start.z}, {direction.x, direction.y, direction.z});
}
}

void configureBoxStackView(graphics::Workbench& app, StackWorld& world)
{
    app.camera = {{3.0f, 4.0f, 11.0f}, {0, 2.2f, 0}};
    app.controls = [&world]
    {
        ImGui::Spacing();
        ImGui::SeparatorText("Launch speed");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##speed", &world.launchSpeed, 8.0f, 40.0f, "%.1f m/s", ImGuiSliderFlags_AlwaysClamp);
        ImGui::TextWrapped("Left-click in the scene to launch a ball.");
    };
    app.inspector = [&world]
    {
        ImGui::SeparatorText("Scene");
        ImGui::Text("Boxes  %zu", world.boxes.size());
        ImGui::Text("Balls  %zu", world.balls.size());
    };

    app.drawScene = [&app, &world, boxMesh = graphics::MeshHandle{}, sphereMesh = graphics::MeshHandle{}]
                    (graphics::SceneRenderer& renderer) mutable
    {
        if (!boxMesh) boxMesh = renderer.createMesh(graphics::primitives::box());
        if (!sphereMesh) sphereMesh = renderer.createMesh(graphics::primitives::sphere());
        glm::ivec4 viewport;
        glGetIntegerv(GL_VIEWPORT, &viewport.x);
        const auto& camera = app.camera;
        const float aspect = static_cast<float>(viewport.z) / viewport.w;
        const auto viewProjection = glm::perspective(glm::radians(camera.fieldOfView), aspect,
                                                      camera.nearPlane, camera.farPlane)
                                  * glm::lookAt(camera.position, camera.target, camera.up);
        launchOnClick(world, camera, viewProjection, viewport);

        renderer.drawMesh(boxMesh, {{0, -0.05f, 0}, {}, {20, 0.05f, 20}},
                          {{0.32f, 0.37f, 0.42f, 1}});
        const graphics::Material colors[] = {{{0.22f, 0.68f, 0.73f, 1}}, {{0.35f, 0.82f, 0.79f, 1}}};
        for (std::size_t i = 0; i < world.boxes.size(); ++i)
        {
            auto transform = transformOf(*world.boxes[i]);
            transform.scale = glm::vec3{StackWorld::halfExtent};
            renderer.drawMesh(boxMesh, transform, colors[i % 2]);
        }

        if (world.balls.empty()) return;
        const graphics::Material ballColor{{1.0f, 0.42f, 0.08f, 1.0f}, true};
        for (const auto* body : world.balls)
        {
            auto pose = transformOf(*body);
            pose.scale = glm::vec3{StackWorld::ballRadius};
            renderer.drawMesh(sphereMesh, pose, ballColor);
        }
    };
}
