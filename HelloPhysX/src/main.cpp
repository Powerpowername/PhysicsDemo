#include <PxPhysicsAPI.h>
#include <iostream>

using namespace physx;

int main()
{
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errorCallback;
    PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
    if (!foundation)
        return 1;

    std::cout << "PhysX environment OK\n";
    foundation->release();
    return 0;
}
