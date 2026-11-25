#include "ArmView.hpp"
#include "ArmWorld.hpp"

#include <graphics/Primitives.hpp>
#include <graphics/Workbench.hpp>
#include <imgui.h>

#include <iterator>

namespace
{
constexpr const char* jointNames[] = {"Shoulder", "Elbow"};
const graphics::Material linkColors[] = {{{0.20f, 0.75f, 0.80f, 1}}, {{1.0f, 0.52f, 0.18f, 1}}};

graphics::Transform transformOf(const physx::PxArticulationLink& link, const physx::PxVec3& halfExtents)
{
    const auto pose = link.getGlobalPose();
    return {{pose.p.x, pose.p.y, pose.p.z}, {pose.q.w, pose.q.x, pose.q.y, pose.q.z},
            {halfExtents.x, halfExtents.y, halfExtents.z}};
}
}

void configureArmView(graphics::Workbench& app, ArmWorld& world)
{
    app.camera = {{2.3f, 3.4f, 7.1f}, {0.9f, 2.25f, 0}};
    app.controls = [&world]
    {
        ImGui::SeparatorText("Arm controls");
        ImGui::Checkbox("Auto motion", &world.autoMotion);
        const float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        const bool reach = ImGui::Button("Reach", {buttonWidth, 0});
        ImGui::SameLine();
        const bool fold = ImGui::Button("Fold", {buttonWidth, 0});
        if (reach || fold)
        {
            const auto& pose = reach ? ArmWorld::reachAngles : ArmWorld::foldAngles;
            for (std::size_t i = 0; i < pose.size(); ++i)
                world.setTarget(i, pose[i]);
        }

        ImGui::TextWrapped("Click a pose or drag an angle to take control.");
        for (std::size_t i = 0; i < std::size(jointNames); ++i)
        {
            float target = world.target(i);
            ImGui::SetNextItemWidth(-75);
            if (ImGui::SliderAngle(jointNames[i], &target, glm::degrees(ArmWorld::minAngle[i]),
                                   glm::degrees(ArmWorld::maxAngle[i]), "%.0f deg", ImGuiSliderFlags_AlwaysClamp))
                world.setTarget(i, target);
            // Hold the current targets as soon as the user grabs a moving slider.
            if (ImGui::IsItemActivated()) world.autoMotion = false;
        }
    };
    app.inspector = [&world]
    {
        ImGui::SeparatorText("Measured angles");
        for (std::size_t i = 0; i < std::size(jointNames); ++i)
            ImGui::Text("%s: %.1f deg", jointNames[i], glm::degrees(world.angle(i)));
    };
    app.drawScene = [&world, box = graphics::MeshHandle{}](graphics::SceneRenderer& renderer) mutable
    {
        if (!box) box = renderer.createMesh(graphics::primitives::box());
        renderer.drawMesh(box, transformOf(world.base(), ArmWorld::baseHalfExtents), {{0.47f, 0.53f, 0.59f, 1}});
        for (std::size_t i = 0; i < std::size(jointNames); ++i)
            renderer.drawMesh(box, transformOf(world.link(i), ArmWorld::linkHalfExtents[i]), linkColors[i]);
    };
}
