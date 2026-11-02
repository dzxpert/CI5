#pragma once

#include "feature.h"

namespace Features {

class StatsFeature : public IFeature {
public:
    const char* GetId() const override { return "stats"; }
    const char* GetName() const override { return "Player Inspector & Stats"; }
    const char* GetCategory() const override { return "Player"; }

    void OnTick() override;
    void OnRenderUI() override;

private:
    int m_customLives = 99;
    int m_customMissiles = 99;
    int m_customScore = 1000000;
    int m_customFood = 500;
    int m_customKeys = 1000;
};

} // namespace Features
