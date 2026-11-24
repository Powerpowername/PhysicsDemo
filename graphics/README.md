# 物理项目的图形接口

## Graphics::Workbench

需要仿真窗口的项目链接 `Graphics::Workbench`，包含 `<graphics/Workbench.hpp>`。工作台负责 GLFW、GLAD、ImGui 停靠、固定步长循环和场景视口。入口只连接物理操作：

```cpp
graphics::Workbench app{"PhysX Rigid Body", fixedStep};
app.step = [&] { world.step(); };
app.reset = [&] { world.reset(); };
app.run();
```

`step` 是唯一必填的回调，每次调用应推进构造时指定的步长（秒）。工作台自带暂停和单步；提供 `reset` 就显示重置按钮。进入 `run()` 前还可以设置：

| 成员 | 用途 |
| --- | --- |
| `camera`、`drawScene` | 设置相机、提交本项目的场景物体，例如调用 `renderer.drawMesh(...)`。工作台负责开始和结束绘制。 |
| `controls` | 在左侧播放按钮下面添加项目自己的控件或说明。 |
| `inspector` | 在右侧显示物理状态；未设置时，中央视口使用这部分空间。 |

这两个面板回调只写面板内容，无需调用 `ImGui::Begin/End`。`run()` 在主线程运行到窗口关闭；回调引用的物理世界需要在此期间保持有效。不同窗口标题的布局分别保存到当前工作目录的 `imgui-<ID>.ini`。

## Graphics::Window

项目内的目标链接 `Graphics::Window`，源码只需包含 `<graphics/Window.hpp>`，即可使用窗口、OpenGL 函数、GLM 向量和 Dear ImGui 控件。`HelloPhysX` 已链接此库；只有构造 `graphics::Window` 才会打开窗口。

| 接口 | 约定 |
| --- | --- |
| `graphics::Window(title, width, height)` | 在主线程创建 GLFW 窗口、OpenGL 4.1 Core 上下文，初始化 GLAD 和 ImGui。默认尺寸 1280×720。失败时抛出带原因的异常。 |
| `window.beginFrame()` | 处理窗口事件、更新输入、按帧缓冲像素尺寸设置视口并清屏。窗口最小化时等待事件，窗口关闭时返回 `false`。 |
| `window.endFrame()` | 每次 `beginFrame()` 返回 `true` 后调用一次。两者之间可直接绘制 OpenGL 场景和定义 ImGui 面板；结束时库将面板叠加到窗口并交换缓冲。 |
| `window.requestClose()` | 请求退出，下一次 `beginFrame()` 返回 `false`。关闭快捷键由业务决定。 |
| `window.framebufferSize()` / `window.deltaTime()` | 当前帧缓冲的像素尺寸 / 两帧之间的秒数。高 DPI 下帧缓冲尺寸可能大于窗口尺寸。 |

在帧内用 `window.input()` 读取场景输入：

- `key(ImGuiKey_W)`、`mouse(ImGuiMouseButton_Left)` 返回相同的三个状态：`pressed`（本帧按下）、`down`（持续按住）、`released`（本帧松开）。可换用其他键或鼠标按钮，不预设相机或移动方式。
- `mousePosition` 是相对窗口左上角的逻辑坐标；指针离开时保留最后的位置。`mouseDelta` 是本帧位移，`scrollDelta` 是本帧滚轮的水平和垂直步数。
- ImGui 捕获键盘或鼠标时，对应的场景输入会清零，鼠标绝对位置仍可读取。失焦也会取消按住状态；在界面里开始的按键不会在返回场景时变成一次操作。

当前支持在主线程一次持有一个窗口。窗口析构会清理 ImGui、OpenGL 上下文和 GLFW；业务创建的纹理、缓冲等应在窗口析构前释放。

窗口已启用 Dear ImGui `v1.92.9b-docking`，核心与 GLFW/OpenGL 后端随项目离线构建。所有窗口共用石墨色与青绿色主题；应用自行决定哪些面板参与停靠。现有普通 ImGui 窗口无需调整。

## Graphics::SceneRenderer

`Graphics::SceneRenderer` 只依赖当前 OpenGL 上下文，不依赖 PhysX。它只负责把网格提交到当前场景，不知道网格是方块、球体还是项目自定义形状：

先创建 `Graphics::Window`，再创建 `SceneRenderer`，并在同一个主线程中使用它们；这样构造和析构时 OpenGL 上下文始终有效。

```cpp
#include <graphics/Primitives.hpp>

graphics::SceneRenderer renderer;
renderer.beginScene(camera, window.framebufferSize());
const auto box = renderer.createMesh(graphics::primitives::box());
renderer.drawMesh(box, {position, rotation, scale}, {color});
renderer.endScene();
```

`MeshData` 只包含 CPU 侧顶点和索引，`createMesh()` 在当前 OpenGL 上下文中上传一次并返回 `MeshHandle`。后续每帧只传句柄、变换和材质。`Material::referenceLines` 可以给需要观察旋转的物体开启局部坐标参考线；球体使用它绘制随姿态旋转的纬线和经线。`Transform`、`Camera` 和 `Material` 是渲染层的数据类型；PhysX 的 `PxTransform` 应在应用层转换成 `Transform`。

如果只绘制窗口的一部分，用 `beginScene(camera, viewportSize, viewportOrigin)`。尺寸和左下角坐标均为帧缓冲像素；场景只清除和绘制这个矩形，`endScene()` 恢复之前的视口和裁剪区域。

常用的方块和球体数据集中在 `graphics::primitives` 中。项目在第一次绘制时创建句柄，之后使用同一个句柄绘制任意数量的实例。新增形状只需提供 `MeshData`；不需要修改 `SceneRenderer`，也不需要为每种形状增加 Renderer 类或 CMake 目标。

## 着色器归档

GLSL 源文件统一放在 `graphics/shaders/`，按所有者分目录、按用途命名。同一程序的两个阶段使用相同文件名，仅后缀不同：

- `scene/mesh.vert.glsl`、`scene/mesh.frag.glsl`：所有基础三角网格共用的顶点与片元着色器。
- 项目专用的着色器放在以项目命名的子目录，例如 `RigidBody/`；复用共享程序的项目不必另存一份。

CMake 构建时按原目录结构同步 `*.glsl` 到构建目录的 `graphics/shaders/`。链接 `Graphics::SceneRenderer` 后，可包含 `<graphics/Shader.hpp>`，并在窗口创建后用归档内的两个相对路径构造 `graphics::Shader`。路径不依赖启动程序的工作目录。调用 `use()` 绑定程序；`uniformLocation("model")` 查询一个已启用的 uniform，位置可以在初始化时保存。对象析构时释放 OpenGL 程序，因此应在销毁窗口前释放。
