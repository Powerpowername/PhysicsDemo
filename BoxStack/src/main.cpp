#include "BoxStackView.hpp"
#include "StackWorld.hpp"

#include <graphics/Workbench.hpp>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        StackWorld world;
        world.create();

        graphics::Workbench app{"PhysX Box Stack", StackWorld::fixedStep};
        app.step = [&] { world.step(); };
        app.reset = [&] { world.reset(); };
        configureBoxStackView(app, world);
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "BoxStack: " << error.what() << '\n';
        return 1;
    }
}
