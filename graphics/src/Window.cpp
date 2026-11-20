#include <graphics/Window.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <initializer_list>
#include <stdexcept>
#include <string>

namespace graphics
{
namespace
{
void applyWorkbenchStyle()
{
    auto& style = ImGui::GetStyle();
    style.FontSizeBase = 16.0f;
    style.WindowPadding = {12, 10};
    style.FramePadding = {8, 4};
    style.ItemSpacing = {8, 6};
    style.WindowRounding = style.PopupRounding = 5;
    style.FrameRounding = style.GrabRounding = style.TabRounding = 3;
    style.DockingSeparatorSize = 3;
    style.TabBarOverlineSize = 2;

    auto paint = [&](ImU32 color, std::initializer_list<ImGuiCol> roles)
    {
        for (ImGuiCol role : roles)
            style.Colors[role] = ImGui::ColorConvertU32ToFloat4(color);
    };

    // Graphite surfaces, from the recessed workspace to raised controls.
    paint(IM_COL32(20, 24, 28, 255), {ImGuiCol_DockingEmptyBg, ImGuiCol_TitleBg,
        ImGuiCol_TitleBgCollapsed, ImGuiCol_TabDimmed, ImGuiCol_ScrollbarBg});
    paint(IM_COL32(28, 33, 38, 255), {ImGuiCol_WindowBg, ImGuiCol_MenuBarBg, ImGuiCol_Tab});
    paint(IM_COL32(37, 44, 50, 255), {ImGuiCol_PopupBg, ImGuiCol_FrameBg, ImGuiCol_TitleBgActive,
        ImGuiCol_TabDimmedSelected, ImGuiCol_TableHeaderBg, ImGuiCol_TableBorderLight});
    paint(IM_COL32(54, 63, 69, 255), {ImGuiCol_Border, ImGuiCol_Separator, ImGuiCol_ResizeGrip,
        ImGuiCol_TreeLines, ImGuiCol_ScrollbarGrab, ImGuiCol_TableBorderStrong});
    paint(IM_COL32(231, 237, 240, 255), {ImGuiCol_Text, ImGuiCol_InputTextCursor});
    paint(IM_COL32(155, 169, 177, 255), {ImGuiCol_TextDisabled});

    // Dark teal carries text; bright teal is reserved for marks and outlines.
    paint(IM_COL32(39, 59, 63, 255), {ImGuiCol_Button, ImGuiCol_Header, ImGuiCol_TabSelected});
    paint(IM_COL32(43, 74, 76, 255), {ImGuiCol_ButtonHovered, ImGuiCol_HeaderHovered,
        ImGuiCol_TabHovered, ImGuiCol_FrameBgHovered, ImGuiCol_ScrollbarGrabHovered});
    paint(IM_COL32(34, 92, 88, 255), {ImGuiCol_ButtonActive, ImGuiCol_HeaderActive,
        ImGuiCol_FrameBgActive, ImGuiCol_CheckboxSelectedBg, ImGuiCol_TextSelectedBg,
        ImGuiCol_TabDimmedSelectedOverline, ImGuiCol_ScrollbarGrabActive});
    paint(IM_COL32(91, 208, 190, 255), {ImGuiCol_CheckMark, ImGuiCol_SliderGrab, ImGuiCol_TextLink,
        ImGuiCol_TabSelectedOverline, ImGuiCol_SeparatorHovered, ImGuiCol_ResizeGripHovered,
        ImGuiCol_NavCursor, ImGuiCol_DragDropTarget, ImGuiCol_PlotLines, ImGuiCol_PlotHistogram});
    paint(IM_COL32(148, 232, 216, 255), {ImGuiCol_SeparatorActive, ImGuiCol_ResizeGripActive,
        ImGuiCol_SliderGrabActive, ImGuiCol_PlotLinesHovered, ImGuiCol_PlotHistogramHovered});
    paint(IM_COL32(91, 208, 190, 90), {ImGuiCol_DockingPreview});
}

std::runtime_error glfwError(const char* operation)
{
    const char* detail = nullptr;
    glfwGetError(&detail);
    return std::runtime_error(std::string(operation) + ": " + (detail ? detail : "unknown GLFW error"));
}

// Only a press accepted by the scene can become a scene hold or release.
// In particular, releasing Enter after leaving a text box isn't a scene event.
ButtonState nextButton(bool wasDown, bool held, bool pressed)
{
    return {held && (wasDown || pressed), pressed, wasDown && !held};
}
}

ButtonState Input::key(ImGuiKey key) const
{
    if (key < ImGuiKey_Tab || key >= ImGuiKey_GamepadStart)
        return {};
    return keys_[key - ImGuiKey_Tab];
}

ButtonState Input::mouse(ImGuiMouseButton button) const
{
    if (button < 0 || button >= ImGuiMouseButton_COUNT)
        return {};
    return buttons_[button];
}

void Input::update(bool focused)
{
    const ImGuiIO& io = ImGui::GetIO();
    const bool keyboard = focused && !io.WantCaptureKeyboard;
    const bool mouse = focused && !io.WantCaptureMouse;
    for (int i = ImGuiKey_Tab; i < ImGuiKey_GamepadStart; ++i)
    {
        const auto key = static_cast<ImGuiKey>(i);
        auto& state = keys_[i - ImGuiKey_Tab];
        state = keyboard ? nextButton(state.down, ImGui::IsKeyDown(key), ImGui::IsKeyPressed(key, false)) : ButtonState{};
    }
    for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
    {
        auto& state = buttons_[i];
        state = mouse ? nextButton(state.down, ImGui::IsMouseDown(i), ImGui::IsMouseClicked(i)) : ButtonState{};
    }

    if (ImGui::IsMousePosValid())
        mousePosition = {io.MousePos.x, io.MousePos.y};
    mouseDelta = mouse ? glm::vec2{io.MouseDelta.x, io.MouseDelta.y} : glm::vec2{0.0f};
    scrollDelta = mouse ? glm::vec2{io.MouseWheelH, io.MouseWheel} : glm::vec2{0.0f};
}

struct Window::Impl
{
    GLFWwindow* window = nullptr;
    ImGuiContext* context = nullptr;
    bool glfwReady = false;
    bool frameOpen = false;

