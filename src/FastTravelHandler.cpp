#include "FastTravelHandler.h"

#include "Settings.h"
#include "MessageBoxHandler.h"

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
		auto playerMapTravelDistanceMeters = utils::GetDistanceInMeters(playerMapTravelDistance);
		playerMapTravelDistance = 0.0f;
		return playerMapTravelDistanceMeters;
	}
	// Everything else check the activator ref
	else if (speakerPtr && speakerPtr.get()) {
		const auto a_player = RE::PlayerCharacter::GetSingleton();
		auto distance = a_player->GetDistance(speakerPtr.get());
		// If it's an interior cell then get the parent location and try to pull its center marker
		if (speakerPtr->parentCell && speakerPtr->parentCell->IsInteriorCell()) {
			auto speakerLoc = speakerPtr->parentCell->GetLocation();
			if (speakerLoc && speakerLoc->parentLoc && speakerLoc->parentLoc->worldLocMarker) {
				distance = a_player->GetDistance(speakerLoc->parentLoc->worldLocMarker.get().get());
			}
		}
		// We don't need the speaker after this check
		speakerPtr.reset();
		return utils::GetDistanceInMeters(distance);
	}
	return 0.0f;
}

void FastTravelHandler::SetupMessageBoxOnFastTravelEndEvent(const std::string a_fastTravelType)
{
	MessageBoxHandler::GetSingleton()->SetupCurrentEncounterData(a_fastTravelType);
	// Show the MessageBox after 1.5 seconds
	fTimerAfterLoading = 1.5f;
}

