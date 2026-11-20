#pragma once

#include <PxPhysicsAPI.h>
#include <vector>

class StackWorld
{
public:
    static constexpr float fixedStep = 1.0f / 60.0f;
    static constexpr float halfExtent = 0.5f;
    static constexpr float ballRadius = 0.35f;
    static constexpr unsigned maxBalls = 32;

    StackWorld() = default;
    ~StackWorld();
    StackWorld(const StackWorld&) = delete;
    StackWorld& operator=(const StackWorld&) = delete;

    void create();
    void step();
    void reset();
    void launch(const physx::PxVec3& origin, const physx::PxVec3& direction);

    float launchSpeed = 24.0f;
    std::vector<physx::PxRigidDynamic*> boxes;
    std::vector<physx::PxRigidDynamic*> balls;

private:
    physx::PxRigidDynamic* addBody(const physx::PxVec3& position,
                                   const physx::PxGeometry& geometry, float density);
    physx::PxDefaultAllocator allocator;
    physx::PxDefaultErrorCallback errors;
    physx::PxFoundation* foundation = nullptr;
    physx::PxPhysics* physics = nullptr;
    physx::PxDefaultCpuDispatcher* dispatcher = nullptr;
    physx::PxScene* scene = nullptr;
    physx::PxMaterial* material = nullptr;
    physx::PxRigidStatic* ground = nullptr;
    bool extensionsInitialized = false;
};
