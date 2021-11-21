#include "mcv_platform.h"
#include "engine.h"
#include "module_subtitles.h"
#include "modules/module_events.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "ui/ui_params.h"
#include "ui/effects/ui_effect_fade.h"
#include "audio/module_audio.h"
#include "lua/module_scripting.h"
#include "components/messages.h"

static NamedValues<CModuleSubtitles::EState>::TEntry state_entries[] = {
	 {CModuleSubtitles::EState::STATE_NONE, "none"},
	 {CModuleSubtitles::EState::STATE_IN, "in"},
	 {CModuleSubtitles::EState::STATE_OUT, "out"},
	 {CModuleSubtitles::EState::STATE_ACTIVE, "active"}
};
NamedValues<CModuleSubtitles::EState> state_entries_names(state_entries, sizeof(state_entries) / sizeof(NamedValues<CModuleSubtitles::EState>::TEntry));

bool CModuleSubtitles::start()
{
	const json& j = loadJson("data/ui/subtitles.json");
	assert(j.is_array());

	for (int i = 0; i < j.size(); ++i)
	{
		const json& jItem = j[i];
		parseCaption(jItem);
	}

	return true;
}

void CModuleSubtitles::parseCaption(const json& j)
{
	assert(j.is_object());
	assert(j.count("captions") > 0);

	std::vector<SCaptionParams> captions;

	for (int i = 0; i < j["captions"].size(); ++i)
	{
		const json& jItem = j["captions"][i];
		SCaptionParams caption;
		parseCaptionList(caption, jItem);
		captions.push_back(caption);
	}

	_registeredCaptions[j["name"]] = captions;
	_fadeCaptions = j.value("fadeCaptions", _fadeCaptions);
	
	if (_fadeCaptions)
	{
		ui::CWidget* w = EngineUI.getWidget("eon_subtitles");
		ui::CEffect_FadeIn* fadeInEffect = static_cast<ui::CEffect_FadeIn*>(w->getEffect("Fade In"));
		ui::CEffect_FadeOut* fadeOutEffect = static_cast<ui::CEffect_FadeOut*>(w->getEffect("Fade Out"));
		_fadeThreshold = j.value("fadeThreshold", _fadeThreshold);
		_fadeInTime = fadeInEffect->getFadeTime() + _fadeThreshold;
		_fadeOutTime = fadeOutEffect->getFadeTime() + _fadeThreshold;
	}
}

void CModuleSubtitles::parseCaptionList(SCaptionParams& caption, const json& j)
{
	assert(j.is_object());
	caption.texture = j.value("texture", std::string());
	caption.event = j.value("event", std::string());
	caption.script = j.value("script", std::string());
	caption.audio = j.value("audio", std::string());
	caption.time = j.value("time", 0.f);
	caption.pause = j.value("pause", false);
}

void CModuleSubtitles::update(float dt)
{
	if (!_active)
	return;

	_timer += dt;

	auto& list = _registeredCaptions[_currentCaption];
	SCaptionParams& current = list[_currentIndex];
	
	if (getState() == EState::STATE_OUT)
	{
		if (_timer >= _fadeOutTime)
		{
			_active = setCaptionEntry();
			setState(EState::STATE_IN);
			_timer = 0.f;
			return;
		}
	}

	if (getState() == EState::STATE_IN && _timer >= _fadeInTime)
	{
		setState(EState::STATE_ACTIVE);
		triggerAudio();
	}

	if(_timer >= current.time)
	{
		stopCaptionEntry();
		
		_currentIndex++;
		_timer = 0.f;

		if (_currentIndex == list.size())
		{
			stopCaption();
			return;
		}

		setState(EState::STATE_OUT);
	} 
}

bool CModuleSubtitles::setCaptionEntry()
{
	ui::CWidget* w = EngineUI.getWidget("eon_subtitles");

	assert(w);
	if (!w)
		return false;

	auto& list = _registeredCaptions[_currentCaption];
	ui::TImageParams* imgParams = w->getImageParams();
	const auto& currentCaption = list[_currentIndex];

	if (currentCaption.pause)
	{
		imgParams->texture = nullptr;
	}
	else
	{
		imgParams->texture = Resources.get(currentCaption.texture)->as<CTexture>();

		assert(imgParams->texture);
		if (!imgParams->texture)
			return false;
	}

	return true;
}

