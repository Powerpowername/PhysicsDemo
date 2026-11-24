# JointPlayground

场景同时平铺六个相互独立的 PhysX 关节样例。每个样例有一个橙色运动学 driver、一个蓝色动态 body，以及连接它们的一个真实关节。鼠标只移动被点中的橙色 driver；蓝色 body 只能按自己的关节规则响应。

六个样例分成两排：

- **Fixed**：driver 和 body 保持相对位置与姿态不变。
- **Distance**：两端距离保持在 0.75–1.25 m 范围内。
- **Spherical**：连接点保持重合，body 可以绕连接点转动。
- **Revolute**：body 只能绕关节的 X 轴转动。
- **Prismatic**：body 只能沿关节的 X 轴移动。
- **D6**：只放开 X 平移和绕 X 的转动，其余自由度锁住。

在仓库根目录运行：

```sh
cmake -S . -B build
cmake --build build --target JointPlayground
./build/JointPlayground/JointPlayground
```

按住鼠标左键拖动任意橙色 driver，滚轮沿视线调整它的深度，松开后停止控制。左侧面板同时列出六个样例和各自的控制含义；右侧面板显示六个关节的当前值。`Pause`、`Step` 和 `Reset` 对六个样例同时生效。

物理创建和鼠标命中逻辑位于 [JointWorld.cpp](src/JointWorld.cpp)，六个样例的平铺绘制和鼠标射线位于 [JointView.cpp](src/JointView.cpp)。
