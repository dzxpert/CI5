#pragma once

#include "feature.h"

namespace Features {

class TeleportFeature : public IFeature {
public:
    const char* GetId() const override { return "teleport"; }
    const char* GetName() const override { return "Position & Teleport"; }
    const char* GetCategory() const override { return "Movement"; }

    void OnTick() override;
    void OnRenderUI() override;

private:
    float m_targetX = 0.0f;
    float m_targetY = -0.6f;
};

} // namespace Features
