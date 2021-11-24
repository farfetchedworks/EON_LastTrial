#include "mcv_platform.h"
#include "geometry/curve.h"
#include "render/draw_primitives.h"

class CCurveResourceType : public CResourceType {
public:
    const char* getExtension(int idx) const { return ".curve"; }
    const char* getName() const { return "Curve"; }
    IResource* create(const std::string& name) const
    {
        json jData = loadJson(name);
        if (jData.empty())
        {
            return nullptr;
        }

        CCurve* curve = new CCurve();
        const bool ok = curve->parse(jData);
        assert(ok);
        if(!ok)
        {
            delete curve;
            return nullptr;
        }

        return curve;
    }
};

template<>
CResourceType* getClassResourceType<CCurve>() {
    static CCurveResourceType factory;
    return &factory;
}
// -----------------------------------------

bool CCurve::parse(const json& jFile)
{
    _loop = jFile.value("loop", false);
    for (const json& jKnot : jFile["knots"])
    {
        const VEC3 pos = loadVEC3(jKnot);
        addKnot(pos);
    }
    return true;
}

void CCurve::addKnot(const VEC3& pos)
{
    _knots.push_back(pos);
}

VEC3 CCurve::getKnot(int idx) const
{
    assert(_knots.size() > idx);
    if (_knots.size() > idx)
        return _knots[idx];
    return VEC3();
}

VEC3 CCurve::evaluate(float ratio, const MAT44& world) const
{
    return VEC3::Transform(evaluate(ratio), world);
}

VEC3 CCurve::evaluate(float ratio) const
{
    assert(_knots.size() >= 4);

    const int numKnots = static_cast<int>(_knots.size());
    int numSegments = 1;

    if (_loop)
    {
        ratio = fmodf(ratio, 1.f);
        if (ratio < 0.f)
        {
            ratio = ratio + 1.f;
        }

        numSegments = numKnots;
    }
    else
    {
        if (ratio >= 1.f)
        {
            return _knots.at(numKnots - 2);
        }
        numSegments = numKnots - 3;
    }

    const float ratioPerSegment = 1.f / numSegments;
    const int segmentIdx = static_cast<int>(ratio / ratioPerSegment);
    const float ratioInSegment = fmodf(ratio, ratioPerSegment) / ratioPerSegment;

    const VEC3 v1 = _knots.at(segmentIdx % numKnots);
    const VEC3 v2 = _knots.at((segmentIdx + 1) % numKnots);
    const VEC3 v3 = _knots.at((segmentIdx + 2) % numKnots);
    const VEC3 v4 = _knots.at((segmentIdx + 3) % numKnots);

    const VEC3 pos = VEC3::CatmullRom(v1, v2, v3, v4, ratioInSegment);
    return pos;
}

bool CCurve::renderInMenu() const
{
    ImGui::Checkbox("loop", &_loop);
    if (ImGui::TreeNode("knots"))
    {
        int idx = 0;
        for (auto& knot : _knots)
        {
            ImGui::PushID(idx++);
            ImGui::DragFloat3("knot", &knot.x, 0.1f);
            ImGui::PopID();
        }
        if (ImGui::Button("Add knot"))
        {
            _knots.push_back(VEC3::Zero);
        }
        ImGui::TreePop();
    }
    return true;
}

void CCurve::renderDebug(const MAT44& world, const VEC4& color, int numSamples) const
{
    for (int idx = 1; idx < _knots.size(); ++idx)
    {
        const VEC3 src = VEC3::Transform(_knots.at(idx - 1), world);
        const VEC3 tgt = VEC3::Transform(_knots.at(idx), world);
        drawLine(src, tgt, VEC4::One);
    }

    VEC3 prevPos = evaluate(0.f, world);
    for (int idx = 1; idx <= numSamples; ++idx)
    {
        const VEC3 currPos = evaluate(static_cast<float>(idx) / numSamples, world);
        drawLine(prevPos, currPos, color);
        prevPos = currPos;
    }
}
