#include "JointWorld.hpp"

#include <cmath>
#include <stdexcept>

using namespace physx;

namespace
{
PxFilterFlags filter(PxFilterObjectAttributes, PxFilterData, PxFilterObjectAttributes, PxFilterData,
                     PxPairFlags& pairs, const void*, PxU32)
{
    pairs = PxPairFlag::eCONTACT_DEFAULT;
    return PxFilterFlag::eDEFAULT;
}

template <typename T>
void release(T*& object)
{
    PX_RELEASE(object);
}
}

JointWorld::~JointWorld()
{
    endDrag();
    for (auto& demo : demos_)
    {
        release(demo.joint);
        release(demo.body);
        release(demo.driver);
    }
    release(ground_);
    release(scene_);
    release(material_);
    release(dispatcher_);
    if (extensionsInitialized_) PxCloseExtensions();
    release(physics_);
    release(foundation_);
}

void JointWorld::create()
{
    foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, allocator_, errors_);
    if (!foundation_) throw std::runtime_error("Could not create PhysX foundation");
    physics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation_, PxTolerancesScale{});
    if (!physics_) throw std::runtime_error("Could not create PhysX instance");
    extensionsInitialized_ = PxInitExtensions(*physics_, nullptr);
    if (!extensionsInitialized_) throw std::runtime_error("Could not initialize PhysX extensions");
    dispatcher_ = PxDefaultCpuDispatcherCreate(1);
    if (!dispatcher_) throw std::runtime_error("Could not create PhysX dispatcher");

    PxSceneDesc desc(physics_->getTolerancesScale());
    desc.gravity = {0.0f, -9.81f, 0.0f};
    desc.cpuDispatcher = dispatcher_;
    desc.filterShader = filter;
    scene_ = physics_->createScene(desc);
    if (!scene_) throw std::runtime_error("Could not create joint scene");
    material_ = physics_->createMaterial(0.55f, 0.45f, 0.05f);
    if (!material_) throw std::runtime_error("Could not create joint material");
    ground_ = PxCreatePlane(*physics_, PxPlane(0, 1, 0, 0), *material_);
    if (!ground_ || !scene_->addActor(*ground_)) throw std::runtime_error("Could not add ground");

    const PxVec3 positions[] = {
        {-5.0f, 3.0f, -2.0f}, {0.0f, 3.0f, -2.0f}, {5.0f, 3.0f, -2.0f},
        {-5.0f, 3.0f,  2.0f}, {0.0f, 3.0f,  2.0f}, {5.0f, 3.0f,  2.0f},
    };
    for (std::size_t i = 0; i < demos_.size(); ++i) createDemo(demos_[i], positions[i]);
}

void JointWorld::createDemo(Demo& demo, const PxVec3& position)
{
    demo.initialDriverPose = PxTransform(position);
    demo.initialBodyPose = PxTransform(position + PxVec3(0.0f, -1.0f, 0.0f));
    const PxBoxGeometry geometry(halfExtent, halfExtent, halfExtent);
    demo.driver = PxCreateDynamic(*physics_, demo.initialDriverPose, geometry, *material_, 1.0f);
    demo.body = PxCreateDynamic(*physics_, demo.initialBodyPose, geometry, *material_, 1.0f);
    if (!demo.driver || !demo.body) throw std::runtime_error("Could not create joint demo bodies");
    demo.driver->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    demo.driver->setName(demo.name);
    demo.body->setName(demo.name);
    if (!scene_->addActor(*demo.driver) || !scene_->addActor(*demo.body))
        throw std::runtime_error("Could not add joint demo bodies");

    const PxTransform driverFrame(PxIdentity);
    const PxTransform bodyFrame = demo.initialBodyPose.transformInv(demo.initialDriverPose);
    demo.joint = createJoint(demo);
    if (!demo.joint) throw std::runtime_error("Could not create joint demo joint");
    demo.joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);
    demo.joint->setLocalPose(PxJointActorIndex::eACTOR0, driverFrame);
    demo.joint->setLocalPose(PxJointActorIndex::eACTOR1, bodyFrame);
}

