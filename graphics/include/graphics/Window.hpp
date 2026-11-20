#pragma once

#include <glad/gl.h>
#include <glm/vec2.hpp>
#include <imgui.h>

#include <array>
#include <memory>

namespace graphics
{
struct ButtonState
{
    bool down = false;
    bool pressed = false;
    bool released = false;
};

// Scene input for the current frame. ImGui widgets still receive all events.
class Input
{
public:
    ButtonState key(ImGuiKey key) const;
    ButtonState mouse(ImGuiMouseButton button) const;

    glm::vec2 mousePosition{}; // Window coordinates, origin at the top left.
    glm::vec2 mouseDelta{};
    glm::vec2 scrollDelta{};   // Horizontal and vertical wheel steps.

private:
    friend class Window;
    void update(bool focused);
    std::array<ButtonState, ImGuiKey_GamepadStart - ImGuiKey_Tab> keys_{};
    std::array<ButtonState, ImGuiMouseButton_COUNT> buttons_{};
};

// Owns the application's GLFW window, OpenGL context and Dear ImGui context.
// Use one Window at a time, on the main thread.
class Window
{
public:
    explicit Window(const char* title, int width = 1280, int height = 720);
    ~Window();

    // Returns false when closed. Pair each successful call with endFrame().
    bool beginFrame();
    void endFrame();
    void requestClose();

    const Input& input() const { return input_; }
    glm::ivec2 framebufferSize() const { return framebufferSize_; }
    float deltaTime() const { return deltaTime_; } // Seconds.

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Input input_;
    glm::ivec2 framebufferSize_{};
    float deltaTime_ = 0.0f;
};
}
