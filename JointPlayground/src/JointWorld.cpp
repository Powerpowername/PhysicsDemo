#include "JointWorld.hpp"

#include <stdexcept>

using namespace physx;

namespace
{
PxFilterFlags contactFilter(PxFilterObjectAttributes, PxFilterData, PxFilterObjectAttributes,
                            PxFilterData, PxPairFlags& pairs, const void*, PxU32)
{
    pairs = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eDETECT_CCD_CONTACT;
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
    desc.filterShader = contactFilter;
    desc.flags |= PxSceneFlag::eENABLE_CCD;
    scene_ = physics_->createScene(desc);
    if (!scene_) throw std::runtime_error("Could not create joint scene");
    material_ = physics_->createMaterial(0.55f, 0.45f, 0.05f);
    if (!material_) throw std::runtime_error("Could not create joint material");
    ground_ = PxCreatePlane(*physics_, PxPlane(0, 1, 0, 0), *material_);
    if (!ground_ || !scene_->addActor(*ground_)) throw std::runtime_error("Could not add ground");

    const PxVec3 positions[] = {
        {-4.0f, 5.6f, 0}, {0, 5.6f, 0}, {4.0f, 5.6f, 0},
        {-4.0f, 2.4f, 0}, {0, 2.4f, 0}, {4.0f, 2.4f, 0},
    };
    for (std::size_t i = 0; i < demos_.size(); ++i) createDemo(demos_[i], positions[i]);
}

void JointWorld::createParts(Demo& demo, const PxTransform& driverPose, const PxVec3& driverHalfExtents,
                             const PxVec3& bodyOffset, const PxVec3& bodyHalfExtents)
{
    demo.initialDriverPose = driverPose;
    demo.initialBodyPose = PxTransform(driverPose.p + bodyOffset);
    demo.driverHalfExtents = driverHalfExtents;
    demo.bodyHalfExtents = bodyHalfExtents;
    if (demo.kind == Kind::Revolute || demo.kind == Kind::D6)
    {
        // PhysX capsules extend along local X; their total half-length includes the end cap.
        const float radius = driverHalfExtents.y;
        const PxCapsuleGeometry axle(radius, driverHalfExtents.x - radius);
        demo.driver = PxCreateDynamic(*physics_, driverPose, axle, *material_, 1.0f);
    }
    else
        demo.driver = PxCreateDynamic(*physics_, driverPose, PxBoxGeometry(driverHalfExtents), *material_, 1.0f);

    demo.body = PxCreateDynamic(*physics_, demo.initialBodyPose, PxBoxGeometry(bodyHalfExtents), *material_, 1.0f);
    if (!demo.driver || !demo.body) throw std::runtime_error("Could not create joint demo bodies");
    demo.driver->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    demo.body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    demo.driver->setName(demo.name);
    demo.body->setName(demo.name);
    if (!scene_->addActor(*demo.driver) || !scene_->addActor(*demo.body))
        throw std::runtime_error("Could not add joint demo bodies");
}

void JointWorld::createDemo(Demo& demo, const PxVec3& position)
{
    const PxTransform mount(position);
    const PxTransform origin(PxIdentity);
    switch (demo.kind)
    {
    case Kind::Fixed:
    {
        // The two plate ends meet at x = +0.42 on the mount.
        createParts(demo, mount, {0.42f, 0.28f, 0.24f}, {1.04f, 0, 0}, {0.62f, 0.16f, 0.24f});
        demo.joint = PxFixedJointCreate(*physics_, demo.driver, PxTransform(0.42f, 0, 0),
                                       demo.body, PxTransform(-0.62f, 0, 0));
        break;
    }
    case Kind::Distance:
    {
        createParts(demo, mount, {0.42f, 0.15f, 0.25f}, {0, -1.0f, 0}, {0.29f, 0.29f, 0.29f});
        auto* joint = PxDistanceJointCreate(*physics_, demo.driver, origin, demo.body, origin);
        if (!joint) break;
        joint->setMinDistance(0.75f);
        joint->setMaxDistance(1.25f);
        joint->setTolerance(0.001f); // Keep the displayed length within a millimetre of the limit at rest.
        joint->setDistanceJointFlags(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED |
                                     PxDistanceJointFlag::eMAX_DISTANCE_ENABLED);
        demo.joint = joint;
        break;
    }
    case Kind::Spherical:
    {
        createParts(demo, mount, {0.50f, 0.16f, 0.26f}, {0, -0.81f, 0}, {0.15f, 0.65f, 0.15f});
        // The limit cone is centered on the joint's X axis; point it down the arm.
        const PxQuat down(-PxHalfPi, PxVec3(0, 0, 1));
        auto* joint = PxSphericalJointCreate(*physics_, demo.driver, PxTransform({0, -0.16f, 0}, down),
                                             demo.body, PxTransform({0, 0.65f, 0}, down));
        if (!joint) break;
        joint->setLimitCone(PxJointLimitCone(0.85f, 0.85f));
        joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
        demo.joint = joint;
        break;
    }
    case Kind::Revolute:
    {
        // The pin and joint X axes point toward the camera (+Z); the lever swings in XY.
        const PxQuat towardCamera(-PxHalfPi, PxVec3(0, 1, 0));
        const float radius = 0.19f;
        createParts(demo, PxTransform(position, towardCamera),
                    {radius * (shaftHalfLengthInRadii + 1), radius, radius},
                    {0, -0.52f, 0}, {0.17f, 0.70f, 0.17f});
        auto* joint = PxRevoluteJointCreate(*physics_, demo.driver, origin,
                                            demo.body, PxTransform({0, 0.52f, 0}, towardCamera));
        if (!joint) break;
        joint->setLimit(PxJointAngularLimitPair(-1.2f, 1.2f));
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
        demo.joint = joint;
        break;
    }
    case Kind::Prismatic:
    {
        // The carriage rests on the upper face of the rail and travels along its X axis.
        createParts(demo, mount, {1.58f, 0.12f, 0.21f}, {0, 0.38f, 0}, {0.31f, 0.26f, 0.28f});
        auto* joint = PxPrismaticJointCreate(*physics_, demo.driver, PxTransform(0, 0.12f, 0),
                                             demo.body, PxTransform(0, -0.26f, 0));
        if (!joint) break;
        joint->setLimit(PxJointLinearLimitPair(-1.25f, 1.25f, PxSpring(80.0f, 12.0f)));
        joint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);
        demo.joint = joint;
        break;
    }
    case Kind::D6:
    {
        const float radius = 0.22f;
        createParts(demo, mount, {radius * (shaftHalfLengthInRadii + 1), radius, radius},
                    {0, -0.48f, 0}, {0.20f, 0.66f, 0.20f});
        auto* joint = PxD6JointCreate(*physics_, demo.driver, origin, demo.body, PxTransform(0, 0.48f, 0));
        if (!joint) break;
        joint->setMotion(PxD6Axis::eX, PxD6Motion::eFREE);
        joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
        demo.joint = joint;
        break;
    }
    }
    if (!demo.joint) throw std::runtime_error("Could not create joint demo joint");
}

