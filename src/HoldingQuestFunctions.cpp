#include "HoldingQuestFunctions.h"
#include "Utils.h"

namespace TESSERACT::HoldingQuest {
    // Core alias management functions
    std::vector<RE::TESObjectREFR*> AliasExtractor(RE::TESQuest* quest) {
        std::vector<RE::TESObjectREFR*> extractedRefs;
        for (auto& [aliasID, handle] : quest->refAliasMap) {
            extractedRefs.push_back(handle.get().get());
        }
        return extractedRefs;
    }

    std::vector<RE::TESObjectREFR*> AliasExtractorPlaceholder(RE::TESQuest* quest) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<RE::TESObjectREFR*> extractedRefs;
        for (auto& [aliasID, handle] : quest->refAliasMap) {
            extractedRefs.push_back(handle.get().get());
        }

        if (!extractedRefs.empty()) {
            extractedRefs.erase(extractedRefs.begin());
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        logger::info("ExtractRefsPlaceholder execution time: {} microseconds", duration.count());

        return extractedRefs;
    }

    void FastQuestFill(RE::TESQuest* quest, std::vector<RE::TESObjectREFR*> newActors, RE::TESQuest* placeholderQuest) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (!placeholderQuest || newActors.empty() || !quest || 
            quest->refAliasMap.size() < (placeholderQuest->refAliasMap.size() - 1)) {
            logger::error("FastQuestFill: Invalid input parameters");
            return;
        }
        
        // Extract placeholders
        std::vector<RE::TESObjectREFR*> placeholders;
        for (const auto& [aliasID, handle] : placeholderQuest->refAliasMap) {
            placeholders.push_back(handle.get().get());
        }

        // Remove the first placeholder
        if (!placeholders.empty()) {
            placeholders.erase(placeholders.begin());
        }

        // Remove player reference
        auto* playerRef = RE::PlayerCharacter::GetSingleton();
        newActors.erase(std::remove(newActors.begin(), newActors.end(), playerRef), newActors.end());

        // Replace placeholders with new actors
        size_t placeholderIndex = 0;
        for (auto* newActor : newActors) {
            if (placeholderIndex >= placeholders.size()) break;

            if (std::find(placeholders.begin(), placeholders.end(), newActor) == placeholders.end()) {
                Utils::ForceRefToAlias(quest, placeholderIndex, newActor);
                placeholders[placeholderIndex] = newActor;
                placeholderIndex++;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        logger::info("FastQuestFillFunction execution time: {} microseconds", duration.count());
    }

    void NaiveQuestFill(RE::TESQuest* quest, std::vector<RE::TESObjectREFR*> objectList) {
        auto startTime = std::chrono::high_resolution_clock::now();

        if (!quest) {
            logger::error("NaiveQuestFillFunction: Invalid quest pointer");
            return;
        }

        if (objectList.size() != quest->aliases.size()) {
            logger::error("NaiveQuestFillFunction: Object list size does not match the number of aliases");
            return;
        }

        auto* playerRef = skyrim_cast<RE::TESObjectREFR*>(RE::PlayerCharacter::GetSingleton());

        for (size_t i = 0; i < objectList.size(); ++i) {
            if (objectList[i] && objectList[i] != playerRef) {
                Utils::ForceRefToAlias(quest, i, objectList[i]);
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        logger::info("NaiveQuestFillFunction execution time: {} microseconds", duration.count());
    }

    // NPC management functions
    std::vector<RE::TESObjectREFR*> GetUniqueNPCs(std::vector<RE::TESObjectREFR*> scannedObjects) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (scannedObjects.empty()) {
            logger::error("GetUniqueNPCs: Inputted an empty list");
            return {};
        }

        auto* playerRef = skyrim_cast<RE::TESObjectREFR*>(RE::PlayerCharacter::GetSingleton());
        scannedObjects.erase(std::remove(scannedObjects.begin(), scannedObjects.end(), playerRef), 
                           scannedObjects.end());
        
        logger::info("GetUniqueNPCs: Scanning {} objects", scannedObjects.size());

        std::vector<RE::Actor*> uniqueNPCs;
        for (auto* refr : scannedObjects) {
            if (refr->GetFormType() == RE::FormType::ActorCharacter) {
                auto* actor = skyrim_cast<RE::Actor*>(refr);
                if (actor && actor->GetActorBase() && actor->GetActorBase()->IsUnique() && !actor->IsDead()) {
                    uniqueNPCs.push_back(actor);
                    logger::info("Unique actor added with FormID {:08X}", refr->formID);
                }
            }
        }

        std::vector<RE::TESObjectREFR*> scannedUniques;
        for (auto* actor : uniqueNPCs) {
            scannedUniques.push_back(static_cast<RE::TESObjectREFR*>(actor));
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        logger::info("GetUniqueNPCs execution time: {} microseconds", duration.count());
        logger::info("GetUniqueNPCs: Returning {} unique NPCs", scannedUniques.size());
        
        return scannedUniques;
    }

    std::vector<RE::Actor*> BatchUpcastObj2Actor(std::vector<RE::TESObjectREFR*> objectList) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<RE::Actor*> actorList;

        for (auto* object : objectList) {
            if (object && object->GetFormType() == RE::FormType::ActorCharacter) {
                RE::Actor* actor = skyrim_cast<RE::Actor*>(object);
                if (actor) {
                    actorList.push_back(actor);
                    logger::info("BatchUpcastObj2Actor: Successfully upcast object to actor");
                } else {
                    logger::warn("BatchUpcastObj2Actor: Failed to cast object to actor");
                }
            } else {
                logger::warn("BatchUpcastObj2Actor: Object is not an actor character or is null");
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        logger::info("BatchUpcastObj2Actor execution time: {} microseconds", duration.count());
        logger::info("BatchUpcastObj2Actor: Returning actor list with {} elements", actorList.size());
        
        return actorList;
    }

    std::vector<std::string> BatchExtractNames(std::vector<RE::TESObjectREFR*> objectList) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<std::string> objectNames;
        for (auto* object : objectList) {
            if (object) {
                objectNames.push_back(object->GetName());
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        logger::info("BatchExtractNames execution time: {} microseconds", duration.count());
        
        return objectNames;
    }

    // Papyrus-exposed versions
    void __stdcall AliasExtractorPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest) {
        AliasExtractor(quest);
    }

    void __stdcall AliasExtractorPlaceholderPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest) {
        AliasExtractorPlaceholder(quest);
    }

    void __stdcall FastQuestFillPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest, 
                                       std::vector<RE::TESObjectREFR*> newActors, RE::TESQuest* placeholderQuest) {
        FastQuestFill(quest, newActors, placeholderQuest);
    }

    void __stdcall NaiveQuestFillPapyrus(RE::StaticFunctionTag*, RE::TESQuest* quest, 
                                        std::vector<RE::TESObjectREFR*> objectList) {
        NaiveQuestFill(quest, objectList);
    }

    void __stdcall GetUniqueNPCsPapyrus(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> scannedObjects) {
        GetUniqueNPCs(scannedObjects);
    }

    void __stdcall BatchUpcastObj2ActorPapyrus(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> objectList) {
        BatchUpcastObj2Actor(objectList);
    }

    void __stdcall BatchExtractNamesPapyrus(RE::StaticFunctionTag*, std::vector<RE::TESObjectREFR*> objectList) {
        BatchExtractNames(objectList);
    }
}