export module instancing_example:grass;

import std;
import boza;
using namespace boza;

export void create_grass_blade_mesh()
{
    Mesh::create_obj("grass_blade", "instancing_example/grass_blade.obj");
}
