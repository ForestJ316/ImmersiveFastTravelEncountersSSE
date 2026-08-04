#include "FastTravelHandler.h"

#include "Utils.h"
#include "MessageBoxHandler.h"
#include "Settings.h"

#include <algorithm>

std::string FastTravelHandler::sFastTravelType = "";
float FastTravelHandler::fThirtySecondsCheck = 0.0f;
float FastTravelHandler::fTimerAfterLoading = 0.0f;

void FastTravelHandler::Initialize()
{
	logger::info("Initializing FastTravelHandler...");

	// Confirm fast travel on the map
	_FastTravelConfirm = REL::Relocation<uintptr_t>(RE::VTABLE___FastTravelConfirmCallback[0]).write_vfunc(0x01, FastTravelConfirm);
	// Player OnUpdate
	_Update = REL::Relocation<uintptr_t>(RE::VTABLE_PlayerCharacter[0]).write_vfunc(0xAD, Update);

	const auto UI = RE::UI::GetSingleton();
	if (!UI) {
		logger::error("UI Singleton not found.");
		return;
	}
	UI->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(FastTravelHandler::GetSingleton());

	const auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
	if (!scriptEventSourceHolder) {
		logger::error("ScriptEventSourceHolder Singleton not found.");
		return;
	}
	scriptEventSourceHolder->GetEventSource<RE::TESActivateEvent>()->AddEventSink(FastTravelHandler::GetSingleton());

	logger::info("...FastTravelHandler initialized.");
}

RE::TESObjectCELL*& FastTravelHandler::GetNearestCellWithLocation()
{
	return nearestCellWithLocation;
}

float FastTravelHandler::GetDistanceTraveled()
{
	// Check if it was a map fast travel
	if (playerMapTravelDistance > 0.0f) {
		auto playerMapTravelDistanceMeters = Utils::GetDistanceInMeters(playerMapTravelDistance);
		playerMapTravelDistance = 0.0f;
		return playerMapTravelDistanceMeters;
	}
	// Everything else check the activator ref
	else if (speakerPtr && speakerPtr.get()) {
		auto distanceMeters = Utils::GetDistanceInMeters(RE::PlayerCharacter::GetSingleton()->GetDistance(speakerPtr.get()));
		// We don't need the speaker after this check
		speakerPtr.reset();
		return distanceMeters;
	}
	return 0.0f;
}

void FastTravelHandler::SetupMessageBoxOnFastTravelEndEvent(std::string a_fastTravelType)
{
	MessageBoxHandler::GetSingleton()->SetupCurrentEncounterData(a_fastTravelType);
	// Show the MessageBox after 1 second
	fTimerAfterLoading = 1.0f;
}

