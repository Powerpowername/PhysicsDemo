#include "ClothView.hpp"
#include "ClothWorld.hpp"

#include <graphics/Workbench.hpp>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        ClothWorld world;
        world.create();

        graphics::Workbench app{"XPBD Cloth", ClothWorld::fixedStep};
        app.step = [&] { world.step(); };
        app.reset = [&] { world.reset(); };
        configureClothView(app, world);
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Cloth: " << error.what() << '\n';
        return 1;
    }
}
