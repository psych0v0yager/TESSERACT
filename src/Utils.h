#pragma once
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace TESSERACT::Utils {
    // Core game cell and reference scanning
    std::vector<RE::TESObjectCELL*> GetNeighboringCells(RE::TESObjectCELL* currentCell, int depth = 2);
    std::vector<RE::TESObjectREFR*> ScanningFunction(RE::TESObjectREFR* center, float radius, int depth = 2);
    std::vector<RE::TESObjectREFR*> FastScanningFunction(RE::TESObjectREFR* center, float radius); //Thank you po3

    // Quest and reference management
    bool ForceRefToAlias(RE::TESQuest* script, unsigned int aliasID, RE::TESObjectREFR* ref);
    bool __stdcall ForceRefToAliasPapyrus(RE::StaticFunctionTag*, RE::TESQuest* script, unsigned int aliasID, RE::TESObjectREFR* ref);

    // Spell management
    RE::SpellItem* GetSpell(RE::BSFixedString spellName);
    RE::SpellItem* __stdcall GetSpellPapyrus(RE::StaticFunctionTag*, RE::BSFixedString spellName);

    // Math utilities
    float CalculateDistance(RE::NiPoint3 point1, RE::NiPoint3 point2);
}