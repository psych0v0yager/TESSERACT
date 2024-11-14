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

    // Package Storage Dictionary
    // TODO Change all 128s to 127s since Placeholder0 is a container
    inline std::array<RE::TESPackage*, 127> travelPackages;
    inline std::array<RE::TESPackage*, 127> acquirePackages;
    void InitializePackages();

    // Package Getter
    RE::TESPackage* GetPackageForActor(RE::Actor* actor, RE::TESQuest* quest, 
                                    const std::array<RE::TESPackage*, 127>& packages);


    // Papyrus-exposed versions
    void __stdcall ExecuteSpellPapyrus(RE::StaticFunctionTag*, RE::Actor* actor, RE::SpellItem* spellItem, RE::TESObjectREFR* target);
    void __stdcall ExecuteSpellAcquirePapyrus(RE::StaticFunctionTag*, RE::Actor* actor, RE::TESQuest* questDestination, uint32_t aliasID, float radius, RE::BSFixedString itemName);
    void __stdcall ExecuteSpellTravelPapyrus(RE::StaticFunctionTag*, RE::Actor* actor, RE::TESQuest* questDestination, RE::TESObjectREFR* target, unsigned int aliasID);
    RE::TESPackage* __stdcall GetTravelPackagePapyrus(RE::StaticFunctionTag*, RE::Actor* caster, RE::TESQuest* quest);
    RE::TESPackage* __stdcall GetAcquirePackagePapyrus(RE::StaticFunctionTag*, RE::Actor* caster, RE::TESQuest* quest);
}