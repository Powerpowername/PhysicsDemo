#include "StackWorld.hpp"

#include <cmath>
#include <stdexcept>

using namespace physx;

namespace
{
constexpr int layers = 5;

PxFilterFlags contactFilter(PxFilterObjectAttributes, PxFilterData, PxFilterObjectAttributes,
                            PxFilterData, PxPairFlags& pairFlags, const void*, PxU32)
{
    pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eDETECT_CCD_CONTACT;
    return PxFilterFlag::eDEFAULT;
}

void releaseBodies(std::vector<PxRigidDynamic*>& bodies)
{
    for (auto* body : bodies) body->release();
    bodies.clear();
}
}

StackWorld::~StackWorld()
{
    releaseBodies(balls);
    releaseBodies(boxes);
    PX_RELEASE(ground);
    PX_RELEASE(scene);
    PX_RELEASE(material);
    PX_RELEASE(dispatcher);
    if (extensionsInitialized) PxCloseExtensions();
    PX_RELEASE(physics);
    PX_RELEASE(foundation);
}

void StackWorld::create()
{
    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errors);
    if (!foundation) throw std::runtime_error("Could not create PhysX foundation");
    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale{});
    if (!physics) throw std::runtime_error("Could not create PhysX instance");
    extensionsInitialized = PxInitExtensions(*physics, nullptr);
    if (!extensionsInitialized) throw std::runtime_error("Could not initialize PhysX extensions");
    dispatcher = PxDefaultCpuDispatcherCreate(1);
    if (!dispatcher) throw std::runtime_error("Could not create PhysX dispatcher");

    PxSceneDesc desc(physics->getTolerancesScale());
    desc.gravity = PxVec3(0, -9.81f, 0);
    desc.cpuDispatcher = dispatcher;
    desc.filterShader = contactFilter;
    desc.flags |= PxSceneFlag::eENABLE_CCD;
    scene = physics->createScene(desc);
    if (!scene) throw std::runtime_error("Could not create box stack scene");

    material = physics->createMaterial(0.6f, 0.5f, 0.05f);
    if (!material) throw std::runtime_error("Could not create contact material");
    ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    if (!ground || !scene->addActor(*ground)) throw std::runtime_error("Could not add ground");

    boxes.reserve(layers * (layers + 1) / 2);
    balls.reserve(maxBalls);
    reset();
}

PxRigidDynamic* StackWorld::addBody(const PxVec3& position, const PxGeometry& geometry, float density)
{
    auto* body = PxCreateDynamic(*physics, PxTransform(position), geometry, *material, density);
    if (!body) throw std::runtime_error("Could not create dynamic body");
    if (!scene->addActor(*body))
    {
        body->release();
        throw std::runtime_error("Could not add dynamic body to scene");
    }
    return body;
}

void StackWorld::reset()
{
    releaseBodies(balls);
    releaseBodies(boxes);
    const PxBoxGeometry cube(halfExtent, halfExtent, halfExtent);
    for (int level = 0; level < layers; ++level)
    {
        const int width = layers - level;
        for (int column = 0; column < width; ++column)
        {
            // Center each row so its cubes bridge the seams in the row below.
            const float x = (2 * column + 1 - width) * halfExtent;
            const float y = (2 * level + 1) * halfExtent;
            boxes.push_back(addBody({x, y, 0}, cube, 1.0f));
        }
    }
}

void StackWorld::launch(const PxVec3& origin, const PxVec3& direction)
{
    const float length = direction.magnitude();
    if (!origin.isFinite() || !std::isfinite(length) || length == 0.0f ||
        !std::isfinite(launchSpeed) || launchSpeed <= 0.0f)
        throw std::invalid_argument("Ball launch requires a finite position, direction and positive speed");
    if (balls.size() == maxBalls)
    {
        balls.front()->release();
        balls.erase(balls.begin());
    }
    // The denser sphere can move several cubes on impact. CCD checks its swept path.
    auto* ball = addBody(origin, PxSphereGeometry(ballRadius), 12.0f);
    ball->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, false);
    // ball->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    ball->setLinearVelocity((direction / length) * launchSpeed);
    balls.push_back(ball);
}

void StackWorld::step()
{
    scene->simulate(fixedStep);
    scene->fetchResults(true);
}
