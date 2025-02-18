#include "GameObject.hpp"

namespace boza
{
    GameObject::GameObject() : entity{ registry.create() } {}
    GameObject::~GameObject() { registry.destroy(entity); }
}
