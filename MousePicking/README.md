# MousePicking

这个场景有两个青色方块、两个橙色球和一块固定地面。用左键抓住物体表面并拖动；选中的物体会变成黄色，右侧显示名称和抓取深度。试着抓住方块上角提起来：它会在重力作用下绕抓点摆动。松手后，物体保留当时的速度并落回地面。

在仓库根目录运行：

```sh
cmake -S . -B build
cmake --build build --target MousePicking
./build/MousePicking/MousePicking
```

- **左键按下**：从相机经过鼠标位置做一次 PhysX 射线查询。抓住最近的动态物体；点空、点地面或点侧栏都不会抓取，也不会透过近物选到后面的物体。
- **按住拖动**：鼠标在一个与屏幕平行、经过原始命中点的平面上移动抓取目标。滚轮向上把平面拉近，向下推远，每格 0.5 m。移出中央视口时目标停在最后的位置。
- **松开左键**：解除抓取。**Reset** 会放手并把所有物体放回起点。暂停时可以选择物体或调整目标；物体在单步或继续模拟时跟随。

物理实现见 [PickingWorld.cpp](src/PickingWorld.cpp)：`grab()` 把射线命中点换算到刚体局部坐标，并在该点创建 D6 关节；三个平移方向使用弹簧和阻尼，三个旋转方向自由。`moveGrab()` 更新关节目标，不直接修改刚体位姿。`releaseGrab()` 释放关节。[PickingView.cpp](src/PickingView.cpp) 负责把鼠标位置和滚轮换成三维目标。方块和球体都通过公共 `Graphics::SceneRenderer::drawMesh()` 提交。
