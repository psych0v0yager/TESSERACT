#pragma once
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include <vector>
#include <unordered_set>

/**
 * Quest Alias System Overview
 * 
 * In Skyrim, every quest can have up to 128 aliases. Each alias must always be filled 
 * with an ObjectReference. TESSERACT uses this system by:
 * 
 * 1. HoldingQuest Setup:
 *    - Creates a quest with 128 aliases
 *    - All aliases are initially filled with a placeholder object (stored in a debug chest)
 *    - This ensures all aliases are valid but "empty" from the system's perspective
 * 
 * 2. Dynamic NPC Assignment:
 *    - The UI settings control how many aliases (0-128) can be used for NPCs
 *    - Example: If set to 10, only the first 10 aliases can hold NPCs
 *    - The remaining 118 aliases stay filled with placeholders
 *    - This allows for dynamic scaling of the system without changing quest structure
 * 
 * 3. Placeholder Management:
 *    - Placeholder is a permanent ObjectReference in the game
 *    - When an NPC is removed, their alias is refilled with the placeholder
 *    - This maintains quest stability by ensuring aliases are never empty
 * 
 * This system provides a robust way to:
 * - Dynamically manage NPC references
 * - Scale system capacity through settings
 * - Maintain quest stability
 * - Track and swap NPCs efficiently
 */

namespace TESSERACT::HoldingQuest {
    // Core alias management functions
    std::vector<RE::TESObjectREFR*> AliasExtractor(RE::TESQuest* quest);
    std::vector<RE::TESObjectREFR*> AliasExtractorPlaceholder(RE::TESQuest* quest);
    void FastQuestFill(RE::TESQuest* quest, std::vector<RE::TESObjectREFR*> newActors, RE::TESQuest* placeholderQuest);
    void NaiveQuestFill(RE::TESQuest* quest, std::vector<RE::TESObjectREFR*> objectList);

    // NPC management functions
    std::vector<RE::TESObjectREFR*> GetUniqueNPCs(std::vector<RE::TESObjectREFR*> scannedObjects);
    std::vector<RE::Actor*> BatchUpcastObj2Actor(std::vector<RE::TESObjectREFR*> objectList);
    std::vector<std::string> BatchExtractNames(std::vector<RE::TESObjectREFR*> objectList);

    // Papyrus-exposed versions
    void __stdcall AliasExtractorPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest);
    void __stdcall AliasExtractorPlaceholderPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest);
    void __stdcall FastQuestFillPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest, std::vector<RE::TESObjectREFR*> newActors, RE::TESQuest* placeholderQuest);
    void __stdcall NaiveQuestFillPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest, std::vector<RE::TESObjectREFR*> objectList);
    void __stdcall GetUniqueNPCsPapyrus(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> scannedObjects);
    void __stdcall BatchUpcastObj2ActorPapyrus(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> objectList);
    void __stdcall BatchExtractNamesPapyrus(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> objectList);
}