#include "PickingView.hpp"
#include "PickingWorld.hpp"

#include <graphics/Primitives.hpp>
#include <graphics/Workbench.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui_internal.h>

#include <algorithm>

namespace
{
graphics::Transform transformOf(const physx::PxRigidDynamic& body)
{
    const auto pose = body.getGlobalPose();
    return {{pose.p.x, pose.p.y, pose.p.z}, {pose.q.w, pose.q.x, pose.q.y, pose.q.z}};
}

void updateGrab(PickingWorld& world, const graphics::Camera& camera, const glm::mat4& viewProjection,
                const glm::ivec4& viewport, float& depth)
{
    const auto& io = ImGui::GetIO();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || io.AppFocusLost)
    {
        if (world.selected()) world.releaseGrab();
        return;
    }

    const bool pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (!world.selected() && !pressed) return;
    if (io.WantCaptureMouse) return;
    const ImVec2 mouse = pressed ? io.MouseClickedPos[ImGuiMouseButton_Left] : io.MousePos;
    if (!ImGui::IsMousePosValid(&mouse)) return;
    const auto origin = ImGui::GetMainViewport()->Pos;
    const float x = (mouse.x - origin.x) * io.DisplayFramebufferScale.x;
    const float y = (io.DisplaySize.y - mouse.y + origin.y) * io.DisplayFramebufferScale.y;
    if (x < viewport.x || x >= viewport.x + viewport.z ||
        y <= viewport.y || y > viewport.y + viewport.w) return;

    const glm::vec2 ndc = 2.0f * (glm::vec2{x, y} - glm::vec2(viewport)) /
                          glm::vec2{viewport.z, viewport.w} - 1.0f;
    const auto nearPoint = glm::inverse(viewProjection) * glm::vec4{ndc, -1, 1};
    const auto direction = glm::normalize(glm::vec3(nearPoint) / nearPoint.w - camera.position);
    const auto forward = glm::normalize(camera.target - camera.position);
    if (pressed)
    {
        physx::PxVec3 hit;
        const auto& eye = camera.position;
        if (!world.grab({eye.x, eye.y, eye.z}, {direction.x, direction.y, direction.z}, hit)) return;
        depth = glm::dot(glm::vec3{hit.x, hit.y, hit.z} - eye, forward);
    }

    // Intersect the cursor ray with a screen-parallel plane through the grab point.
    // Only scrolling moves the plane towards or away from the camera.
    if (io.MouseWheel != 0.0f)
        depth = std::clamp(depth - io.MouseWheel * 0.5f, 1.0f, camera.farPlane - 1.0f);
    const auto target = camera.position + direction * (depth / glm::dot(direction, forward));
    world.moveGrab({target.x, target.y, target.z});
}
}

void PickingView::configure(graphics::Workbench& app, PickingWorld& world)
{
    app.camera = {{3.0f, 4.8f, 10.5f}, {0, 1, 0}};
    app.controls = []
    {
        ImGui::Spacing();
        ImGui::SeparatorText("Grab");
        ImGui::TextWrapped("Hold the left mouse button on a box or sphere to lift and drag it.");
        ImGui::TextWrapped("Scroll up to pull closer, down to push away. Release to let go.");
    };
    app.inspector = [this, &world]
    {
        ImGui::SeparatorText("Selection");
        const auto* actor = world.selected();
        ImGui::Text("Selected: %s", actor ? actor->getName() : "None");
        if (actor) ImGui::Text("Depth: %.2f m", grabDepth);
    };
    app.drawScene = [this, &app, &world,
                     boxMesh = graphics::MeshHandle{}, sphereMesh = graphics::MeshHandle{}]
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
        updateGrab(world, camera, viewProjection, viewport, grabDepth);

        const graphics::Material boxColor{{0.22f, 0.72f, 0.74f, 1}};
        const graphics::Material ballColor{{0.99f, 0.42f, 0.12f, 1}, true};
        const graphics::Material selectedBallColor{{1.0f, 0.85f, 0.22f, 1}, true};
        const graphics::Material selectedColor{{1.0f, 0.85f, 0.22f, 1}};
        renderer.drawMesh(boxMesh, {{0, -0.05f, 0}, {}, {25, 0.05f, 25}},
                          {{0.31f, 0.36f, 0.40f, 1}});
        for (const auto* body : world.boxes)
        {
            auto transform = transformOf(*body);
            transform.scale = glm::vec3{PickingWorld::halfExtent};
            renderer.drawMesh(boxMesh, transform,
                              body == world.selected() ? selectedColor : boxColor);
        }

        for (const auto* body : world.balls)
        {
            auto transform = transformOf(*body);
            transform.scale = glm::vec3{PickingWorld::ballRadius};
            renderer.drawMesh(sphereMesh, transform,
                              body == world.selected() ? selectedBallColor : ballColor);
        }
    };
}
