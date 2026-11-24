#pragma once

#include <PxPhysicsAPI.h>
#include <array>

class PickingWorld
{
public:
    static constexpr float fixedStep = 1.0f / 60.0f;
    static constexpr float halfExtent = 0.65f;
    static constexpr float ballRadius = 0.6f;

    PickingWorld() = default;
    ~PickingWorld();
    PickingWorld(const PickingWorld&) = delete;
    PickingWorld& operator=(const PickingWorld&) = delete;

    void create();
    void step();
    void reset();
    bool grab(const physx::PxVec3& origin, const physx::PxVec3& unitDir, physx::PxVec3& hitPoint);
    void moveGrab(const physx::PxVec3& target);
    void releaseGrab();
    physx::PxRigidDynamic* selected() const { return held; }

    std::array<physx::PxRigidDynamic*, 2> boxes{};
    std::array<physx::PxRigidDynamic*, 2> balls{};

private:
    physx::PxRigidDynamic* addBody(const physx::PxGeometry& geometry, const physx::PxVec3& position);
    physx::PxDefaultAllocator allocator;
    physx::PxDefaultErrorCallback errors;
    physx::PxFoundation* foundation = nullptr;
    physx::PxPhysics* physics = nullptr;
    physx::PxDefaultCpuDispatcher* dispatcher = nullptr;
    physx::PxScene* scene = nullptr;
    physx::PxMaterial* material = nullptr;
    physx::PxRigidStatic* ground = nullptr;
    physx::PxD6Joint* joint = nullptr;
    physx::PxRigidDynamic* held = nullptr;
    bool extensionsInitialized = false;
};