bool CModuleSubtitles::stopCaptionEntry()
{
	ui::CWidget* w = EngineUI.getWidget("eon_subtitles");

	assert(w);
	if (!w)
		return false;

	auto& list = _registeredCaptions[_currentCaption];
	ui::TImageParams* imgParams = w->getImageParams();
	const auto& currentCaption = list[_currentIndex];

	// System event?
	if (currentCaption.event.length())
	{
		EventSystem.dispatchEvent(currentCaption.event, _trigger);
	}

	// Lua script?
	if (currentCaption.script.length())
	{
		EngineLua.executeScript(currentCaption.script);
	}

	return true;
}

void CModuleSubtitles::setState(EState state)
{
	_state = state;

	if (!_fadeCaptions)
		return;

	if(_state == EState::STATE_IN)
		EngineUI.activateWidget("eon_subtitles");
	else if (_state == EState::STATE_OUT)
		EngineUI.deactivateWidget("eon_subtitles");
}

void CModuleSubtitles::triggerAudio()
{
	auto& list = _registeredCaptions[_currentCaption];
	SCaptionParams& current = list[_currentIndex];
	const std::string& event = current.audio;
	
	// Case where the first caption entry has the full audio record...
	if (!event.length())
	return;

	cur_audio_event = EngineAudio.post2DEventGetInst(event);
}

bool CModuleSubtitles::startCaption(const std::string& name, CHandle t)
{
	if (_active)
		return false;

	auto& list = _registeredCaptions[name];
	assert(list.size());
	_currentCaption = name;
	_active = setCaptionEntry();
	_timer = 0.f;

	if (_active)
	{
		setState(EState::STATE_IN);
	}

	_trigger = t;

	return _active;
}

void CModuleSubtitles::stopCaption()
{
	EngineUI.deactivateWidget("eon_subtitles", _fadeCaptions);
	setState(EState::STATE_NONE);
	_currentCaption = "";
	_currentIndex = 0;
	_active = false;

	CEntity* eTrigger = _trigger;
	if (eTrigger)
	{
		eTrigger->sendMsg(TMsgStopCaption());
		
	}

	_trigger = CHandle();

	stopAudio();
}

void CModuleSubtitles::stop()
{
	_registeredCaptions.clear();
}

void CModuleSubtitles::renderInMenu()
{
    if (ImGui::TreeNode("Subtitles"))
    {
		if (_active)
		{
			auto& list = _registeredCaptions[_currentCaption];
			SCaptionParams& current = list[_currentIndex];

			ImGui::Text("Current scene: %s", _currentCaption.c_str());
			ImGui::Text("State: %s", state_entries_names.nameOf(_state));
			ImGui::Text("Current tex: %s", current.texture.c_str());
			ImGui::Text("Current audio: %s", current.audio.c_str());
			ImGui::Text("Index: %d", _currentIndex);
			ImGui::Text("Timer: %f", _timer);
		}

		if (ImGui::TreeNode("Registered captions"))
		{
			for (auto& cs : _registeredCaptions) {

				ImGui::Text("Name: %s", cs.first.c_str());

				ImGui::PushID(&cs);

				if (ImGui::TreeNode("Entries")) {
					for (auto& c : cs.second) {
						ImGui::Text("Texture: %s", c.texture.c_str());
						ImGui::Text("Audio: %s", c.audio.c_str());
						ImGui::Text("Time: %f", c.time);
						ImGui::Separator();
					}
					ImGui::TreePop();
				}

				if (ImGui::Button("Start")) {
					startCaption(cs.first);
				}
				ImGui::SameLine();
				if (ImGui::Button("Stop")) {
					stopCaption();
				}

				ImGui::PopID();
			}

			ImGui::TreePop();
		}

        ImGui::TreePop();
    }
}