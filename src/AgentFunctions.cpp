#include "AgentFunctions.h"

namespace TESSERACT::AgentFunctions {
    // Force the NPC to cast a spell
    void ExecuteSpell(RE::Actor* actor, RE::SpellItem* spellItem, RE::TESObjectREFR* target) {
        if (!actor || !spellItem) {
            logger::error("ExecuteSpell: Invalid actor or spell");
            return;
        }

        auto* magicCaster = actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
        if (magicCaster) {
            magicCaster->CastSpellImmediate(spellItem, false, target, 1.0f, false, 1.0f, actor);
            logger::info("Spell {} cast by actor {} on target {}.", 
                spellItem->GetFormEditorID(), 
                actor->GetFormID(), 
                target ? target->GetFormID() : actor->GetFormID());
        } else {
            logger::error("MagicCaster not found for actor {}", actor->GetFormID());
        }
    }

    // Force the Acquire Package on to the NPC
    void ExecuteSpellAcquire(RE::Actor* actor, RE::TESQuest* questDestination, 
                            uint32_t aliasID, float radius, const RE::BSFixedString& itemName) {
        if (!actor || !questDestination) {
            logger::error("ExecuteSpellAcquire: Invalid actor or quest");
            return;
        }

        logger::info("ExecuteSpellAcquire: Starting acquisition for {}", itemName.c_str());
        
        // Scan for objects in radius
        // std::vector<RE::TESObjectREFR*> scannedObjects = Utils::ScanningFunction(actor, radius);
        std::vector<RE::TESObjectREFR*> scannedObjects = Utils::FastScanningFunction(actor, radius);

        // Find matching objects
        std::vector<RE::TESObjectREFR*> matchingObjects;
        for (auto* object : scannedObjects) {
            if (object && std::strcmp(object->GetName(), itemName.c_str()) == 0) {
                matchingObjects.push_back(object);
            }
        }

        if (matchingObjects.empty()) {
            logger::info("ExecuteSpellAcquire: No matching items found");
            return;
        }

        // Find closest object
        RE::TESObjectREFR* target = nullptr;
        float minDistance = (std::numeric_limits<float>::max)();
        RE::NiPoint3 actorPos = actor->GetPosition();
        
        for (auto* object : matchingObjects) {
            float distance = Utils::CalculateDistance(actorPos, object->GetPosition());
            if (distance < minDistance) {
                minDistance = distance;
                target = object;
            }
        }

        // Force reference to alias and cast spell
        if (target && Utils::ForceRefToAlias(questDestination, aliasID, target)) {
            RE::SpellItem* spellItem = RE::TESForm::LookupByEditorID<RE::SpellItem>("MP_TestAcquireEndSpell");
            if (spellItem) {
                ExecuteSpell(actor, spellItem, target);
            }
        }
    }

    // Force the Travel Package on to the NPC
    void ExecuteSpellTravel(RE::Actor* actor, RE::TESQuest* questDestination, 
                           RE::TESObjectREFR* target, unsigned int aliasID) {
        if (!actor || !questDestination || !target) {
            logger::error("ExecuteSpellTravel: Invalid parameters");
            return;
        }

        if (Utils::ForceRefToAlias(questDestination, aliasID, target)) {
            RE::SpellItem* spellItem = RE::TESForm::LookupByEditorID<RE::SpellItem>("MP_TestTravelEndSpell");
            if (spellItem) {
                ExecuteSpell(actor, spellItem, target);
            }
        }
    }

    // Initialize the package arrays
    void InitializePackages() {
        for (size_t i = 0; i < 128; i++) {
            // Lookup packages by FormID or EditorID - adjust base IDs as needed
            travelPackages[i] = RE::TESForm::LookupByEditorID<RE::TESPackage>("TESSERACT_TravelPackage_NPC" + i);
            acquirePackages[i] = RE::TESForm::LookupByEditorID<RE::TESPackage>("TESSERACT_AcquirePackage_NPC" + i);
        }
    }

    // Get package for an actor by searching quest aliases
    RE::TESPackage* GetPackageForActor(RE::Actor* actor, RE::TESQuest* quest, 
                                      const std::array<RE::TESPackage*, 128>& packages) {
        if (!actor || !quest) return nullptr;

        // Extract refs from aliases
        std::vector<RE::TESObjectREFR*> refs;
        for (auto& [aliasID, handle] : quest->refAliasMap) {
            refs.push_back(handle.get().get());
        }

        // Find actor and use index as alias ID
        for (size_t i = 0; i < refs.size(); i++) {
            if (refs[i] == actor) {
                return packages[i];
            }
        }
        
        return nullptr;
    }

    // Papyrus-exposed versions
    void __stdcall ExecuteSpellPapyrus(RE::StaticFunctionTag*, RE::Actor* actor, 
                                      RE::SpellItem* spellItem, RE::TESObjectREFR* target) {
        ExecuteSpell(actor, spellItem, target);
    }

    void __stdcall ExecuteSpellAcquirePapyrus(RE::StaticFunctionTag*, RE::Actor* actor, 
                                             RE::TESQuest* questDestination, uint32_t aliasID, 
                                             float radius, RE::BSFixedString itemName) {
        ExecuteSpellAcquire(actor, questDestination, aliasID, radius, itemName);
    }

    void __stdcall ExecuteSpellTravelPapyrus(RE::StaticFunctionTag*, RE::Actor* actor, 
                                            RE::TESQuest* questDestination, RE::TESObjectREFR* target, 
                                            unsigned int aliasID) {
        ExecuteSpellTravel(actor, questDestination, target, aliasID);
    }

    RE::TESPackage* __stdcall GetTravelPackage(RE::StaticFunctionTag*, RE::Actor* caster, RE::TESQuest* quest) {
        GetPackageForActor(caster, quest, travelPackages);
    }

    RE::TESPackage* __stdcall GetAcquirePackage(RE::StaticFunctionTag*, RE::Actor* caster, RE::TESQuest* quest) {
        GetPackageForActor(caster, quest, acquirePackages);
    }


}
