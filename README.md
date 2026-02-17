# Planetoid Entity Component System

A fast, lightweight C++17 Entity Component System made as a hobby project.

## Requirements

- C++ 17 or newer
- An STL with typeindex, unordered_map and vector

## Features

- Sparse Set based Entity Component System
- Fast Create / Delete Entity with Entity ID reusing
- O(1) Time Complexity component querying, adding and removing
- Multi-Component query and iteration
- Contiguous Component storage for fast iteration 

## Planned Features

In no particular order:
- Entity Versioning
- System Scheduling
- More robust component iteration utilities
- Debugging tools
- Custom memory allocator/deallocator
- Examples
- C / STL-free implementation

## Installation

Simply drop PECS.hpp into your project and #include

## Usage

```cpp

// PECS Entity ID Type defaults to size_t but can be changed by defining PECS_CUSTOM_TYPE before including PECS.hpp
// This is optional
#include <cstdint>
#define PECS_CUSTOM_TYPE uint64_t

#include <PECS.hpp>

// Components can be any struct or class that is move-constructible and move-assignable
struct MyComponent
{
    MyComponent(float v)
        : Value(v) {}
    float Value = 0.0f;
};

PECS::ECSInstance ECS(1000); // Create an ECS Instance, Max Entities = 1000
PECS::EntityID Entity = ECS.CreateEntity(); // Returns an ID

// AddComponent<> takes type as template, Entity ID to add to and the constructor parameters
// Returns a reference to the component so can be retrieved as auto& or explicit typing
ECS.AddComponent<MyComponent>(Entity, 12.5f); 

// Or use GetComponent<>() as per below
MyComponent& Component = ECS.GetComponent<MyComponent>(Entity);

// HasComponent<>() returns whether the entity has the given component.
bool EntityHasMyComponent = ECS.HasComponent<MyComponent>(Entity); // Returns true here

// Sparse Sets allow iteration through the dense component storage. Each entry is a private struct containing EntityID and type, so you can do either of the following:
for (auto& [entity, component] : ECS.Iterate<MyComponent>())
{
    ...
}

for (auto entry : ECS.Iterate<MyComponent>())
{
    EntityID entity = entry.entity;
    MyComponent& component = entry.data;
    ...
}

// Want to iterate through entities with a set of components? Use a View!
// Imagine CompA, CompB CompC as components for this example...
for (auto& [entity, myCompA, myCompB, myCompC] : ECS.View<CompA, CompB, CompC>())
{
    ...
}

// RemoveComponent<> does exactly what you'd expect - removes the component.
ECS.RemoveComponent<MyComponent>(Entity);

// And DeleteEntity also does what it says on the tin. This will also remove all components from all sparse sets associated to this entity
ECS.DeleteEntity(Entity);
```

This library implements a macro called PECS_ASSERT for assertions - feel free to define this before including to replace functionality

## Contributing

Not accepting external contributions yet, but this may change as time goes on - watch this space!

## License

[LICENSE](license) - MIT