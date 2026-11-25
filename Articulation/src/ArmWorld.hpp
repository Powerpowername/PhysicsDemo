#pragma once

#include <PxPhysicsAPI.h>

#include <array>
#include <cstddef>

class ArmWorld
{
public:
    static constexpr float fixedStep = 1.0f / 60.0f;
    // Joint 0 is the shoulder; joint 1 is the elbow. Angles are in radians.
    static constexpr std::array<float, 2> minAngle{0.0f, -100.0f * physx::PxPi / 180.0f};
    static constexpr std::array<float, 2> maxAngle{physx::PxHalfPi, 100.0f * physx::PxPi / 180.0f};
    static constexpr std::array<float, 2> reachAngles{physx::PxPi / 12, 0.0f};
    static constexpr std::array<float, 2> foldAngles{5 * physx::PxPi / 12, physx::PxHalfPi};
    inline static const physx::PxVec3 baseHalfExtents{0.34f, 0.85f, 0.32f};
    inline static const std::array<physx::PxVec3, 2> linkHalfExtents{{
        {0.90f, 0.14f, 0.16f}, {0.70f, 0.11f, 0.13f}}};

    ArmWorld() = default;
    ~ArmWorld();
    ArmWorld(const ArmWorld&) = delete;
    ArmWorld& operator=(const ArmWorld&) = delete;

    void create();
    void step();
    void reset();
    bool autoMotion = true;
    void setTarget(std::size_t index, float radians);
    float target(std::size_t index) const;
    float angle(std::size_t index) const;
    const physx::PxArticulationLink& base() const { return *base_; }
    const physx::PxArticulationLink& link(std::size_t index) const { return *links_.at(index); }

private:
    physx::PxArticulationLink* addLink(physx::PxArticulationLink* parent, const physx::PxVec3& center,
                                      const physx::PxVec3& halfExtents);

    physx::PxDefaultAllocator allocator_;
    physx::PxDefaultErrorCallback errors_;
    physx::PxFoundation* foundation_ = nullptr;
    physx::PxPhysics* physics_ = nullptr;
    physx::PxDefaultCpuDispatcher* dispatcher_ = nullptr;
    physx::PxScene* scene_ = nullptr;
    physx::PxMaterial* material_ = nullptr;
    physx::PxArticulationReducedCoordinate* arm_ = nullptr;
    physx::PxArticulationLink* base_ = nullptr;
    std::array<physx::PxArticulationLink*, 2> links_{};
    int cycleStep_ = 0;
    bool extensionsInitialized_ = false;
};
