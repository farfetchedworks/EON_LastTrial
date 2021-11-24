#pragma once

#include "resources/resources.h"

class CCurve : public IResource
{
public:
    bool parse(const json& jFile);
    bool renderInMenu() const override;
    void renderDebug(const MAT44& world, const VEC4& color, int numSamples = 100) const;
    
    void addKnot(const VEC3& pos);
    VEC3 evaluate(float ratio, const MAT44& world) const;
    VEC3 evaluate(float ratio) const;

    VEC3 getKnot(int idx) const;

private:
    mutable bool _loop = false;
    mutable std::vector<VEC3> _knots;
};
