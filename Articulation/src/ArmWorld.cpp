#include "ArmWorld.hpp"

#include <stdexcept>

using namespace physx;

namespace
{
constexpr auto hingeAxis = PxArticulationAxis::eSWING2; // Local Z, perpendicular to the arm's plane.
constexpr int cycleSteps = 240; // Four simulation seconds from reach to fold and back.
}

ArmWorld::~ArmWorld()
{
    PX_RELEASE(arm_); // The articulation owns its links and their inbound joints.
    PX_RELEASE(scene_);
    PX_RELEASE(material_);
    PX_RELEASE(dispatcher_);
    if (extensionsInitialized_) PxCloseExtensions();
    PX_RELEASE(physics_);
    PX_RELEASE(foundation_);
}

PxArticulationLink* ArmWorld::addLink(PxArticulationLink* parent, const PxVec3& center,
                                     const PxVec3& halfExtents)
{
    auto* link = arm_->createLink(parent, PxTransform(center));
    if (!link) throw std::runtime_error("Could not create articulation link");
    if (!PxRigidActorExt::createExclusiveShape(*link, PxBoxGeometry(halfExtents), *material_))
        throw std::runtime_error("Could not create articulation link shape");
    if (!PxRigidBodyExt::updateMassAndInertia(*link, 3.0f))
        throw std::runtime_error("Could not set articulation link inertia");
    return link;
}

void ArmWorld::create()
{
    foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, allocator_, errors_);
    if (!foundation_) 
        throw std::runtime_error("Could not create PhysX foundation");
    physics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation_, PxTolerancesScale{});
    if (!physics_) 
        throw std::runtime_error("Could not create PhysX instance");
    extensionsInitialized_ = PxInitExtensions(*physics_, nullptr);
    if (!extensionsInitialized_) 
        throw std::runtime_error("Could not initialize PhysX extensions");
    dispatcher_ = PxDefaultCpuDispatcherCreate(1);
    if (!dispatcher_) 
        throw std::runtime_error("Could not create PhysX dispatcher");
    PxSceneDesc desc(physics_->getTolerancesScale());
    desc.gravity = {0, -9.81f, 0};
    desc.cpuDispatcher = dispatcher_;
    desc.filterShader = PxDefaultSimulationFilterShader;
    scene_ = physics_->createScene(desc);
    if (!scene_) 
        throw std::runtime_error("Could not create articulation scene");
    material_ = physics_->createMaterial(0.5f, 0.5f, 0.0f);
    if (!material_) 
        throw std::runtime_error("Could not create articulation material");
    arm_ = physics_->createArticulationReducedCoordinate();
    if (!arm_) 
        throw std::runtime_error("Could not create articulation");
    arm_->setArticulationFlag(PxArticulationFlag::eFIX_BASE, true);
    base_ = addLink(nullptr, {0, baseHalfExtents.y, 0}, baseHalfExtents);

    auto* parent = base_;
    PxVec3 parentAnchor{0, baseHalfExtents.y, 0};
    for (std::size_t i = 0; i < links_.size(); ++i)
    {
        // Assemble straight along +X at zero angle, with coincident anchors on adjacent links.
        const PxVec3 halfArm{linkHalfExtents[i].x, 0, 0};
        const PxVec3 pivot = parent->getGlobalPose().transform(parentAnchor);
        links_[i] = addLink(parent, pivot + halfArm, linkHalfExtents[i]);
        auto* joint = links_[i]->getInboundJoint();
        joint->setParentPose(PxTransform(parentAnchor));
        joint->setChildPose(PxTransform(-halfArm));
        joint->setJointType(PxArticulationJointType::eREVOLUTE);
        joint->setMotion(hingeAxis, PxArticulationMotion::eLIMITED);
        joint->setLimitParams(hingeAxis, PxArticulationLimit(minAngle[i], maxAngle[i]));
        joint->setDriveParams(hingeAxis, PxArticulationDrive(750.0f, 40.0f, PxPerformanceEnvelope(160.0f)));
        parent = links_[i];
        parentAnchor = halfArm;
    }
    if (!scene_->addArticulation(*arm_)) throw std::runtime_error("Could not add articulation to scene");
    reset();
}

void ArmWorld::setTarget(std::size_t index, float radians)
{
    auto* joint = links_.at(index)->getInboundJoint();
    autoMotion = false;
    joint->setDriveTarget(hingeAxis, PxClamp(radians, minAngle[index], maxAngle[index]));
}

float ArmWorld::target(std::size_t index) const
{
    return links_.at(index)->getInboundJoint()->getDriveTarget(hingeAxis);
}

float ArmWorld::angle(std::size_t index) const
{
    return links_.at(index)->getInboundJoint()->getJointPosition(hingeAxis);
}

void ArmWorld::step()
{
    if (autoMotion)
    {
        cycleStep_ = (cycleStep_ + 1) % cycleSteps;
        const float blend = 0.5f * (1.0f - PxCos(PxTwoPi * cycleStep_ / cycleSteps));
        for (std::size_t i = 0; i < links_.size(); ++i)
        {
            const float goal = reachAngles[i] + blend * (foldAngles[i] - reachAngles[i]);
            links_[i]->getInboundJoint()->setDriveTarget(hingeAxis, goal);
        }
    }
    scene_->simulate(fixedStep);
    scene_->fetchResults(true);
}

void ArmWorld::reset()
{
    cycleStep_ = 0;
    autoMotion = true;
    for (std::size_t i = 0; i < links_.size(); ++i)
    {
        auto* joint = links_[i]->getInboundJoint();
        joint->setJointPosition(hingeAxis, reachAngles[i]);
        joint->setJointVelocity(hingeAxis, 0.0f);
        joint->setDriveTarget(hingeAxis, reachAngles[i]);
    }
    // Propagate the reset coordinates to link poses and velocities, even while paused.
    arm_->updateKinematic(PxArticulationKinematicFlag::ePOSITION);
}
