# BoxStack

15 个边长 1 m 的动态立方体在静态地面上错缝堆成五层。左键点击中央视口，橙色小球会从相机前方沿点击方向飞出，随后受重力影响并与方块碰撞。瞄准中间一层，比较不同初速度下方块的位移。

在仓库根目录运行：

```sh
cmake -S . -B build
cmake --build build --target BoxStack
./build/BoxStack/BoxStack
```

左侧 **Launch speed** 控制下一颗球的初速度（默认 24 m/s，可调 8–40 m/s）。**Pause** 暂停后仍可发球，按 **Step** 可逐步观察碰撞。**Reset** 清除所有球并重新堆好方块。场景最多保留最近的 32 个球，右侧 **Balls** 显示当前数量。

看物理实现从 [StackWorld.cpp](src/StackWorld.cpp) 开始：`reset()` 从底层向上摆放方块；`launch()` 创建动态球体，用 `setLinearVelocity()` 设置初速度，并启用连续碰撞检测；`step()` 每次推进 1/60 秒。[BoxStackView.cpp](src/BoxStackView.cpp) 将鼠标点击转换为发射方向。球体专用着色器位于 [graphics/shaders/BoxStack/](../graphics/shaders/BoxStack/)；方块和地面沿用公共渲染器。