FastTravelHandler::EventResult FastTravelHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
	//logger::info("menu event name: {}, opening: {}", a_event->menuName, a_event->opening);
	if (a_event->menuName == "Dialogue Menu"sv) {
		// Re-check on new dialogue openings before fast traveling as well
		// There are cases where fast travel destination can be changed
		if (a_event->opening) {
			auto speaker = RE::MenuTopicManager::GetSingleton()->speaker;
			if (speaker && speaker.get()) {
				auto activatorCache = Settings::GetSingleton()->GetFastTravelActivatorCache();
				auto speakerBase = speaker.get()->GetBaseObject();
				// Fall-back check the object itself (should have a base though)
				auto fastTravelSource = activatorCache.find(speaker.get()->formID);
				if (speakerBase) {
					fastTravelSource = activatorCache.find(speakerBase->formID);
				}
				if (fastTravelSource != activatorCache.end()) {
					sFastTravelType = fastTravelSource->second;
					// Store the speaker for distance check
					FastTravelHandler::GetSingleton()->speakerPtr = speaker.get();
				}
			}
		}
		// Give 30 seconds to start the fast travel after closing the dialogue
		else if (!a_event->opening && !string::is_empty(sFastTravelType.c_str())) {
			fThirtySecondsCheck = 30.0f;
		}
	}
	if (a_event->menuName == "Loading Menu"sv) {
		// TEST
		logger::info("loading menu opening: {}", a_event->opening);
		if (RE::PlayerCharacter::GetSingleton()->parentCell) {
			logger::info("player cell formid is: {}", RE::PlayerCharacter::GetSingleton()->parentCell->formID);
		}

		// If loading menu started within 30 seconds of closing the dialogue, then it could be a fast travel
		if (a_event->opening && fThirtySecondsCheck > 0.0f) {
			fThirtySecondsCheck = 0.0f;
			// Additional check for distance, for case where the player might go
			// fast travel with another activator nearby (that is not in the list) within the 30s window
			auto a_player = RE::PlayerCharacter::GetSingleton();
			if (a_player && speakerPtr && speakerPtr.get() && a_player->GetDistance(speakerPtr.get()) > 800.0f) {
				sFastTravelType = "";
				speakerPtr.reset(); // Deallocate memory
			}
		}
		// Use loading menu instead of TESFastTravelEndEvent for cases where fast travel is being done with moveto functionality
		// From limited testing TESFastTravelEndEvent fires before loading menu closes
		if (!a_event->opening) {
			if (!string::is_empty(sFastTravelType.c_str())) {
				std::string sFastTravelType_temp = sFastTravelType;
				sFastTravelType = "";
				logger::info("fast travel end event with type: {}", sFastTravelType_temp);
				auto a_player = RE::PlayerCharacter::GetSingleton();
				// From testing cell is already attached when loading menu closes
				// But better to check anyway
				if (!a_player || !a_player->parentCell) {
					return EventResult::kContinue;
				}
				// Distance check based on the setting
				auto distanceTraveled = GetDistanceTraveled();
				if (distanceTraveled < Settings::fMinimumDistance) {
					return EventResult::kContinue;
				}
				// Get the relevant cell now so we don't have to re-check it later in MessageBoxHandler
				nearestCellWithLocation = Utils::GetCellNearPlayerWithLocation(a_player->parentCell);
				// Additional checks for mods that interrupt fast travel and resume it after some kind of event
				// Fast traveling through map only
				if (mapMarkerPtr && mapMarkerPtr->parentCell) {
					auto mapMarkerCell = mapMarkerPtr->parentCell;
					// Check against the parent location of the map marker, since the nearest cell can be a different one
					// Fall-back if no parent loc, check direct location
					if (mapMarkerCell->GetLocation()) {
						auto mapMarkerCellLocParentLoc = mapMarkerCell->GetLocation()->parentLoc;
						if ((mapMarkerCellLocParentLoc && Utils::GetCellIsInLocation(mapMarkerCellLocParentLoc->GetFullName(), nearestCellWithLocation))
							|| (!mapMarkerCellLocParentLoc && Utils::GetCellIsInLocation(mapMarkerCell->GetLocation()->GetFullName(), nearestCellWithLocation))) {
							SetupMessageBoxOnFastTravelEndEvent(sFastTravelType_temp);
						}
					}
					mapMarkerPtr.reset(); // Deallocate memory
				}
				// Everything else pretty much same
				// Map (ini specified activators), Carriage, Ferry, Other
				else {
					SetupMessageBoxOnFastTravelEndEvent(sFastTravelType_temp);
				}
			}
		}
	}
	return EventResult::kContinue;
}

/*
FastTravelHandler::EventResult FastTravelHandler::ProcessEvent(const RE::TESFastTravelEndEvent*, RE::BSTEventSource<RE::TESFastTravelEndEvent>*)
{
	logger::info("in fast travel end event");
	std::string sFastTravelType_temp = sFastTravelType;
	sFastTravelType = "";
	logger::info("fast travel end event with type: {}", sFastTravelType_temp);
	if (!string::is_empty(sFastTravelType_temp.c_str())) {
		auto a_player = RE::PlayerCharacter::GetSingleton();
		// From testing cell is already attached when this event gets called
		// But better to check
		if (!a_player || !a_player->parentCell) {
			// TEST
			logger::info("no player cell in fast travel end event");
			return EventResult::kContinue;
		}
		// Distance check based on the setting
		auto distanceTraveled = GetDistanceTraveled();
		if (distanceTraveled < Settings::fMinimumDistance) {
			return EventResult::kContinue;
		}
		// Get the relevant cell now so we don't have to re-check it later in MessageBoxHandler
		nearestCellWithLocation = Utils::GetCellNearPlayerWithLocation(a_player->parentCell);
		// Additional checks for mods that interrupt fast travel and resume it after some kind of event
		// Fast traveling through map only
		if (mapMarkerPtr && mapMarkerPtr->parentCell) {
			auto mapMarkerCell = mapMarkerPtr->parentCell;
			// Check against the parent location of the map marker, since the nearest cell can be a different one
			// Fall-back if no parent loc, check direct location
			if (mapMarkerCell->GetLocation()) {
				auto mapMarkerCellLocParentLoc = mapMarkerCell->GetLocation()->parentLoc;
				if ((mapMarkerCellLocParentLoc && Utils::GetCellIsInLocation(mapMarkerCellLocParentLoc->GetFullName(), nearestCellWithLocation))
					|| (!mapMarkerCellLocParentLoc && Utils::GetCellIsInLocation(mapMarkerCell->GetLocation()->GetFullName(), nearestCellWithLocation))) {
					ProcessMessageBoxOnFastTravelEndEvent(sFastTravelType_temp);
				}
			}
			mapMarkerPtr.reset(); // Deallocate memory
		}
		// Everything else pretty much same
		// Map (ini specified activators), Carriage, Ferry, Other
		else {
			ProcessMessageBoxOnFastTravelEndEvent(sFastTravelType_temp);
		}
	}
	return EventResult::kContinue;
}
*/



