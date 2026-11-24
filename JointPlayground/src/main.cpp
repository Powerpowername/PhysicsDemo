#include "JointView.hpp"
#include "JointWorld.hpp"

#include <graphics/Workbench.hpp>

#include <exception>
#include <iostream>

int main()
{
    try
    {
        JointWorld world;
        world.create();

        graphics::Workbench app{"PhysX Joint Playground", JointWorld::fixedStep};
        app.step = [&] { world.step(); };
        app.reset = [&] { world.reset(); };
        JointView view;
        view.configure(app, world);
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "JointPlayground: " << error.what() << '\n';
        return 1;
    }
}
