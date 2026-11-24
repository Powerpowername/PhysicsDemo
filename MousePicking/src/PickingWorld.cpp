#include "PickingWorld.hpp"

#include <stdexcept>

using namespace physx;

namespace
{
const PxVec3 boxPositions[] = {{-1.7f, PickingWorld::halfExtent, 0.8f}, {1.7f, PickingWorld::halfExtent, -1.2f}};
const PxVec3 ballPositions[] = {{1.5f, PickingWorld::ballRadius, 1.4f}, {-1.5f, PickingWorld::ballRadius, -1.7f}};

void resetBody(PxRigidDynamic& body, const PxVec3& position)
{
    body.setGlobalPose(PxTransform(position));
    body.setLinearVelocity(PxVec3(0));
    body.setAngularVelocity(PxVec3(0));
}
}

PickingWorld::~PickingWorld()
{
    releaseGrab();
    for (auto*& body : balls) PX_RELEASE(body);
    for (auto*& body : boxes) PX_RELEASE(body);
    PX_RELEASE(ground);
    PX_RELEASE(scene);
    PX_RELEASE(material);
    PX_RELEASE(dispatcher);
    if (extensionsInitialized) PxCloseExtensions();
    PX_RELEASE(physics);
    PX_RELEASE(foundation);
}

void PickingWorld::create()
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
    desc.filterShader = PxDefaultSimulationFilterShader;
    scene = physics->createScene(desc);
    if (!scene) throw std::runtime_error("Could not create picking scene");
    material = physics->createMaterial(0.6f, 0.5f, 0.15f);
    if (!material) throw std::runtime_error("Could not create contact material");
    ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    if (!ground || !scene->addActor(*ground)) throw std::runtime_error("Could not add ground");
    ground->setName("Ground");

    for (std::size_t i = 0; i < boxes.size(); ++i)
        boxes[i] = addBody(PxBoxGeometry(halfExtent, halfExtent, halfExtent), boxPositions[i]);
    for (std::size_t i = 0; i < balls.size(); ++i)
        balls[i] = addBody(PxSphereGeometry(ballRadius), ballPositions[i]);
    boxes[0]->setName("Box 1");
    boxes[1]->setName("Box 2");
    balls[0]->setName("Sphere 1");
    balls[1]->setName("Sphere 2");
}

PxRigidDynamic* PickingWorld::addBody(const PxGeometry& geometry, const PxVec3& position)
{
    auto* body = PxCreateDynamic(*physics, PxTransform(position), geometry, *material, 1.0f);
    if (!body) throw std::runtime_error("Could not create a pickable body");
    if (!scene->addActor(*body))
    {
        body->release();
        throw std::runtime_error("Could not add pickable body to scene");
    }
    return body;
}

void PickingWorld::step()
{
    scene->simulate(fixedStep);
    scene->fetchResults(true);
}

void PickingWorld::reset()
{
    releaseGrab();
    for (std::size_t i = 0; i < boxes.size(); ++i) resetBody(*boxes[i], boxPositions[i]);
    for (std::size_t i = 0; i < balls.size(); ++i) resetBody(*balls[i], ballPositions[i]);
}

bool PickingWorld::grab(const PxVec3& origin, const PxVec3& unitDir, PxVec3& hitPoint)
{
    releaseGrab();
    PxRaycastBuffer hit;
    if (!scene->raycast(origin, unitDir, PX_MAX_F32, hit)) return false;
    auto* body = hit.block.actor->is<PxRigidDynamic>();
    if (!body) return false; // The closest surface may be the static ground.

    // Frame 0 is world space; frame 1 remains at the clicked point as the body rotates.
    const PxVec3 localHit = body->getGlobalPose().transformInv(hit.block.position);
    joint = PxD6JointCreate(*physics, nullptr, PxTransform(PxIdentity), body, PxTransform(localHit));
    if (!joint) throw std::runtime_error("Could not create mouse picking joint");
    for (int axis = 0; axis < PxD6Axis::eCOUNT; ++axis)
        joint->setMotion(static_cast<PxD6Axis::Enum>(axis), PxD6Motion::eFREE);
    const PxD6JointDrive spring(400.0f, 40.0f, PX_MAX_F32, true);
    for (auto axis : {PxD6Drive::eX, PxD6Drive::eY, PxD6Drive::eZ})
        joint->setDrive(axis, spring);
    held = body;
    hitPoint = hit.block.position;
    moveGrab(hitPoint);
    return true;
}

void PickingWorld::moveGrab(const PxVec3& target)
{
    if (joint) joint->setDrivePosition(PxTransform(target));
}

void PickingWorld::releaseGrab()
{
    PX_RELEASE(joint);
    held = nullptr;
}
