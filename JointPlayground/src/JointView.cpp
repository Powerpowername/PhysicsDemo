#include "JointView.hpp"
#include "JointWorld.hpp"

#include <graphics/Primitives.hpp>
#include <graphics/Workbench.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>

namespace
{
using Kind = JointWorld::Kind;

glm::vec3 vectorOf(const physx::PxVec3& v) { return {v.x, v.y, v.z}; }

graphics::Transform transformOf(const physx::PxRigidDynamic& actor, glm::vec3 halfExtents)
{
    const auto pose = actor.getGlobalPose();
    return {vectorOf(pose.p), {pose.q.w, pose.q.x, pose.q.y, pose.q.z}, halfExtents};
}

// Both pins use the same capsule, scaled by their PhysX radius along all three axes.
graphics::MeshData axleMesh()
{
    graphics::MeshData mesh;
    constexpr int slices = 24;
    constexpr int capRings = 6;
    for (int side : {-1, 1})
    {
        for (int ring = 0; ring <= capRings; ++ring)
        {
            const float latitude = glm::half_pi<float>() * (static_cast<float>(ring) / capRings - (side < 0 ? 1 : 0));
            for (int i = 0; i < slices; ++i)
            {
                const float angle = glm::two_pi<float>() * i / slices;
                const glm::vec3 normal{std::sin(latitude), std::cos(latitude) * std::cos(angle),
                                       std::cos(latitude) * std::sin(angle)};
                mesh.vertices.push_back({normal + glm::vec3{side * JointWorld::shaftHalfLengthInRadii, 0, 0}, normal});
            }
        }
    }
    const auto ringCount = 2 * (capRings + 1);
    for (int r = 0; r < ringCount - 1; ++r)
        for (int i = 0; i < slices; ++i)
        {
            const auto a = static_cast<std::uint32_t>(r * slices + i);
            const auto b = static_cast<std::uint32_t>(r * slices + (i + 1) % slices);
            if (r > 0) mesh.indices.insert(mesh.indices.end(), {a, b, a + slices});
            if (r < ringCount - 2) mesh.indices.insert(mesh.indices.end(), {b, b + slices, a + slices});
        }
    return mesh;
}

// The scene occupies the center dock only. Labels and picking must use that same rectangle.
struct SceneProjection
{
    ImRect rect;
    glm::mat4 viewProjection;

    glm::vec3 rayDirection(ImVec2 mouse, glm::vec3 cameraPosition) const
    {
        const float x = 2 * (mouse.x - rect.Min.x) / rect.GetWidth() - 1;
        const float y = 1 - 2 * (mouse.y - rect.Min.y) / rect.GetHeight();
        const auto near = glm::inverse(viewProjection) * glm::vec4{x, y, -1, 1};
        return glm::normalize(glm::vec3{near} / near.w - cameraPosition);
    }

    void label(glm::vec3 position, const char* name) const
    {
        const auto clip = viewProjection * glm::vec4{position, 1};
        if (clip.w <= 0 || std::abs(clip.z) > clip.w) return;
        const glm::vec2 ndc = glm::vec2{clip} / clip.w;
        ImVec2 text{rect.Min.x + (ndc.x + 1) * rect.GetWidth() * 0.5f,
                    rect.Min.y + (1 - ndc.y) * rect.GetHeight() * 0.5f};
        text.x -= ImGui::CalcTextSize(name).x * 0.5f;
        ImGui::GetBackgroundDrawList()->AddText(text, IM_COL32(230, 237, 242, 255), name);
    }
};

SceneProjection sceneProjection(const graphics::Camera& camera)
{
    glm::ivec4 viewport;
    glGetIntegerv(GL_VIEWPORT, &viewport.x);
    const auto* main = ImGui::GetMainViewport();
    const auto scale = ImGui::GetIO().DisplayFramebufferScale;
    const ImVec2 min{main->Pos.x + viewport.x / scale.x,
                     main->Pos.y + main->Size.y - (viewport.y + viewport.w) / scale.y};
    const ImVec2 max{min.x + viewport.z / scale.x, min.y + viewport.w / scale.y};
    const auto projection = glm::perspective(glm::radians(camera.fieldOfView),
        static_cast<float>(viewport.z) / viewport.w, camera.nearPlane, camera.farPlane);
    return {{min, max}, projection * glm::lookAt(camera.position, camera.target, camera.up)};
}

void updateDrag(JointWorld& world, const graphics::Camera& camera, const SceneProjection& scene, float& depth)
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
    if (!ImGui::IsMousePosValid(&mouse) || !scene.rect.Contains(mouse)) return;

