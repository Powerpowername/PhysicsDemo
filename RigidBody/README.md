# RigidBody

这个项目模拟一个从 `y = 3 m` 释放的动态方块落到静态地面。物理以固定的 `1/60 s` 步长推进，窗口和播放控制由 `Graphics::Workbench` 提供。

接触回调只报告新的接触事件。终端中的 `impulse` 是当前物理步的接触冲量，`averageForce` 是用 `impulse / (1/60 s)` 得到的该步平均力。方块持续静止时不会重复打印。

从仓库根目录运行：

```sh
cmake -S . -B build
cmake --build build --target RigidBody
./build/RigidBody/RigidBody
```

中央是 3D 场景。拖动两侧的分隔线可调整视图宽度，拖动面板标签可重新停靠。工作台按窗口标题单独保存布局；选 `Workspace → Restore default layout` 可恢复初始布局。

- 左侧 **Simulation**：暂停或继续，暂停后可单步推进。**Reset** 会把方块放回 3 m 高处，清空时间与接触记录，并保持当前播放状态。
- 右侧 **Inspector**：查看方块高度、速度和最近一次触地信息。位置、法线、冲量和平均力属于一个代表性接触点；点数是整对形状的接触点总数。终端保留完整向量。

看物理实现从 [PhysicsWorld.cpp](src/PhysicsWorld.cpp) 开始：这里创建场景、推进模拟并报告碰撞。[main.cpp](src/main.cpp) 把物理步进和重置交给工作台。[RigidBodyView.cpp](src/RigidBodyView.cpp) 定义这个项目要显示的方块、地面和接触信息。其他同级项目的构建方式见[仓库说明](../README.md)。
