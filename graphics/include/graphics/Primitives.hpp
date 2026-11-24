#pragma once

#include <graphics/Mesh.hpp>

namespace graphics::primitives
{
// Shared primitive data. The returned objects are immutable and live for the process lifetime.
const MeshData& box();
const MeshData& sphere();
}