    const glm::vec3 direction = scene.rayDirection(mouse, camera.position);
    const glm::vec3 forward = glm::normalize(camera.target - camera.position);
    if (pressed)
    {
        JointWorld::DemoId id;
        physx::PxVec3 hit;
        const auto eye = camera.position;
        if (!world.beginDrag({eye.x, eye.y, eye.z}, {direction.x, direction.y, direction.z}, id, hit)) return;
        depth = glm::dot(vectorOf(hit) - eye, forward);
    }
    if (!world.activeDemo()) return;
    depth = std::clamp(depth - io.MouseWheel * 0.5f, camera.nearPlane + 0.05f, camera.farPlane - 1.0f);
    const glm::vec3 target = camera.position + direction * (depth / glm::dot(direction, forward));
    world.moveDrag(*world.activeDemo(), {target.x, target.y, target.z});
}
}

void JointView::configure(graphics::Workbench& app, JointWorld& world)
{
    app.camera = {{0, 6.2f, 13.2f}, {0, 3.65f, 0}};
    app.controls = []
    {
        ImGui::SeparatorText("Mouse");
        ImGui::TextWrapped("Hold and drag an orange part. Watch how the blue part follows.");
        ImGui::Spacing();
        ImGui::TextWrapped("Scroll while holding to move in depth.");
    };
    app.inspector = [&world]
    {
        ImGui::SeparatorText("Joint motion");
        for (const auto& demo : world.demos())
        {
            ImGui::TextUnformatted(demo.name);
            ImGui::TextDisabled("%s", demo.control);
            const float value = world.jointValue(demo);
            if (demo.kind == Kind::D6)
                ImGui::Text("X: %.2f m | Twist: %.0f deg", value,
                            glm::degrees(demo.joint->is<physx::PxD6Joint>()->getTwistAngle()));
            else if (demo.kind == Kind::Distance || demo.kind == Kind::Prismatic)
                ImGui::Text("%.2f m", value);
            else if (demo.kind == Kind::Spherical || demo.kind == Kind::Revolute)
                ImGui::Text("%.0f deg", glm::degrees(value));
            ImGui::Spacing();
        }
    };
    app.drawScene = [&app, &world, this, box = graphics::MeshHandle{}, axle = graphics::MeshHandle{}]
                    (graphics::SceneRenderer& renderer) mutable
    {
        if (!box) box = renderer.createMesh(graphics::primitives::box());
        if (!axle) axle = renderer.createMesh(axleMesh());
        const auto scene = sceneProjection(app.camera);
        updateDrag(world, app.camera, scene, grabDepth);
        renderer.drawMesh(box, {{0, -0.05f, 5}, {}, {18, 0.05f, 7}}, {{0.19f, 0.23f, 0.27f, 1}});
        auto* labels = ImGui::GetBackgroundDrawList();
        labels->PushClipRect(scene.rect.Min, scene.rect.Max, true);
        for (const auto& demo : world.demos())
        {
            const bool pin = demo.kind == Kind::Revolute || demo.kind == Kind::D6;
            const auto driver = transformOf(*demo.driver, pin ? glm::vec3{demo.driverHalfExtents.y}
                                                               : vectorOf(demo.driverHalfExtents));
            const auto body = transformOf(*demo.body, vectorOf(demo.bodyHalfExtents));
            const graphics::Material orange{demo.dragging ? glm::vec4{1, 0.72f, 0.22f, 1}
                                                          : glm::vec4{1, 0.51f, 0.17f, 1}};
            renderer.drawMesh(pin ? axle : box, driver, orange);
            renderer.drawMesh(box, body, {{0.19f, 0.74f, 0.82f, 1}});
            if (demo.kind == Kind::Distance)
            {
                // Only this joint spans a gap: its two anchors are at the part centers.
                const glm::vec3 delta = body.position - driver.position;
                const float length = glm::length(delta);
                if (length > 0.001f)
                    renderer.drawMesh(box, {(driver.position + body.position) * 0.5f,
                        glm::rotation(glm::vec3{0, 1, 0}, delta / length), {0.017f, length * 0.5f, 0.017f}},
                        {{0.85f, 0.87f, 0.89f, 1}});
            }
            scene.label(driver.position + glm::vec3{0, 1.12f, 0}, demo.name);
        }
        labels->PopClipRect();
    };
}
