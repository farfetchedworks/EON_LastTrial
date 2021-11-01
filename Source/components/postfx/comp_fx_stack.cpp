#include "mcv_platform.h"
#include "comp_fx_stack.h"
#include "comp_edge_detector.h"

DECL_OBJ_MANAGER("fx_stack", TCompFXStack);

CTexture* TCompFXStack::currentRT = nullptr;
CTexture* TCompFXStack::lastRT = nullptr;

void TCompFXStack::load(const json& j, TEntityParseContext& ctx) {
	enabled = j.value("enabled", enabled);
}

CTexture* TCompFXStack::render(CTexture* rt, CTexture* last_rt)
{
	if (!enabled)
		return rt;

	currentRT = rt;
	lastRT = last_rt;

	for (auto fx : fx_list)
	{
		CEntity* owner = fx.second.getOwner();
		owner->sendMsg(TMsgRenderPostFX({ fx.second }));
	}

	return currentRT;
}

void TCompFXStack::debugInMenu() {

	ImGui::Text("Num VFX: %d", fx_list.size());
	ImGui::Checkbox("Enabled", &enabled);

	static int g_id = 0;
	g_id = 0;

	for (auto fxInfo : fx_list)
	{
		CHandle fx = fxInfo.second;
		ImGui::Separator();

		if (ImGui::TreeNode(fx.getTypeName()))
		{
			auto hm = CHandleManager::getByName(fx.getTypeName());
			hm->debugInMenu(fx);
			ImGui::TreePop();
		}

		if (fx != fx_list[0].second)
		{
			ImGui::SameLine();
			ImGui::PushID(g_id++);	
			if(ImGui::SmallButton("Up"))
				reorder(fx, false);
			ImGui::PopID();
		}

		if (fx != last_fx)
		{
			ImGui::SameLine();
			ImGui::PushID(g_id++);
			if(ImGui::SmallButton("Down"))
				reorder(fx, true);
			ImGui::PopID();
		}
	}
}

void TCompFXStack::reorder(CHandle fx, bool dir)
{
	auto it = std::find_if(begin(fx_list), end(fx_list), [fx](std::pair<int, CHandle> fxInfo) {
		return fxInfo.second == fx;
	});

	// move up
	if (!dir)
	{
		std::swap(*(it - 1), *it);
	}
	// move down
	else
	{
		std::swap(*it, *(it + 1));
	}

	int num_fx = (int)fx_list.size();
	last_fx = fx_list[num_fx - 1].second;
}

bool sortByPriority(std::pair<int, CHandle> a, std::pair<int, CHandle> b) {
	return a.first < b.first;
}

void TCompFXStack::sort()
{
	std::sort(fx_list.begin(), fx_list.end(), sortByPriority);

	int num_fx = (int)fx_list.size();
	last_fx = fx_list[num_fx - 1].second;
}

void TCompFXStack::onNewFx(const TMsgFXToStack& msg)
{
	if (msg.fx.isValid())
	{
		fx_list.push_back(std::make_pair(msg.priority, msg.fx));
		sort();
	}
}