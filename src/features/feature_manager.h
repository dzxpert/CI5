#pragma once

#include "feature.h"
#include <vector>
#include <memory>
#include <map>
#include <string>

namespace Features {

class FeatureManager {
public:
    static FeatureManager& Get();

    void Initialize();
    void RegisterFeature(std::shared_ptr<IFeature> feature);
    
    void OnTick();
    void RenderUI();

    const std::vector<std::shared_ptr<IFeature>>& GetFeatures() const { return m_features; }

    template<typename T>
    std::shared_ptr<T> GetFeature() {
        for (const auto& feat : m_features) {
            auto casted = std::dynamic_pointer_cast<T>(feat);
            if (casted) return casted;
        }
        return nullptr;
    }

private:
    FeatureManager() = default;
    ~FeatureManager() = default;

    std::vector<std::shared_ptr<IFeature>> m_features;
    bool m_initialized = false;
};

} // namespace Features
