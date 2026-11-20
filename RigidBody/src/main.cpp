#include "PhysicsWorld.hpp"
#include "RigidBodyView.hpp"

#include <graphics/Workbench.hpp>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        PhysicsWorld world;
        world.create();

        graphics::Workbench app{"PhysX Rigid Body", fixedStep};
        app.step = [&] { world.step(); };
        app.reset = [&] { world.reset(); };
        configureRigidBodyView(app, world);
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "RigidBody: " << error.what() << '\n';
        return 1;
    }
}
