#include "ArmView.hpp"
#include "ArmWorld.hpp"

#include <graphics/Workbench.hpp>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        ArmWorld world;
        world.create();

        graphics::Workbench app{"PhysX Articulation", ArmWorld::fixedStep};
        app.step = [&] { world.step(); };
        app.reset = [&] { world.reset(); };
        configureArmView(app, world);
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Articulation: " << error.what() << '\n';
        return 1;
    }
}