    ~Impl()
    {
        if (context)
        {
            glfwMakeContextCurrent(window);
            ImGui::SetCurrentContext(context);
            if (frameOpen)
                ImGui::EndFrame();
            if (ImGui::GetIO().BackendRendererUserData)
                ImGui_ImplOpenGL3_Shutdown();
            if (ImGui::GetIO().BackendPlatformUserData)
                ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext(context);
        }
        if (window)
            glfwDestroyWindow(window);
        if (glfwReady)
            glfwTerminate();
    }
};

Window::Window(const char* title, int width, int height) : impl_(std::make_unique<Impl>())
{
    if (!title || width <= 0 || height <= 0)
        throw std::invalid_argument("Window requires a title and positive dimensions");
    if (ImGui::GetCurrentContext())
        throw std::logic_error("Window requires its own ImGui context; use one Window at a time");
    if (!glfwInit())
        throw glfwError("Could not initialize GLFW");
    impl_->glfwReady = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    impl_->window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!impl_->window)
        throw glfwError("Could not create an OpenGL 4.1 core window");

    glfwMakeContextCurrent(impl_->window);
    if (!gladLoadGL(glfwGetProcAddress) || !GLAD_GL_VERSION_4_1)
        throw std::runtime_error("GLAD could not load OpenGL 4.1 core");
    glfwSwapInterval(1);
    glfwGetFramebufferSize(impl_->window, &framebufferSize_.x, &framebufferSize_.y);

    IMGUI_CHECKVERSION();
    impl_->context = ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    applyWorkbenchStyle();
    if (!ImGui_ImplGlfw_InitForOpenGL(impl_->window, true))
        throw std::runtime_error("Could not initialize the ImGui GLFW backend");
    if (!ImGui_ImplOpenGL3_Init("#version 410 core"))
        throw std::runtime_error("Could not initialize the ImGui OpenGL backend");
}

Window::~Window() = default;

bool Window::beginFrame()
{
    if (impl_->frameOpen)
        throw std::logic_error("Call endFrame() before starting another frame");
    glfwPollEvents();
    while (!glfwWindowShouldClose(impl_->window))
    {
        glfwGetFramebufferSize(impl_->window, &framebufferSize_.x, &framebufferSize_.y);
        if (framebufferSize_.x > 0 && framebufferSize_.y > 0 && !glfwGetWindowAttrib(impl_->window, GLFW_ICONIFIED))
            break;
        glfwWaitEvents();
    }
    if (glfwWindowShouldClose(impl_->window))
        return false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    impl_->frameOpen = true;
    deltaTime_ = ImGui::GetIO().DeltaTime;
    input_.update(glfwGetWindowAttrib(impl_->window, GLFW_FOCUSED) == GLFW_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferSize_.x, framebufferSize_.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return true;
}

void Window::endFrame()
{
    if (!impl_->frameOpen)
        throw std::logic_error("Call beginFrame() before endFrame()");
    ImGui::Render();
    impl_->frameOpen = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(impl_->window);
}

void Window::requestClose()
{
    glfwSetWindowShouldClose(impl_->window, GLFW_TRUE);
}
}
