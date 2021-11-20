#pragma once
#include "modules/module.h"
#include "fmod_studio.hpp"

class CModuleSubtitles : public IModule
{
public:

	enum class EState {
		STATE_NONE,
		STATE_ACTIVE,
		STATE_IN,
		STATE_OUT
	};

	CModuleSubtitles(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void renderInMenu() override;
	void update(float dt) override;

	bool startCaption(const std::string& name, CHandle t = CHandle());
	void stopCaption();
	void stopAudio() { cur_audio_event->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT); }

private:


	struct SCaptionParams {
		std::string texture;
		std::string audio;
		float time;
		bool pause;
	};

	bool _active = false;
	bool _fadeCaptions = true;

	CHandle _trigger;
	float _timer = 0.f;
	float _fadeInTime = 0.f;
	float _fadeOutTime = 0.f;
	float _fadeThreshold = 0.1f;

	EState _state = EState::STATE_NONE;
	int _currentIndex = 0;
	std::string _currentCaption;
	std::map<std::string, std::vector<SCaptionParams>> _registeredCaptions;
	
	FMOD::Studio::EventInstance* cur_audio_event = nullptr;

	void parseCaptionList(SCaptionParams& caption, const json& j);
	void parseCaption(const json& j);
	
	EState getState() { return _state; }
	void setState(EState state);
	bool setCaptionEntry();
	void triggerAudio();
};