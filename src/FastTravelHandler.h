#pragma once

class FastTravelHandler :
	public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
	public RE::BSTEventSink<RE::TESActivateEvent>
{
private:
	using EventResult = RE::BSEventNotifyControl;

public:
	static FastTravelHandler* GetSingleton()
	{
		static FastTravelHandler singleton;
		return std::addressof(singleton);
	}
	virtual EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
	virtual EventResult ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*) override;

	void Initialize();
	// Getter for MessageBoxHandler
	RE::TESObjectCELL*& GetNearestCellWithLocation();
	// Reset vars on re-load game
	void ResetVars();

private:
	// Confirm fast travel on the map
	static void FastTravelConfirm(RE::FastTravelConfirmCallback* a_this, std::uint8_t a_button);
	static inline REL::Relocation<decltype(FastTravelConfirm)> _FastTravelConfirm;

	static void Update(RE::PlayerCharacter* a_player, float a_delta);
	static inline REL::Relocation<decltype(Update)> _Update;

	float GetDistanceTraveled();
	void SetupMessageBoxOnFastTravelEndEvent(std::string a_fastTravelType);

	RE::TESObjectCELL* nearestCellWithLocation = nullptr;
	// Have to do additional checks for mods that might interrupt fast travel and then resume it after some kind of event
	// (Map fast travel only)
	RE::NiPointer<RE::TESObjectREFR> mapMarkerPtr = nullptr;
	static std::string sFastTravelType;
	// Stored distance for map fast travel distance check
	float playerMapTravelDistance = 0.0f;
	// Give the player 30 seconds to initiate fast travel with an activator
	static float fThirtySecondsCheck;
	// Store the activator speaker for distance check for activator based types of fast travel
	RE::NiPointer<RE::TESObjectREFR> speakerPtr = nullptr;
	// Timer to show message box 1 second after loading menu closes
	static float fTimerAfterLoading;
};
