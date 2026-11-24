#include "PickingView.hpp"
#include "PickingWorld.hpp"

#include <graphics/Workbench.hpp>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        PickingWorld world;
        world.create();

        graphics::Workbench app{"PhysX Mouse Picking", PickingWorld::fixedStep};
        app.step = [&] { world.step(); };
        app.reset = [&] { world.reset(); };
        PickingView view;
        view.configure(app, world);
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "MousePicking: " << error.what() << '\n';
        return 1;
    }
}
