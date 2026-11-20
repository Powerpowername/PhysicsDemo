#include <graphics/Workbench.hpp>
#include <graphics/Window.hpp>
#include <imgui_internal.h> // DockBuilder is used only to create or restore the layout.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <utility>

namespace graphics
{
namespace
{
constexpr char controlsPanel[] = "Simulation";
constexpr char inspectorPanel[] = "Inspector";

// A simulation or UI callback may throw; close its panel before Window unwinds.
struct Panel
{
    explicit Panel(const char* title) : visible(ImGui::Begin(title)) {}
    ~Panel() { ImGui::End(); }
    bool visible;
};

ImRect dockspace(Window& window, const char* title, ImGuiID root, bool hasInspector)
{
    bool restore = false;
    if (ImGui::BeginMainMenuBar())
    {
        ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextLink), "%s", title);
        ImGui::Separator();
        if (ImGui::BeginMenu("Workspace"))
        {
            restore = ImGui::MenuItem("Restore default layout");
            if (ImGui::MenuItem("Exit")) window.requestClose();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    const auto* viewport = ImGui::GetMainViewport();
    constexpr auto flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingOverCentralNode;
    if (restore || !ImGui::DockBuilderGetNode(root))
    {
        if (restore) ImGui::DockBuilderRemoveNode(root);
        ImGui::DockBuilderAddNode(root, ImGuiDockNodeFlags_DockSpace | flags);
        ImGui::DockBuilderSetNodeSize(root, viewport->WorkSize);
        ImGuiID center = root;
        const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
        ImGui::DockBuilderDockWindow(controlsPanel, left);
        if (hasInspector)
        {
            const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.29f, nullptr, &center);
            ImGui::DockBuilderDockWindow(inspectorPanel, right);
        }
        ImGui::DockBuilderFinish(root);
    }
    ImGui::DockSpaceOverViewport(root, viewport, flags);
    const auto* scene = ImGui::DockBuilderGetCentralNode(root);
    return scene ? scene->Rect() : ImRect(0, 0, 0, 0);
}

void drawViewport(const Workbench& app, SceneRenderer& renderer, const ImRect& rect, glm::ivec2 framebuffer)
{
    // Dock coordinates start at the top left; OpenGL expects framebuffer pixels from the bottom left.
    const auto* viewport = ImGui::GetMainViewport();
    const auto pixel = [&](ImVec2 point) -> glm::ivec2
    {
        return {std::lround((point.x - viewport->Pos.x) * framebuffer.x / viewport->Size.x),
                std::lround((point.y - viewport->Pos.y) * framebuffer.y / viewport->Size.y)};
    };
    const glm::ivec2 topLeft = pixel(rect.Min);
    const glm::ivec2 bottomRight = pixel(rect.Max);
    const glm::ivec2 size = bottomRight - topLeft;
    if (size.x <= 0 || size.y <= 0) return;
    renderer.beginScene(app.camera, size, {topLeft.x, framebuffer.y - bottomRight.y});
    app.drawScene(renderer);
    renderer.endScene();
}
}

Workbench::Workbench(std::string title, float fixedStep) : title_(std::move(title)), fixedStep_(fixedStep)
{
    if (title_.empty()) throw std::invalid_argument("Workbench requires a title");
    if (!std::isfinite(fixedStep_) || fixedStep_ <= 0.0f)
        throw std::invalid_argument("Workbench requires a positive, finite time step in seconds");
}

void Workbench::run() const
{
    if (!step) throw std::logic_error("Workbench requires a step callback");
    const ImGuiID root = ImHashData(title_.data(), title_.size());
    // ImGui reads this pointer again when the Window destroys the context.
    const std::string iniFile = "imgui-" + std::to_string(root) + ".ini";
    Window window(title_.c_str());
    ImGui::GetIO().IniFilename = iniFile.c_str();
    std::optional<SceneRenderer> renderer;
    if (drawScene) renderer.emplace();

    char stepLabel[40];
    std::snprintf(stepLabel, sizeof(stepLabel), "Step  %.4g s", fixedStep_);
    bool paused = false;
    double accumulator = 0;
    while (window.beginFrame())
    {
        const ImRect scene = dockspace(window, title_.c_str(), root, static_cast<bool>(inspector));
        if (Panel playback{controlsPanel}; playback.visible)
        {
            ImGui::SeparatorText("Playback");
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextLink), "%s", paused ? "Paused" : "Running");
            if (ImGui::Button(paused ? "Resume" : "Pause", {-1, 0}))
            {
                paused = !paused;
                accumulator = 0;
            }
            ImGui::BeginDisabled(!paused);
            const bool singleStep = ImGui::Button(stepLabel, {-1, 0});
            ImGui::EndDisabled();
            if (singleStep) step();
            if (reset && ImGui::Button("Reset", {-1, 0}))
            {
                reset();
                accumulator = 0;
            }
            if (controls) controls();
        }

        if (!paused)
        {
            accumulator += std::min(window.deltaTime(), 0.25f);
            while (accumulator >= fixedStep_)
            {
                step();
                accumulator -= fixedStep_;
            }
        }
        if (inspector)
        {
            if (Panel panel{inspectorPanel}; panel.visible) inspector();
        }
        if (renderer) drawViewport(*this, *renderer, scene, window.framebufferSize());
        window.endFrame();
    }
}
}
