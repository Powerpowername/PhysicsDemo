#include "RigidBodyView.hpp"
#include "PhysicsWorld.hpp"

#include <graphics/Primitives.hpp>
#include <graphics/Workbench.hpp>
#include <imgui.h>

void configureRigidBodyView(graphics::Workbench& app, const PhysicsWorld& world)
{
    app.camera = {{5.0f, 4.0f, 8.0f}, {0.0f, 1.0f, 0.0f}};
    app.controls = []
    {
        ImGui::Spacing();
        ImGui::SeparatorText("Scene");
        ImGui::TextUnformatted("Box on a plane");
        ImGui::TextDisabled("Gravity   -9.81 m/s^2");
        ImGui::TextDisabled("Release    3.00 m");
    };
    app.inspector = [&world]
    {
        ImGui::SeparatorText("Body");
        ImGui::Text("Time       %.2f s", world.time);
        ImGui::Text("Height     %.3f m", world.box->getGlobalPose().p.y);
        ImGui::Text("Speed      %.3f m/s", world.box->getLinearVelocity().magnitude());
        ImGui::Spacing();
        ImGui::SeparatorText("Last contact");
        const ContactReport& hit = world.callback.report();
        if (hit.count)
        {
            ImGui::Text("%s  /  %s", hit.shapeA, hit.shapeB);
            ImGui::Text("At %.3f s   |   %u points", hit.time, hit.pointCount);
            ImGui::Spacing();
            const auto& p = hit.point;
            ImGui::TextDisabled("Position (m)");
            ImGui::Text("%.3f, %.3f, %.3f", p.position.x, p.position.y, p.position.z);
            ImGui::TextDisabled("Normal");
            ImGui::Text("%.2f, %.2f, %.2f", p.normal.x, p.normal.y, p.normal.z);
            const float impulse = p.impulse.magnitude();
            ImGui::TextDisabled("Impulse / step-average force");
            ImGui::Text("%.2f N s   /   %.2f N", impulse, impulse / fixedStep);
        }
        else
        {
            ImGui::TextDisabled("Waiting for impact");
        }
    };
    app.drawScene = [&world, boxMesh = graphics::MeshHandle{}](graphics::SceneRenderer& renderer) mutable
    {
        if (!boxMesh) boxMesh = renderer.createMesh(graphics::primitives::box());
        const auto pose = world.box->getGlobalPose();
        renderer.drawMesh(boxMesh, {{0, -0.05f, 0}, {}, {5, 0.05f, 5}},
                          {{0.31f, 0.36f, 0.40f, 1}});
        renderer.drawMesh(boxMesh,
                          {{pose.p.x, pose.p.y, pose.p.z},
                           {pose.q.w, pose.q.x, pose.q.y, pose.q.z},
                           {0.5f, 0.5f, 0.5f}},
                          {{1.0f, 0.54f, 0.23f, 1}});
    };
}
