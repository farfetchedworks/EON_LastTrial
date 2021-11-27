#include "mcv_platform.h"
#include "comp_color_grading.h"

DECL_OBJ_MANAGER("color_grading", TCompColorGrading);

// Export with the photoshop plugin: (v8.55.0109.1800)
// - Volume Texture
// - No Mip Maps
// - ARGB 32 bpp unsigned

// ---------------------
void TCompColorGrading::debugInMenu() {
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::DragFloat("Amount", &amount, 0.01f, 0.0f, 1.0f);
}

void TCompColorGrading::load(const json& j, TEntityParseContext& ctx) {
	enabled = j.value("enabled", enabled);
	amount = j.value("amount", amount);
	std::string lut_name = j.value("lut", "");
	lut = Resources.get(lut_name)->as<CTexture>();
}

void TCompColorGrading::fadeIn(float time, const interpolators::IInterpolator* inter)
{
	fadeTime = time;
	enabled = true;
	state = true;
	timer = 0.f;
	amount = 0.f;
	interpolator = inter;
}

void TCompColorGrading::fadeOut(float time, const interpolators::IInterpolator* inter)
{
	fadeTime = time;
	enabled = true;
	state = false;
	timer = fadeTime;
	amount = 1.f;
	interpolator = inter;
}

void TCompColorGrading::update(float dt)
{
	if (!enabled)
		return;

	timer += (state ? dt : -dt);

	float f = clampf(timer / fadeTime, 0.f, 1.f);
	if (interpolator)
		f = interpolator->blend(0.f, 1.f, f);

	amount = f;
}