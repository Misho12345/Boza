export module boza.rhi.objects:graphics_object;

import std;

export namespace boza::rhi
{
    template<typename Derived, typename Desc>
    class GraphicsObject
    {
    public:
        virtual ~GraphicsObject() = default;

        virtual bool init() = 0;
        virtual void destroy() = 0;

        template<typename Concrete>
            requires (std::same_as<Derived, Concrete> ||
                std::is_base_of_v<Derived, Concrete> && std::is_abstract_v<Derived>)
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

    protected:
        explicit GraphicsObject(const Desc& desc) : desc_(desc) {}
        Desc desc_;
    };
}
