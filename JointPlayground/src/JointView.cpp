#include "JointView.hpp"
#include "JointWorld.hpp"

#include <graphics/Primitives.hpp>
#include <graphics/Workbench.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>

namespace
{
graphics::Transform transformOf(const physx::PxRigidDynamic& body)
{
    const auto pose = body.getGlobalPose();
    return {{pose.p.x, pose.p.y, pose.p.z}, {pose.q.w, pose.q.x, pose.q.y, pose.q.z}};
}

struct Ray { glm::vec3 origin; glm::vec3 direction; };

bool makeRay(const ImVec2& mouse, const graphics::Camera& camera,
             const glm::ivec4& viewport, Ray& ray)
{
    if (viewport.z <= 0 || viewport.w <= 0) return false;
    const auto* main = ImGui::GetMainViewport();
    const auto& io = ImGui::GetIO();
    const float x = (mouse.x - main->Pos.x) * io.DisplayFramebufferScale.x;
    const float y = (io.DisplaySize.y - mouse.y + main->Pos.y) * io.DisplayFramebufferScale.y;
    if (x < viewport.x || x >= viewport.x + viewport.z || y < viewport.y || y >= viewport.y + viewport.w)
        return false;
    const glm::vec2 ndc = 2.0f * (glm::vec2{x, y} - glm::vec2{viewport}) /
                          glm::vec2{viewport.z, viewport.w} - 1.0f;
    const auto projection = glm::perspective(glm::radians(camera.fieldOfView),
                                             static_cast<float>(viewport.z) / viewport.w,
                                             camera.nearPlane, camera.farPlane);
    const auto inverse = glm::inverse(projection * glm::lookAt(camera.position, camera.target, camera.up));
    const auto nearPoint = inverse * glm::vec4{ndc, -1.0f, 1.0f};
    const glm::vec3 nearPosition = glm::vec3{nearPoint} / nearPoint.w;
    const glm::vec3 direction = nearPosition - camera.position;
    const float length = glm::length(direction);
    if (!std::isfinite(length) || length <= 0.0f) return false;
    ray = {camera.position, direction / length};
    return true;
}

graphics::Transform lineBetween(glm::vec3 from, glm::vec3 to)
{
    const glm::vec3 delta = to - from;
    const float length = glm::length(delta);
    if (length <= 1.0e-5f) return {from, {}, {0.02f, 0.02f, 0.02f}};
    return {(from + to) * 0.5f,
            glm::rotation(glm::vec3{0, 1, 0}, delta / length),
            {0.02f, length * 0.5f, 0.02f}};
}

void updateDrag(JointWorld& world, const graphics::Camera& camera,
                const glm::ivec4& viewport, float& depth)
{
    const auto& io = ImGui::GetIO();
    if (io.AppFocusLost || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        if (world.activeDemo()) world.endDrag();
        return;
    }
    const bool pressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (!world.activeDemo() && !pressed) return;
    if (pressed && io.WantCaptureMouse) return;
    const ImVec2 mouse = pressed ? io.MouseClickedPos[ImGuiMouseButton_Left] : io.MousePos;
    if (!ImGui::IsMousePosValid(&mouse)) return;
    Ray ray;
    if (!makeRay(mouse, camera, viewport, ray)) return;
    const glm::vec3 forward = glm::normalize(camera.target - camera.position);
    const float rayForward = glm::dot(ray.direction, forward);
    if (rayForward <= 1.0e-4f) return;
    if (pressed)
    {
        JointWorld::DemoId id;
        physx::PxVec3 hit;
        if (!world.beginDrag({ray.origin.x, ray.origin.y, ray.origin.z},
                             {ray.direction.x, ray.direction.y, ray.direction.z}, id, hit))
            return;
        depth = glm::dot(glm::vec3{hit.x, hit.y, hit.z} - ray.origin, forward);
    }
    if (!world.activeDemo()) return;
    if (io.MouseWheel != 0.0f)
        depth = std::clamp(depth - io.MouseWheel * 0.5f, camera.nearPlane + 0.05f, camera.farPlane - 1.0f);
    const float distance = depth / rayForward;
    const glm::vec3 target = ray.origin + ray.direction * distance;
    world.moveDrag(*world.activeDemo(), {target.x, target.y, target.z});
}
}

void JointView::configure(graphics::Workbench& app, JointWorld& world)
{
    app.camera = {{0.0f, 7.0f, 18.0f}, {0.0f, 2.2f, 0.0f}};
    app.controls = [&world]
    {
        ImGui::SeparatorText("Six independent joint demonstrations");
        ImGui::TextWrapped("Drag an orange driver. Each driver controls only the blue body in its own row.");
        for (const auto& demo : world.demos())
            ImGui::BulletText("%s: %s", demo.name, demo.control);
    };
    app.inspector = [&world]
    {
        ImGui::SeparatorText("Joint values");
        for (const auto& demo : world.demos())
            ImGui::Text("%-10s % .3f", demo.name, world.jointValue(demo));
    };
    app.drawScene = [&app, &world, this, boxMesh = graphics::MeshHandle{}, sphereMesh = graphics::MeshHandle{}]
                    (graphics::SceneRenderer& renderer) mutable
    {
        if (!boxMesh) boxMesh = renderer.createMesh(graphics::primitives::box());
        if (!sphereMesh) sphereMesh = renderer.createMesh(graphics::primitives::sphere());
        glm::ivec4 viewport;
        glGetIntegerv(GL_VIEWPORT, &viewport.x);
        updateDrag(world, app.camera, viewport, grabDepth);
        renderer.drawMesh(boxMesh, {{0, -0.05f, 0}, {}, {18, 0.05f, 8}}, {{0.31f, 0.36f, 0.40f, 1}});
        for (const auto& demo : world.demos())
        {
            auto driver = transformOf(*demo.driver);
            driver.scale = glm::vec3{JointWorld::halfExtent};
            auto body = transformOf(*demo.body);
            body.scale = glm::vec3{JointWorld::halfExtent};
            const glm::vec3 driverPosition = driver.position;
            const glm::vec3 bodyPosition = body.position;
            renderer.drawMesh(boxMesh, driver,
                              demo.dragging ? graphics::Material{{1.0f, 0.90f, 0.15f, 1}, true}
                                            : graphics::Material{{1.0f, 0.40f, 0.08f, 1}, true});
            renderer.drawMesh(boxMesh, body, {{0.16f, 0.68f, 0.78f, 1}, true});
            renderer.drawMesh(sphereMesh, {driverPosition, {}, {0.11f, 0.11f, 0.11f}}, {{0.95f, 0.85f, 0.20f, 1}, true});
            renderer.drawMesh(boxMesh, lineBetween(driverPosition, bodyPosition), {{0.95f, 0.78f, 0.20f, 1}, true});
        }
    };
}
