#ifndef _PECS_HPP_
#define _PECS_HPP_

#include <typeindex>
#include <unordered_map>
#include <vector>

namespace PECS
{

#ifndef PECS_CUSTOM_TYPE
    #define PECS_CUSTOM_TYPE size_t
#endif
    using EntityID = PECS_CUSTOM_TYPE;

#ifndef PECS_ASSERT
    #include <cassert>
    #define PECS_ASSERT assert
#endif

    template <typename T>
    class SparseSet
    {
    public:
        SparseSet(size_t MaxEntities)
            : m_MaxEntities(MaxEntities) 
        {
            m_SparseSet.resize(MaxEntities, MaxEntities - 1);
        }

        ~SparseSet() {}

        T& Get(EntityID entity)
        {
            PECS_ASSERT(entity < m_MaxEntities && "Entity ID out of range");
            PECS_ASSERT(Has(entity) && "Component not found on entity");

            return m_DenseSet[m_SparseSet[entity]].data;
        }

        template <typename... Args>
        T& Add(EntityID entity, Args&&... args)
        {
            PECS_ASSERT(entity < m_MaxEntities && "Entity ID out of range");

            if (Has(entity))
            {
                return m_DenseSet[m_SparseSet[entity]].data;
            }

            m_DenseSet.push_back({ entity, T(std::forward<Args>(args)...) });
            m_SparseSet[entity] = m_DenseSet.size() - 1;

            return m_DenseSet.back().data;
        }

        bool Has(EntityID entity) const
        {
            PECS_ASSERT(entity < m_MaxEntities && "Entity ID out of range");

            return m_SparseSet[entity] != m_MaxEntities - 1;
        }

        bool Remove(EntityID entity)
        {
            PECS_ASSERT(entity < m_MaxEntities && "Entity ID out of range");

            if (!Has(entity))
            {
                return false;
            }

            size_t index = m_SparseSet[entity];
            size_t last = m_DenseSet.size() - 1;
            DenseSetEntry& lastEntry = m_DenseSet[last];
            m_DenseSet[index] = std::move(lastEntry);

            m_SparseSet[lastEntry.entity] = index;
            m_DenseSet.pop_back();
            m_SparseSet[entity] = m_MaxEntities - 1;

            return true;
        }

        auto begin() { return m_DenseSet.begin(); }
        auto end()   { return m_DenseSet.end(); }

        auto begin() const { return m_DenseSet.begin(); }
        auto end()   const { return m_DenseSet.end(); }

        
    private:
        size_t m_MaxEntities;
        std::vector<EntityID> m_SparseSet;

        struct DenseSetEntry
        {
            EntityID entity;
            T data;
        };

        std::vector<DenseSetEntry> m_DenseSet;
    };

    struct ISparseSetWrapper
    {
        virtual ~ISparseSetWrapper() = default;

        virtual bool HasEntity(EntityID entity) const = 0;
        virtual bool RemoveEntity(EntityID entity) = 0;
    };

    template <typename T>
    struct SparseSetWrapper : public ISparseSetWrapper
    {
        SparseSet<T> set;

        SparseSetWrapper(size_t MaxEntities)
            : set(MaxEntities) {}

        virtual bool HasEntity(EntityID entity) const override { return set.Has(entity); }
        virtual bool RemoveEntity(EntityID entity) override { return set.Remove(entity); }
    };

    class ECSInstance
    {
    public:
        ECSInstance(unsigned int MaxEntities)
            : m_MaxEntities(MaxEntities) {}
        ~ECSInstance() 
        {
            for (auto& [type, pool] : m_ComponentPools)
            {
                delete pool;
            }
            m_ComponentPools.clear();
        }

        EntityID CreateEntity()
        {
            if (m_FreeEntities.empty())
            {
                PECS_ASSERT(m_NextEntity + 1 < m_MaxEntities && "Max entities reached");
                return m_NextEntity++;
            }
            else 
            {
                EntityID id = m_FreeEntities.back();
                m_FreeEntities.pop_back();
                return id;
            }
        }

        void DeleteEntity(EntityID entity)
        {
            PECS_ASSERT(entity < m_MaxEntities && "DeleteEntity called with invalid entity");

            m_FreeEntities.push_back(entity);
            for (auto&[type, pool] : m_ComponentPools)
            {
                pool->RemoveEntity(entity);
            }
        }

        template <typename T, typename... Args>
        T& AddComponent(EntityID entity, Args&&... args)
        {
            PECS_ASSERT(entity < m_MaxEntities && "AddComponent called with invalid Entity ID");

            return GetOrCreatePool<T>().Add(entity, std::forward<Args>(args)...);
        }

        template <typename T>
        T& GetComponent(EntityID entity)
        {
            PECS_ASSERT(entity < m_MaxEntities && "AddComponent called with invalid Entity ID");

            SparseSet<T>* pool = TryGetPool<T>();

            PECS_ASSERT(pool && "Component type does not exist");
            PECS_ASSERT(pool->Has(entity) && "Entity does not have component");

            return pool->Get(entity);
        }

        template <typename... Ts>
        std::tuple<Ts&...> GetComponents(EntityID entity)
        {
            return std::tuple<Ts&...>(GetComponent<Ts>(entity)...);
        }

        template <typename T>
        bool RemoveComponent(EntityID entity)
        {
            PECS_ASSERT(entity < m_MaxEntities && "RemoveComponent called with invalid Entity ID");

            if (auto* pool = TryGetPool<T>())
            {
                return pool->Remove(entity);
            }
            return false;
        }

        template <typename T>
        bool HasComponent(EntityID entity) const 
        {
            PECS_ASSERT(entity < m_MaxEntities && "HasComponent called with invalid Entity ID");

            if (auto* pool = TryGetPool<T>())
            {
                return pool->Has(entity);
            }
            return false;
        }

        template <typename... Ts>
        bool HasComponents(EntityID entity) const 
        {
            return (HasComponent<Ts>(entity) && ...);
        }

        template<typename T>
        SparseSet<T>& Iterate()
        {
            return GetOrCreatePool<T>();
        }


    private:
        unsigned int m_MaxEntities;
        EntityID m_NextEntity = 0;
        std::vector<EntityID> m_FreeEntities;

        std::unordered_map<std::type_index, ISparseSetWrapper*> m_ComponentPools;

        template <typename T>
        SparseSet<T>& GetOrCreatePool()
        {
            std::type_index key(typeid(T));
            auto it = m_ComponentPools.find(key);
            if (it != m_ComponentPools.end())
            {
                return static_cast<SparseSetWrapper<T>*>(it->second)->set;
            }

            auto* wrapper = new SparseSetWrapper<T>(m_MaxEntities);
            m_ComponentPools.emplace(key, wrapper);
            return wrapper->set;
        }

        template <typename T>
        SparseSet<T>* TryGetPool()
        {
            std::type_index key(typeid(T));
            auto it = m_ComponentPools.find(key);
            if (it != m_ComponentPools.end())
            {
                return &static_cast<SparseSetWrapper<T>*>(it->second)->set;
            }

            return nullptr;
        }

    };
}

#endif