PxJoint* JointWorld::createJoint(Demo& demo)
{
    const PxTransform driverFrame(PxIdentity);
    const PxTransform bodyFrame = demo.initialBodyPose.transformInv(demo.initialDriverPose);
    switch (demo.kind)
    {
    case Kind::Fixed:
        return PxFixedJointCreate(*physics_, demo.driver, driverFrame, demo.body, bodyFrame);
    case Kind::Distance:
    {
        auto* joint = PxDistanceJointCreate(*physics_, demo.driver, driverFrame, demo.body, bodyFrame);
        joint->setMinDistance(0.75f);
        joint->setMaxDistance(1.25f);
        joint->setDistanceJointFlags(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED |
                                     PxDistanceJointFlag::eMAX_DISTANCE_ENABLED);
        return joint;
    }
    case Kind::Spherical:
    {
        auto* joint = PxSphericalJointCreate(*physics_, demo.driver, driverFrame, demo.body, bodyFrame);
        joint->setLimitCone(PxJointLimitCone(0.85f, 0.85f));
        joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
        return joint;
    }
    case Kind::Revolute:
    {
        auto* joint = PxRevoluteJointCreate(*physics_, demo.driver, driverFrame, demo.body, bodyFrame);
        joint->setLimit(PxJointAngularLimitPair(-1.2f, 1.2f));
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
        return joint;
    }
    case Kind::Prismatic:
    {
        auto* joint = PxPrismaticJointCreate(*physics_, demo.driver, driverFrame, demo.body, bodyFrame);
        joint->setLimit(PxJointLinearLimitPair(-1.25f, 1.25f, PxSpring(80.0f, 12.0f)));
        joint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);
        return joint;
    }
    case Kind::D6:
    {
        auto* joint = PxD6JointCreate(*physics_, demo.driver, driverFrame, demo.body, bodyFrame);
        joint->setMotion(PxD6Axis::eX, PxD6Motion::eFREE);
        joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
        return joint;
    }
    }
    return nullptr;
}

void JointWorld::step()
{
    scene_->simulate(fixedStep);
    scene_->fetchResults(true);
}

void JointWorld::reset()
{
    endDrag();
    for (auto& demo : demos_)
    {
        demo.driver->setGlobalPose(demo.initialDriverPose);
        demo.body->setGlobalPose(demo.initialBodyPose);
        demo.driver->setLinearVelocity(PxVec3(0));
        demo.driver->setAngularVelocity(PxVec3(0));
        demo.body->setLinearVelocity(PxVec3(0));
        demo.body->setAngularVelocity(PxVec3(0));
        demo.body->clearForce(PxForceMode::eFORCE);
        demo.body->clearTorque(PxForceMode::eFORCE);
        demo.driver->wakeUp();
        demo.body->wakeUp();
    }
}

bool JointWorld::beginDrag(const PxVec3& origin, const PxVec3& unitDir,
                           DemoId& demoId, PxVec3& hitPoint)
{
    endDrag();
    if (!scene_ || !origin.isFinite() || !unitDir.isFinite() || unitDir.magnitudeSquared() <= 0.0f)
        return false;
    PxRaycastBuffer hit;
    if (!scene_->raycast(origin, unitDir.getNormalized(), PX_MAX_F32, hit) || !hit.hasBlock)
        return false;
    for (std::size_t i = 0; i < demos_.size(); ++i)
    {
        if (hit.block.actor == demos_[i].driver)
        {
            demoId = static_cast<DemoId>(i);
            demos_[i].dragging = true;
            activeDemo_ = demoId;
            hitPoint = hit.block.position;
            return true;
        }
    }
    return false;
}

void JointWorld::moveDrag(DemoId demoId, const PxVec3& target)
{
    const auto index = static_cast<std::size_t>(demoId);
    if (index >= demos_.size() || !demos_[index].dragging || !target.isFinite()) return;
    demos_[index].driver->setKinematicTarget(PxTransform(target, demos_[index].initialDriverPose.q));
}

void JointWorld::endDrag()
{
    for (auto& demo : demos_) demo.dragging = false;
    activeDemo_.reset();
}

float JointWorld::jointValue(const Demo& demo) const
{
    if (const auto* distance = demo.joint->is<PxDistanceJoint>()) return distance->getDistance();
    if (const auto* spherical = demo.joint->is<PxSphericalJoint>())
        return std::sqrt(std::pow(spherical->getSwingYAngle(), 2.0f) +
                         std::pow(spherical->getSwingZAngle(), 2.0f));
    if (const auto* revolute = demo.joint->is<PxRevoluteJoint>()) return revolute->getAngle();
    if (const auto* prismatic = demo.joint->is<PxPrismaticJoint>()) return prismatic->getPosition();
    if (const auto* d6 = demo.joint->is<PxD6Joint>()) return d6->getRelativeTransform().p.x;
    return 0.0f;
}
