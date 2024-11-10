#include "Utils.h"

namespace TESSERACT::Utils {
    std::vector<RE::TESObjectCELL*> GetNeighboringCells(RE::TESObjectCELL* currentCell, int depth) {
        std::vector<RE::TESObjectCELL*> neighboringCells;

        if (!currentCell) {
            logger::error("Current cell is null.");
            return neighboringCells;
        }

        if (currentCell->IsExteriorCell()) {
            auto coords = currentCell->GetCoordinates();
            if (!coords) {
                logger::error("Current cell coordinates are null.");
                return neighboringCells;
            }

            logger::info("Current cell coordinates: ({}, {})", coords->cellX, coords->cellY);

            const auto TES = RE::TES::GetSingleton();
            if (TES && TES->gridCells) {
                for (int dx = -depth; dx <= depth; ++dx) {
                    for (int dy = -depth; dy <= depth; ++dy) {
                        if (dx == 0 && dy == 0) continue;

                        int32_t neighborX = coords->cellX + dx;
                        int32_t neighborY = coords->cellY + dy;

                        RE::NiPoint3 neighborPos{
                            static_cast<float>(neighborX * 4096),
                            static_cast<float>(neighborY * 4096),
                            0.0f
                        };

                        auto neighborCell = TES->GetCell(neighborPos);
                        if (neighborCell && neighborCell->IsAttached()) {
                            neighboringCells.push_back(neighborCell);
                        }
                    }
                }
            }
        }

        return neighboringCells;
    }

    std::vector<RE::TESObjectREFR*> ScanningFunction(RE::TESObjectREFR* center, float radius, int depth) {
        std::vector<RE::TESObjectREFR*> result;

        if (!center) {
            logger::error("Reference object is null.");
            return result;
        }

        auto* cell = center->GetParentCell();
        if (!cell) {
            logger::error("Center reference does not have a valid parent cell.");
            return result;
        }

        // Function to scan references in a given cell
        auto scanCell = [&](RE::TESObjectCELL* scanCell) {
            scanCell->ForEachReferenceInRange(center->GetPosition(), radius, [&](RE::TESObjectREFR* ref) {
                if (ref && ref->Is3DLoaded()) {
                    result.push_back(ref);
                }
                return RE::BSContainer::ForEachResult::kContinue;
            });
        };

        // Scan current cell
        scanCell(cell);

        // Get and scan neighboring cells
        std::vector<RE::TESObjectCELL*> neighboringCells = GetNeighboringCells(cell, depth);
        for (auto* neighborCell : neighboringCells) {
            if (neighborCell) {
                scanCell(neighborCell);
            }
        }

        return result;
    }

    bool ForceRefToAlias(RE::TESQuest* script, unsigned int aliasID, RE::TESObjectREFR* ref) {
        static REL::Relocation<decltype(ForceRefToAlias)> fn{ RELOCATION_ID(24523, 25052) };
        return fn(script, aliasID, ref);
    }

    bool __stdcall ForceRefToAliasPapyrus(RE::StaticFunctionTag*, RE::TESQuest* script, unsigned int aliasID, RE::TESObjectREFR* ref) {
        return ForceRefToAlias(script, aliasID, ref);
    }

    RE::SpellItem* GetSpell(RE::BSFixedString spellName) {
        auto* spell = RE::TESForm::LookupByEditorID<RE::SpellItem>(spellName);
        if (!spell) {
            logger::error("Spell with editor ID {} not found.", spellName.c_str());
        }
        return spell;
    }

    RE::SpellItem* __stdcall GetSpellPapyrus(RE::StaticFunctionTag*, RE::BSFixedString spellName) {
        return GetSpell(spellName);
    }

    float CalculateDistance(RE::NiPoint3 point1, RE::NiPoint3 point2) {
        return std::sqrt(
            std::pow(point1.x - point2.x, 2) +
            std::pow(point1.y - point2.y, 2) +
            std::pow(point1.z - point2.z, 2)
        );
    }
}