module game.terrain;

import :components;
import :config;

namespace game::terrain
{
    RuntimeComponent::RuntimeComponent(const Settings& settings)
    {
        const std::size_t slot_count = std::max<std::size_t>(
            1u,
            std::min(chunk_count(settings), max_concurrent_chunk_jobs));

        slots_.reserve(slot_count);
        in_use_.assign(slot_count, false);
        ready_ = false;

        for (std::size_t index = 0; index < slot_count; ++index)
        {
            auto slot = std::make_unique<ComputeSlot>(settings);
            if (slot->ready()) ready_ = true;
            slots_.push_back(std::move(slot));
        }
    }

    std::optional<std::size_t> RuntimeComponent::acquire_slot()
    {
        for (std::size_t index = 0; index < slots_.size(); ++index)
        {
            if (in_use_[index]) continue;
            if (!slots_[index] || slots_[index]->broken) continue;

            in_use_[index] = true;
            return index;
        }

        return std::nullopt;
    }

    void RuntimeComponent::release_slot(const std::size_t slot_index)
    {
        if (slot_index >= in_use_.size()) return;
        in_use_[slot_index] = false;
    }

    void RuntimeComponent::mark_broken(const std::size_t slot_index)
    {
        if (slot_index >= slots_.size()) return;

        if (slots_[slot_index]) slots_[slot_index]->broken = true;
        release_slot(slot_index);

        ready_ = false;
        for (const auto& slot : slots_)
        {
            if (slot && !slot->broken)
            {
                ready_ = true;
                break;
            }
        }
    }

    void RuntimeComponent::shutdown()
    {
        slots_.clear();
        in_use_.clear();
        ready_ = false;
    }

    ComputeSlot* RuntimeComponent::slot(const std::size_t slot_index)
    {
        if (slot_index >= slots_.size()) return nullptr;
        return slots_[slot_index].get();
    }

    const ComputeSlot* RuntimeComponent::slot(const std::size_t slot_index) const
    {
        if (slot_index >= slots_.size()) return nullptr;
        return slots_[slot_index].get();
    }

    RuntimeComponent::~RuntimeComponent() = default;

    RuntimeComponent::RuntimeComponent(RuntimeComponent&&) noexcept = default;
    RuntimeComponent& RuntimeComponent::operator=(RuntimeComponent&&) noexcept = default;
}
