#pragma once

#include <PxPhysicsAPI.h>

#include <array>
#include <optional>

class JointWorld
{
public:
    static constexpr float fixedStep = 1.0f / 60.0f;
    static constexpr float shaftHalfLengthInRadii = 4.0f;

    enum class Kind { Fixed, Distance, Spherical, Revolute, Prismatic, D6 };
    enum class DemoId : std::size_t { Fixed, Distance, Spherical, Revolute, Prismatic, D6, Count };

    struct Demo
    {
        Kind kind;
        const char* name;
        const char* control;
        physx::PxVec3 driverHalfExtents{0.0f};
        physx::PxVec3 bodyHalfExtents{0.0f};
        physx::PxRigidDynamic* driver = nullptr;
        physx::PxRigidDynamic* body = nullptr;
        physx::PxJoint* joint = nullptr;
        physx::PxTransform initialDriverPose{physx::PxIdentity};
        physx::PxTransform initialBodyPose{physx::PxIdentity};
        bool dragging = false;
    };

    JointWorld() = default;
    ~JointWorld();
    JointWorld(const JointWorld&) = delete;
    JointWorld& operator=(const JointWorld&) = delete;

    void create();
    void step();
    void reset();
    const std::array<Demo, static_cast<std::size_t>(DemoId::Count)>& demos() const { return demos_; }
    bool beginDrag(const physx::PxVec3& origin, const physx::PxVec3& unitDir,
                   DemoId& demo, physx::PxVec3& hitPoint);
    void moveDrag(DemoId demo, const physx::PxVec3& target);
    void endDrag();
    std::optional<DemoId> activeDemo() const { return activeDemo_; }
    float jointValue(const Demo& demo) const;

private:
    void createDemo(Demo& demo, const physx::PxVec3& position);
    void createParts(Demo& demo, const physx::PxTransform& driverPose,
                     const physx::PxVec3& driverHalfExtents, const physx::PxVec3& bodyOffset,
                     const physx::PxVec3& bodyHalfExtents);

    physx::PxDefaultAllocator allocator_;
    physx::PxDefaultErrorCallback errors_;
    physx::PxFoundation* foundation_ = nullptr;
    physx::PxPhysics* physics_ = nullptr;
    physx::PxDefaultCpuDispatcher* dispatcher_ = nullptr;
    physx::PxScene* scene_ = nullptr;
    physx::PxMaterial* material_ = nullptr;
    physx::PxRigidStatic* ground_ = nullptr;
    std::array<Demo, static_cast<std::size_t>(DemoId::Count)> demos_{
        {{Kind::Fixed, "Fixed", "Stays rigid."},
         {Kind::Distance, "Distance", "Tether: 0.75 to 1.25 m."},
         {Kind::Spherical, "Spherical", "Swings around the pivot."},
         {Kind::Revolute, "Revolute", "Swings on the pin."},
         {Kind::Prismatic, "Prismatic", "Slides on the rail."},
         {Kind::D6, "D6", "Slides and twists."}}};
    std::optional<DemoId> activeDemo_;
    physx::PxVec3 grabOffset_{0.0f};
    physx::PxVec3 dragTarget_{0.0f};
    bool extensionsInitialized_ = false;
};