FastTravelHandler::EventResult FastTravelHandler::ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*)
{
	if (string::is_empty(sFastTravelType.c_str()) || (a_event->actionRef && a_event->actionRef.get() != RE::PlayerCharacter::GetSingleton())) {
		return EventResult::kContinue;
	}
	// We only care about teleport doors
	if (a_event->objectActivated && a_event->objectActivated->extraList.GetTeleportLinkedDoor()) {
		fThirtySecondsCheck = 0.0f;
		sFastTravelType = "";
		speakerPtr.reset(); // Deallocate memory
	}
	return EventResult::kContinue;
}

void FastTravelHandler::FastTravelConfirm(RE::FastTravelConfirmCallback* a_this, std::uint8_t a_button)
{
	// TEST
	logger::info("button in confirm: {}", a_button);
	if (a_button == 1) {
		// Reset the timer in case the player decided to use the map instead of an activator
		if (fThirtySecondsCheck > 0.0f) {
			fThirtySecondsCheck = 0.0f;
			FastTravelHandler::GetSingleton()->speakerPtr.reset();
		}
		auto mapMarker = a_this->mapMenu->GetRuntimeData()->mapMarker.get();
		FastTravelHandler::GetSingleton()->mapMarkerPtr = mapMarker;
		FastTravelHandler::GetSingleton()->playerMapTravelDistance = RE::PlayerCharacter::GetSingleton()->GetDistance(mapMarker.get());
		sFastTravelType = "Map";
	}

	_FastTravelConfirm(a_this, a_button);
}

void FastTravelHandler::Update(RE::PlayerCharacter* a_player, float a_delta)
{
	_Update(a_player, a_delta);

	// Give the player 30 seconds to initialize fast travel on non-map events
	if (fThirtySecondsCheck > 0.0f) {
		fThirtySecondsCheck -= RE::BSTimer::GetSingleton()->realTimeDelta;
		// 30 seconds passed without initiating fast travel, no event
		// Keep in mind the few second animation while getting on carriages etc.
		if (fThirtySecondsCheck <= 0.0f && RE::ControlMap::GetSingleton()->IsMovementControlsEnabled()) {
			sFastTravelType = "";
			FastTravelHandler::GetSingleton()->speakerPtr.reset(); // Deallocate memory
		}
	}
	// Show message box 1 second after loading menu closes on fast traveling
	if (fTimerAfterLoading > 0.0f) {
		fTimerAfterLoading -= RE::BSTimer::GetSingleton()->realTimeDelta;
		if (fTimerAfterLoading <= 0.0f) {
			// (Only for non-map fast travel events)
			// From limited testing TESFastTravelEndEvent gets called before the loading menu closes
			// Since it's not confirmed fully, check after 1 second for safety
			// TESFastTravelEndEvent never got called, therefore it wasn't a fast travel
			if (!string::is_empty(sFastTravelType.c_str()) && !FastTravelHandler::GetSingleton()->mapMarkerPtr && fThirtySecondsCheck <= 0.0f) {
				logger::info("nuking fast travel type");
				sFastTravelType = "";
			}
			MessageBoxHandler::GetSingleton()->DisplayMessageBox();
		}
	}
}

void FastTravelHandler::ResetVars()
{
	if (!string::is_empty(sFastTravelType.c_str())) {
		sFastTravelType = "";
		fThirtySecondsCheck = 0.0f;
		mapMarkerPtr.reset();
		nearestCellWithLocation = nullptr;
	}
}
