#pragma once
#include <type_traits>

namespace boza
{
    namespace ahi { class Factory; }

    template<typename Derived, typename Desc>
    class AudioObject
    {
    public:
        virtual ~AudioObject() = default;

        virtual bool init() = 0;
        virtual void destroy() = 0;

    protected:
        explicit AudioObject(const Desc& desc) : desc(desc) {}

        Desc desc;

    private:
        template<typename Concrete> requires std::is_base_of_v<Derived, Concrete>
        static Derived* create(const Desc& desc)
        {
            const auto ptr = new Concrete(desc);

            if (!ptr->init())
            {
                delete ptr;
                return nullptr;
            }

            return ptr;
        }

        friend class ahi::Factory;
    };
}