FastTravelHandler::EventResult FastTravelHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
	if (a_event->menuName == RE::DialogueMenu::MENU_NAME) {
		// Re-check on new dialogue openings before fast traveling as well
		// There are cases where fast travel destination can be changed
		if (a_event->opening) {
			auto speaker = RE::MenuTopicManager::GetSingleton()->speaker;
			if (speaker && speaker.get()) {
				const auto& activatorCache = Settings::GetSingleton()->GetFastTravelActivatorCache();
				auto speakerBase = speaker.get()->GetBaseObject();
				// Fall-back check the reference itself (should have a base though)
				auto fastTravelSource = activatorCache.find(speaker.get()->formID);
				if (speakerBase) {
					fastTravelSource = activatorCache.find(speakerBase->formID);
				}
				if (fastTravelSource != activatorCache.end()) {
					sFastTravelType = fastTravelSource->second;
					// Store the speaker for distance check
					speakerPtr = speaker.get();
				}
			}
		}
		// Give 30 seconds to start the fast travel after closing the dialogue
		else if (!a_event->opening && !string::is_empty(sFastTravelType.c_str())) {
			fThirtySecondsCheck = 30.0f;
		}
	}
	if (a_event->menuName == RE::LoadingMenu::MENU_NAME) {
		// If loading menu started within 30 seconds of closing the dialogue, then it could be a fast travel
		if (a_event->opening && fThirtySecondsCheck > 0.0f) {
			fThirtySecondsCheck = 0.0f;
			// Additional check for distance, for case where the player might go
			// fast travel with another activator nearby (that is not in the list) within the 30s window
			const auto a_player = RE::PlayerCharacter::GetSingleton();
			if (speakerPtr && speakerPtr.get() && a_player && a_player->GetDistance(speakerPtr.get()) > 1000.0f) {
				sFastTravelType = "";
				speakerPtr.reset(); // Deallocate memory
			}
		}
		// Use loading menu instead of TESFastTravelEndEvent for cases where fast travel is being done with moveto functionality
		// From limited testing TESFastTravelEndEvent fires before loading menu closes
		if (!a_event->opening && !string::is_empty(sFastTravelType.c_str())) {
			// If the fast travel type is toggled off then don't do the event
			if (!Settings::GetSingleton()->IsFastTravelTypeEnabled(sFastTravelType)) {
				ResetVars();
				return EventResult::kContinue;
			}
			const auto a_player = RE::PlayerCharacter::GetSingleton();
			// From testing cell is already attached when loading menu closes
			// But better to check anyway
			if (!a_player || !a_player->parentCell) {
				ResetVars();
				return EventResult::kContinue;
			}
			// Distance check based on the setting
			if (GetDistanceTraveled() < Settings::fMinimumDistance) {
				ResetVars();
				return EventResult::kContinue;
			}
			const std::string sFastTravelType_temp = sFastTravelType;
			sFastTravelType = "";
			// Get the relevant cell now so we don't have to re-check it later in MessageBoxHandler
			nearestCellWithLocation = utils::GetCellNearPlayerWithLocation(a_player->parentCell);
			// Additional checks for mods that interrupt fast travel and resume it after some kind of event
			// (Fast traveling through map only)
			if (mapMarkerPtr && mapMarkerPtr->parentCell) {
				auto mapMarkerCell = mapMarkerPtr->parentCell;
				// Check against the parent location of the map marker, since the nearest cell can be a different one
				// Fall-back if no parent loc: check direct location
				if (mapMarkerCell->GetLocation()) {
					auto mapMarkerCellLocParentLoc = mapMarkerCell->GetLocation()->parentLoc;
					if ((mapMarkerCellLocParentLoc && utils::GetCellIsInLocation(nearestCellWithLocation, mapMarkerCellLocParentLoc->GetFullName()))
						|| (!mapMarkerCellLocParentLoc && utils::GetCellIsInLocation(nearestCellWithLocation, mapMarkerCell->GetLocation()->GetFullName()))) {
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
	return EventResult::kContinue;
}

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
	if (a_button == 1) {
		const auto a_fastTravelHandler = FastTravelHandler::GetSingleton();
		// Reset the timer in case the player decided to use the map instead of an activator
		if (fThirtySecondsCheck > 0.0f) {
			fThirtySecondsCheck = 0.0f;
			a_fastTravelHandler->speakerPtr.reset();
		}
		// Different struct for VR in RuntimeData()
		auto mapMarker = !REL::Module::IsVR() ? a_this->mapMenu->GetRuntimeData()->mapMarker.get() : a_this->mapMenu->GetVRRuntimeData()->mapMarker.get();
		a_fastTravelHandler->mapMarkerPtr = mapMarker;
		a_fastTravelHandler->playerMapTravelDistance = RE::PlayerCharacter::GetSingleton()->GetDistance(mapMarker.get());
		sFastTravelType = "Map";
	}

	_FastTravelConfirm(a_this, a_button);
}

void FastTravelHandler::Update(RE::PlayerCharacter* a_player, float a_delta)
{
	_Update(a_player, a_delta);

	// Give the player 30 seconds to initialize fast travel on non-map events
	// PlayerCharacter::Update doesn't fire while the game is paused, so there is time to browse the map
	// for mods that might let the player choose a destination from the map
	// Exception: Unpaused Menus...
	if (fThirtySecondsCheck > 0.0f && !RE::UI::GetSingleton()->IsMenuOpen(RE::MapMenu::MENU_NAME)) {
		fThirtySecondsCheck -= RE::BSTimer::GetSingleton()->realTimeDelta;
		// 30 seconds passed without initiating fast travel, no event
		// Keep in mind the few second animation while getting on carriages etc.
		if (fThirtySecondsCheck <= 0.0f && RE::ControlMap::GetSingleton()->IsMovementControlsEnabled()) {
			sFastTravelType = "";
			FastTravelHandler::GetSingleton()->speakerPtr.reset(); // Deallocate memory
		}
	}
	// Show message box 1.5 seconds after loading menu closes on fast traveling
	if (fTimerAfterLoading > 0.0f) {
		fTimerAfterLoading -= RE::BSTimer::GetSingleton()->realTimeDelta;
		if (fTimerAfterLoading <= 0.0f) {
			MessageBoxHandler::GetSingleton()->DisplayMessageBox(true);
		}
	}
}

void FastTravelHandler::ResetVars()
{
	const auto a_fastTravelHandler = FastTravelHandler::GetSingleton();
	if (!string::is_empty(a_fastTravelHandler->sFastTravelType.c_str())) {
		a_fastTravelHandler->sFastTravelType = "";
		a_fastTravelHandler->fThirtySecondsCheck = 0.0f;
		a_fastTravelHandler->fTimerAfterLoading = 0.0f;
		a_fastTravelHandler->playerMapTravelDistance = 0.0f;
		a_fastTravelHandler->nearestCellWithLocation = nullptr;
		a_fastTravelHandler->mapMarkerPtr.reset();
		a_fastTravelHandler->speakerPtr.reset();
	}
}
