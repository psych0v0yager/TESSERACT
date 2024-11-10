#pragma once
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "Utils.h"
#include <limits>

namespace TESSERACT::AgentFunctions {
    // Core spell execution functions
    void ExecuteSpell(RE::Actor* actor, RE::SpellItem* spellItem, RE::TESObjectREFR* target);
    void ExecuteSpellAcquire(RE::Actor* actor, RE::TESQuest* questDestination, uint32_t aliasID, float radius, const RE::BSFixedString& itemName);
    void ExecuteSpellTravel(RE::Actor* actor, RE::TESQuest* questDestination, RE::TESObjectREFR* target, unsigned int aliasID);

    // Papyrus-exposed versions
    void __stdcall ExecuteSpellPapyrus(RE::StaticFunctionTag*, RE::Actor* actor, RE::SpellItem* spellItem, RE::TESObjectREFR* target);
    void __stdcall ExecuteSpellAcquirePapyrus(RE::StaticFunctionTag*, RE::Actor* actor, RE::TESQuest* questDestination, uint32_t aliasID, float radius, RE::BSFixedString itemName);
    void __stdcall ExecuteSpellTravelPapyrus(RE::StaticFunctionTag*, RE::Actor* actor, RE::TESQuest* questDestination, RE::TESObjectREFR* target, unsigned int aliasID);
}