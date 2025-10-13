#include "boza/core/Layer.hpp"
#include "boza/core/LayerManager.hpp"

namespace boza
{
    Layer::Layer(const std::string& name) : mask_(LayerManager::instance().get_layer_mask(name)) {}

    Layer& Layer::operator=(const uint32_t _mask)
    {
        mask_ = _mask;
        return *this;
    }

    Layer& Layer::operator=(const std::string& _name)
    {
        mask_ = LayerManager::instance().get_layer_mask(_name);
        return *this;
    }

    Layer Layer::operator|(const Layer& other) const { return Layer(mask_ | other.mask_); }
    Layer Layer::operator&(const Layer& other) const { return Layer(mask_ & other.mask_); }
    Layer Layer::operator^(const Layer& other) const { return Layer(mask_ ^ other.mask_); }
    Layer Layer::operator~() const { return Layer(~mask_); }

    Layer& Layer::operator|=(const Layer& other)
    {
        mask_ |= other.mask_;
        return *this;
    }

    Layer& Layer::operator&=(const Layer& other)
    {
        mask_ &= other.mask_;
        return *this;
    }

    Layer& Layer::operator^=(const Layer& other)
    {
        mask_ ^= other.mask_;
        return *this;
    }


    bool Layer::contains(const Layer& other) const { return (mask_ & other.mask_) != 0; }

    bool Layer::contains(const std::string& _name) const
    {
        const uint32_t layer_mask = LayerManager::instance().get_layer_mask(_name);
        return (mask_ & layer_mask) != 0;
    }

    bool Layer::operator==(const Layer& other) const { return mask_ == other.mask_; }
    bool Layer::operator!=(const Layer& other) const { return mask_ != other.mask_; }
    bool Layer::operator!=(const std::string& _name) const { return !(*this == _name); }

    bool Layer::operator==(const std::string& _name) const
    {
        return mask_ == LayerManager::instance().get_layer_mask(_name);
    }

    std::string Layer::get_name() const { return LayerManager::instance().get_layer_name(mask_); }
}
