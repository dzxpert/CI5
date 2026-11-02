#pragma once

#include <string>

namespace Features {

class IFeature {
public:
    virtual ~IFeature() = default;

    virtual const char* GetId() const = 0;
    virtual const char* GetName() const = 0;
    virtual const char* GetCategory() const = 0;

    virtual void OnTick() {}
    virtual void OnRenderUI() {}

    virtual bool IsEnabled() const { return m_enabled; }
    virtual void SetEnabled(bool enabled) { m_enabled = enabled; }

protected:
    bool m_enabled = false;
};

} // namespace Features
