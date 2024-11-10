#include "AgentFunctions.h"

namespace TESSERACT::AgentFunctions {
    /**
     * @brief Executes a spell from an actor to a target
     * 
     * Uses Skyrim's magic casting system to execute a spell immediately.
     * The spell will be cast regardless of actor's magicka or spell knowledge.
     * 
     * @param actor The actor who will cast the spell
     * @param spellItem The spell to be cast
     * @param target The target of the spell (can be nullptr for self-cast)
     * 
     * Example:
     * ```cpp
     * auto healingSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("HealOther");
     * ExecuteSpell(someActor, healingSpell, woundedNPC);
     * ```
     */
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

    /**
     * @brief Makes an actor acquire a specific item within a radius
     * 
     * This function:
     * 1. Scans the area around the actor for items matching the name
     * 2. Selects the closest matching item
     * 3. Assigns it to a quest alias
     * 4. Triggers the acquisition spell effect
     * 
     * @param actor The actor performing the acquisition
     * @param questDestination The quest containing the alias
     * @param aliasID The ID of the alias to store the item
     * @param radius Search radius in game units
     * @param itemName The exact name of the item to find
     * 
     * Example:
     * ```cpp
     * ExecuteSpellAcquire(guard, holdingQuest, 1, 1000.0f, "Iron Sword");
     * ```
     */
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

    /**
     * @brief Makes an actor travel to a target location
     * 
     * Initiates travel behavior by:
     * 1. Assigning the destination to a quest alias
     * 2. Casting the travel spell effect
     * 
     * @param actor The actor who will travel
     * @param questDestination The quest containing the alias
     * @param target The destination reference
     * @param aliasID The ID of the alias to store the destination
     * 
     * Example:
     * ```cpp
     * auto inn = RE::TESForm::LookupByEditorID<RE::TESObjectREFR>("WhiterunBanneredMare");
     * ExecuteSpellTravel(npc, holdingQuest, inn, 2);
     * ```
     */
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
}