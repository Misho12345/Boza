#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class Allocator final : public Singleton<Allocator>
    {
    public:
        [[nodiscard]] static bool create();
        static void destroy();

        [[nodiscard]] static VmaAllocator& get_vma_allocator();

    private:
        VmaAllocator allocator{ nullptr };

        friend Singleton;
        Allocator() = default;
    };
}
