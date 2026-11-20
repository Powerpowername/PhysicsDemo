#include "PhysicsWorld.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace physx;

namespace
{
const PxTransform dropPose(PxVec3(0.0f, 3.0f, 0.0f));

std::ostream& operator<<(std::ostream& output, const PxVec3& value)
{
    return output << '(' << value.x << ", " << value.y << ", " << value.z << ')';
}

PxFilterFlags filterShader(PxFilterObjectAttributes, PxFilterData,
                           PxFilterObjectAttributes, PxFilterData,
                           PxPairFlags& pairFlags, const void*, PxU32)
{
    pairFlags = PxPairFlag::eCONTACT_DEFAULT
        | PxPairFlag::eNOTIFY_TOUCH_FOUND
        | PxPairFlag::eNOTIFY_CONTACT_POINTS;
    return PxFilterFlag::eDEFAULT;
}
}

void ContactCallback::onContact(const PxContactPairHeader&, const PxContactPair* pairs, PxU32 count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        const PxContactPair& pair = pairs[i];
        if (!(pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND))
            continue;
        // The panel displays the first contact point; contactCount still describes the entire pair.
        if (!pair.extractContacts(&report_.point, 1))
            continue;

        ++report_.count;
        report_.time = time_;
        report_.shapeA = pair.shapes[0]->getName();
        report_.shapeB = pair.shapes[1]->getName();
        report_.pointCount = pair.contactCount;
        const auto& point = report_.point;
        std::cout << std::fixed << std::setprecision(3)
                  << "[Contact] t=" << time_ << " s " << report_.shapeA << " <-> " << report_.shapeB
                  << " points=" << report_.pointCount << " position=" << point.position << " m"
                  << " normal=" << point.normal << " impulse=" << point.impulse << " N*s"
                  << " averageForce=" << point.impulse / fixedStep << " N" << std::endl;
    }
}

PhysicsWorld::~PhysicsWorld()
{
    PX_RELEASE(box);
    PX_RELEASE(ground);
    PX_RELEASE(scene);
    PX_RELEASE(material);
    PX_RELEASE(dispatcher);
    if (extensionsInitialized) PxCloseExtensions();
    PX_RELEASE(physics);
    PX_RELEASE(foundation);
}

void PhysicsWorld::addBody(PxRigidActor* actor, const char* shapeName)
{
    PxShape* shape = nullptr;
    if (!actor || actor->getShapes(&shape, 1) != 1 || !shape)
        throw std::runtime_error(std::string("Could not create actor and shape: ") + shapeName);
    shape->setName(shapeName);
    if (!scene->addActor(*actor))
        throw std::runtime_error(std::string("Could not add actor to scene: ") + shapeName);
}

void PhysicsWorld::create()
{
    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
    if (!foundation)
        throw std::runtime_error("Could not create PhysX foundation");
    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale{});
    if (!physics)
        throw std::runtime_error("Could not create PhysX instance");
    extensionsInitialized = PxInitExtensions(*physics, nullptr);
    if (!extensionsInitialized)
        throw std::runtime_error("Could not initialize PhysX extensions");

    dispatcher = PxDefaultCpuDispatcherCreate(1);
    if (!dispatcher)
        throw std::runtime_error("Could not create PhysX CPU dispatcher");
    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = filterShader;
    sceneDesc.simulationEventCallback = &callback;
    scene = physics->createScene(sceneDesc);
    if (!scene)
        throw std::runtime_error("Could not create PhysX scene");
    material = physics->createMaterial(0.5f, 0.5f, 0.0f);
    if (!material)
        throw std::runtime_error("Could not create PhysX material");

    ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    addBody(ground, "Ground");
    box = PxCreateDynamic(*physics, dropPose, PxBoxGeometry(0.5f, 0.5f, 0.5f), *material, 1.0f);
    addBody(box, "FallingBox");
}

void PhysicsWorld::step()
{
    time += fixedStep;
    scene->simulate(fixedStep);
    scene->fetchResults(true);
}

void PhysicsWorld::reset()
{
    box->setGlobalPose(dropPose);
    box->setLinearVelocity(PxVec3(0));
    box->setAngularVelocity(PxVec3(0));
    callback.reset();
    time = 0.0f;
}
