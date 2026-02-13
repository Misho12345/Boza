module boza.input;

import :keys;

namespace boza
{
    KeyCombo operator&(const Key lhs, const Key rhs) { return KeyCombo{ std::vector{ lhs, rhs } }; }

    KeyCombo operator&(KeyCombo lhs, const Key rhs)
    {
        lhs.keys.push_back(rhs);
        return lhs;
    }

    KeyCombo operator&(const Key lhs, KeyCombo rhs)
    {
        rhs.keys.insert(rhs.keys.begin(), lhs);
        return rhs;
    }

    KeyBinding operator|(const Key lhs, const Key rhs)
    {
        return KeyBinding{ std::vector{ KeyCombo{ lhs }, KeyCombo{ rhs } } };
    }

    KeyBinding operator|(KeyCombo lhs, const Key rhs)
    {
        return KeyBinding{ std::vector{ std::move(lhs), KeyCombo{ rhs } } };
    }

    KeyBinding operator|(const Key lhs, KeyCombo rhs)
    {
        return KeyBinding{ std::vector{ KeyCombo{ lhs }, std::move(rhs) } };
    }

    KeyBinding operator|(KeyCombo lhs, KeyCombo rhs)
    {
        return KeyBinding{ std::vector{ std::move(lhs), std::move(rhs) } };
    }

    KeyBinding operator|(KeyBinding lhs, const Key rhs)
    {
        lhs.combos.push_back(KeyCombo{ rhs });
        return lhs;
    }

    KeyBinding operator|(KeyBinding lhs, KeyCombo rhs)
    {
        lhs.combos.push_back(std::move(rhs));
        return lhs;
    }

    KeyBinding operator|(const Key lhs, KeyBinding rhs)
    {
        rhs.combos.insert(rhs.combos.begin(), KeyCombo{ lhs });
        return rhs;
    }

    KeyBinding operator|(KeyCombo lhs, KeyBinding rhs)
    {
        rhs.combos.insert(rhs.combos.begin(), std::move(lhs));
        return rhs;
    }

    KeyBinding operator|(KeyBinding lhs, KeyBinding rhs)
    {
        lhs.combos.insert(lhs.combos.end(),
                          std::make_move_iterator(rhs.combos.begin()),
                          std::make_move_iterator(rhs.combos.end()));
        return lhs;
    }
}
