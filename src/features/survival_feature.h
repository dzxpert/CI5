#pragma once

#include "feature.h"

namespace Features {

class SurvivalFeature : public IFeature {
public:
    const char* GetId() const override { return "survival"; }
    const char* GetName() const override { return "God Mode & Cheats"; }
    const char* GetCategory() const override { return "Cheats"; }

    void OnTick() override;
    void OnRenderUI() override;

    bool IsGodMode() const { return m_godMode; }
    void SetGodMode(bool enable) { m_godMode = enable; }

    bool IsInfiniteLives() const { return m_infiniteLives; }
    void SetInfiniteLives(bool enable) { m_infiniteLives = enable; }

    bool IsInfiniteMissiles() const { return m_infiniteMissiles; }
    void SetInfiniteMissiles(bool enable) { m_infiniteMissiles = enable; }

    bool IsNeverOverheat() const { return m_neverOverheat; }
    void SetNeverOverheat(bool enable) { m_neverOverheat = enable; }

private:
    bool m_godMode = false;
    bool m_infiniteLives = false;
    bool m_infiniteMissiles = false;
    bool m_neverOverheat = false;

    int m_targetLives = 99;
    int m_targetMissiles = 99;
};

} // namespace Features
