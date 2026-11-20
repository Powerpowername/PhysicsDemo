#pragma once

#include <PxPhysicsAPI.h>

constexpr float fixedStep = 1.0f / 60.0f;

struct ContactReport
{
    physx::PxU32 count = 0;
    float time = 0.0f;
    const char* shapeA = nullptr;
    const char* shapeB = nullptr;
    physx::PxU32 pointCount = 0;
    physx::PxContactPairPoint point{};
};

class ContactCallback final : public physx::PxSimulationEventCallback
{
public:
    explicit ContactCallback(const float& simulationTime) : time_(simulationTime) {}
    const ContactReport& report() const { return report_; }
    void reset() { report_.count = 0; }

private:
    void onContact(const physx::PxContactPairHeader&, const physx::PxContactPair*, physx::PxU32) override;
    void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override {}
    void onWake(physx::PxActor**, physx::PxU32) override {}
    void onSleep(physx::PxActor**, physx::PxU32) override {}
    void onTrigger(physx::PxTriggerPair*, physx::PxU32) override {}
    void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, physx::PxU32) override {}

    const float& time_;
    ContactReport report_;
};

class PhysicsWorld
{
public:
    PhysicsWorld() = default;
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    void create();
    void step();
    void reset();

    float time = 0.0f;
    ContactCallback callback{time};
    physx::PxRigidDynamic* box = nullptr;

private:
    void addBody(physx::PxRigidActor* actor, const char* shapeName);
    physx::PxDefaultAllocator allocator;
    physx::PxDefaultErrorCallback errorCallback;
    physx::PxFoundation* foundation = nullptr;
    physx::PxPhysics* physics = nullptr;
    physx::PxDefaultCpuDispatcher* dispatcher = nullptr;
    physx::PxScene* scene = nullptr;
    physx::PxMaterial* material = nullptr;
    physx::PxRigidStatic* ground = nullptr;
    bool extensionsInitialized = false;
};