void JointWorld::step()
{
    if (activeDemo_)
    {
        // A large mouse jump must not give the coupled body an arbitrarily large velocity.
        const auto& demo = demos_[static_cast<std::size_t>(*activeDemo_)];
        PxTransform pose = demo.driver->getGlobalPose();
        const PxVec3 delta = dragTarget_ - pose.p;
        constexpr float maxStep = 5.0f * fixedStep; // metres per physics tick at 5 m/s
        const float length = delta.magnitude();
        pose.p += length > maxStep ? delta * (maxStep / length) : delta;
        demo.driver->setKinematicTarget(pose);
    }
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
        demo.driver->setKinematicTarget(demo.initialDriverPose);
        demo.body->setLinearVelocity(PxVec3(0));
        demo.body->setAngularVelocity(PxVec3(0));
        demo.body->clearForce(PxForceMode::eFORCE);
        demo.body->clearTorque(PxForceMode::eFORCE);
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
            dragTarget_ = demos_[i].driver->getGlobalPose().p;
            grabOffset_ = dragTarget_ - hitPoint;
            return true;
        }
    }
    return false;
}

void JointWorld::moveDrag(DemoId demoId, const PxVec3& target)
{
    const auto index = static_cast<std::size_t>(demoId);
    if (index >= demos_.size() || !demos_[index].dragging || !target.isFinite()) return;
    const auto& demo = demos_[index];
    PxVec3 center = target + grabOffset_;
    // Don't prescribe a kinematic pose that would force the assembled parts below the floor.
    const float dropToBody = demo.kind == Kind::Distance
        ? demo.joint->is<PxDistanceJoint>()->getMaxDistance()
        : demo.initialDriverPose.p.y - demo.initialBodyPose.p.y;
    center.y = PxMax(center.y, PxMax(demo.driverHalfExtents.y, dropToBody + demo.bodyHalfExtents.y));
    dragTarget_ = center;
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
        // x dot (relative rotation * x) is the cosine of the arm's tilt.
        return PxAcos(PxClamp(spherical->getRelativeTransform().q.getBasisVector0().x, -1.0f, 1.0f));
    if (const auto* revolute = demo.joint->is<PxRevoluteJoint>()) return revolute->getAngle();
    if (const auto* prismatic = demo.joint->is<PxPrismaticJoint>()) return prismatic->getPosition();
    if (const auto* d6 = demo.joint->is<PxD6Joint>()) return d6->getRelativeTransform().p.x;
    return 0.0f;
}
