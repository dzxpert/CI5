#pragma once

#include "feature.h"
#include "../game/game_structures.h"

namespace Features {

class WeaponFeature : public IFeature {
public:
    const char* GetId() const override { return "weapons"; }
    const char* GetName() const override { return "Weapon Arsenal & Switcher"; }
    const char* GetCategory() const override { return "Weapons"; }

    void OnTick() override;
    void OnRenderUI() override;

private:
    int m_selectedWeaponId = 0;
    int m_selectedPowerLevel = 11; // Power Level 12 (0-based)

    bool m_rapidFireEnabled = false;
    double m_customFireInterval = 0.05; // 20 shots/sec
};

} // namespace Features
