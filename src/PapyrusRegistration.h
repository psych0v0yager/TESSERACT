#pragma once
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "AgentFunctions.h"
#include "HoldingQuestFunctions.h"
#include "Utils.h"

namespace TESSERACT {
    // Register all Papyrus functions
    bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        // Agent Functions
        vm->RegisterFunction("ExecuteSpell", "TESSERACT", AgentFunctions::ExecuteSpellPapyrus);
        vm->RegisterFunction("ExecuteSpellAcquire", "TESSERACT", AgentFunctions::ExecuteSpellAcquirePapyrus);
        vm->RegisterFunction("ExecuteSpellTravel", "TESSERACT", AgentFunctions::ExecuteSpellTravelPapyrus);
        
        // HoldingQuest Functions
        vm->RegisterFunction("ExtractRefs", "TESSERACT", HoldingQuest::AliasExtractorPapyrus);
        vm->RegisterFunction("ExtractRefsPlaceholder", "TESSERACT", HoldingQuest::AliasExtractorPlaceholderPapyrus);
        vm->RegisterFunction("FastQuestFill", "TESSERACT", HoldingQuest::FastQuestFillPapyrus);
        vm->RegisterFunction("GetUniqueNPCs", "TESSERACT", HoldingQuest::GetUniqueNPCsPapyrus);
        vm->RegisterFunction("BatchUpcastObj2Actor", "TESSERACT", HoldingQuest::BatchUpcastObj2ActorPapyrus);
        vm->RegisterFunction("BatchExtractNames", "TESSERACT", HoldingQuest::BatchExtractNamesPapyrus);
        vm->RegisterFunction("NaiveQuestFill", "TESSERACT", HoldingQuest::NaiveQuestFillPapyrus);
        
        // Utility Functions
        vm->RegisterFunction("GetSpell", "TESSERACT", Utils::GetSpellPapyrus);
        vm->RegisterFunction("ForceRefToAlias", "TESSERACT", Utils::ForceRefToAliasPapyrus);
        
        return true;
    }
}