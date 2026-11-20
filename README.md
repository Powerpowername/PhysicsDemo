# PhysXCodeSpace

`HelloPhysX/` 和 `RigidBody/` 是两个独立的小项目。新项目也直接放在仓库根目录，与它们同级。在新目录的 `CMakeLists.txt` 中定义自己的目标即可，顶层 CMake 会自动发现；项目内部如何组织源文件由项目自己决定。

从仓库根目录配置一次，然后按目标名构建。例如：

```sh
cmake -S . -B build
cmake --build build --target RigidBody
./build/RigidBody/RigidBody
```

加入新项目后重新执行配置，再把构建命令里的 `RigidBody` 换成新目标名。构建新目标只会编译它和需要更新的依赖。需要仿真窗口时，链接 [Graphics::Workbench](graphics/README.md)。
