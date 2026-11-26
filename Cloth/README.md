# XPBD 布料

顶边固定在横杆上，下面的布随风摆动。左侧可以开关风力、暂停、单步和重置。关闭风力后，布会在重力和阻尼作用下逐渐垂下；暂停时点 **Step** 可以观察一个物理步的变化。

从仓库根目录运行：

```sh
cmake -S . -B build
cmake --build build --target Cloth
./build/Cloth/Cloth
```

这个目标只链接共享图形工作台。布料求解器使用 C++ 和 GLM，没有调用 PhysX；仓库的顶层配置仍会照常发现其他示例。

先读 [ClothWorld.cpp](src/ClothWorld.cpp)：

1. `create()` 将布铺成 18 × 13 个粒子。从初始网格量出每条约束的静止长度。横竖边控制拉伸，对角线控制剪切，跨两个格子的较软约束近似抵抗弯折。顶行粒子的逆质量为零，求解时位置不变。
2. `step()` 每次推进 1/120 秒：先用重力和风预测位置，再迭代修正距离，最后用修正后的位移更新速度。
3. `solveConstraint()` 实现 [XPBD 距离约束](https://mmacklin.com/xpbd.pdf)。`compliance` 越大，约束越软；`lambda` 记录该约束在当前时间步内累计的修正量，在下一个时间步开始时清零。

[ClothView.cpp](src/ClothView.cpp) 把相邻的四个粒子画成一块格纹布面；显示和物理分开。这个示例演示约束求解和风力响应，不包含自碰撞或与其他物体的接触。
