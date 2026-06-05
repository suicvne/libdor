#include "DOR.h"
#include "PSU.h"

#include <stdio.h>
#include <string.h>

typedef struct TestAppOptions {
    int ExtendedInfo;
    int SetName;
    const char* pInputPath;
    const char* pOutputPath;
    const char* pName;
} TestAppOptions;

static void PrintUsage(const char* pProgramName)
{
    fprintf(stderr, "usage: %s [-x] <save.psu>\n", pProgramName);
    fprintf(stderr, "       %s [-x] --set-name <input.psu> <output.psu> <name>\n", pProgramName);
}

static int ParseOptions(int argc, char** ppArgv, TestAppOptions* pOptions)
{
    int ArgIndex = 1;

    memset(pOptions, 0, sizeof(*pOptions));
    if (ArgIndex < argc && strcmp(ppArgv[ArgIndex], "-x") == 0) {
        pOptions->ExtendedInfo = 1;
        ArgIndex++;
    }

    if (ArgIndex < argc && strcmp(ppArgv[ArgIndex], "--set-name") == 0) {
        if (argc - ArgIndex != 4) {
            return 0;
        }

        pOptions->SetName = 1;
        pOptions->pInputPath = ppArgv[ArgIndex + 1];
        pOptions->pOutputPath = ppArgv[ArgIndex + 2];
        pOptions->pName = ppArgv[ArgIndex + 3];
        return 1;
    }

    if (argc - ArgIndex != 1) {
        return 0;
    }

    pOptions->pInputPath = ppArgv[ArgIndex];
    return 1;
}

static void PrintPSUEntries(const PSUArchive* pArchive)
{
    size_t Count = PSUArchive_GetEntryCount(pArchive);
    size_t Index;

    printf("PSU entries: %lu\n", (unsigned long)Count);
    for (Index = 0; Index < Count; Index++) {
        PSUEntryInfo Entry;
        if (PSUArchive_GetEntryInfo(pArchive, Index, &Entry) == PSUStatusOk) {
            printf("  %2lu  off=0x%05lx payload=0x%05lx size=0x%08x mode=0x%08x name=%s\n",
                   (unsigned long)Index,
                   (unsigned long)Entry.EntryOffset,
                   (unsigned long)Entry.PayloadOffset,
                   (unsigned)Entry.Size,
                   (unsigned)Entry.Mode,
                   Entry.Name);
        }
    }
}

static void PrintCopySlots(const DORCardInfo* pCardInfo)
{
    size_t SlotIndex;
    size_t ByteIndex;

    printf("      Copy slots:\n");
    for (SlotIndex = 0; SlotIndex < DORCardCopySlotCount; SlotIndex++) {
        printf("        ");
        for (ByteIndex = 0; ByteIndex < DORCardCopySlotByteCount; ByteIndex++) {
            printf("%02X%s",
                   (unsigned)pCardInfo->CopySlots[SlotIndex][ByteIndex],
                   ByteIndex + 1u == DORCardCopySlotByteCount ? "" : " ");
        }
        printf("\n");
    }
}

static void PrintDeck(const DORSave* pSave, int ExtendedInfo)
{
    DORDeckInfo Deck;
    DORCardInfo LeaderInfo;
    char Name[128];
    size_t Index;

    if (DORSave_GetPlayerName(pSave, Name, sizeof(Name)) == DORStatusOk) {
        printf("Player name: %s\n", Name);
    }

    if (DORSave_GetDeckInfo(pSave, A, &Deck) != DORStatusOk) {
        printf("Could not read DOR deck block.\n");
        return;
    }

    printf("Deck leader: %u - %s\n", Deck.LeaderCardId, DORCard_GetName(Deck.LeaderCardId));
    if (DORSave_GetCardInfo(pSave, Deck.LeaderCardId, &LeaderInfo) == DORStatusOk) {
        printf("Leader XP: %u (%s), marker=0x%08x, TotalCopies=%u\n",
               (unsigned)LeaderInfo.Experience,
               DORRank_ToString(DORRank_FromExperience(LeaderInfo.Experience)),
               (unsigned)LeaderInfo.StateMarker,
               DORCardInfo_GetTotalCopyCount(&LeaderInfo)
        );
    }
    printf("Stored deck cost: %u\n", Deck.StoredDeckCost);

    printf("Deck cards:\n");
    DORCardInfo CardInfo;
    for (Index = 0; Index < DORDeckCardCount; Index++) {
        uint16_t CardId = Deck.Cards[Index];

        if (DORSave_GetCardInfo(pSave, CardId, &CardInfo) != DORStatusOk) {
            printf("  %2lu: %3u - %s; Copies: unavailable\n",
                   (unsigned long)Index,
                   (unsigned)CardId,
                   DORCard_GetName(CardId));
            continue;
        }

        printf("  %2lu: %3u - %s; Copies: %u\n",
               (unsigned long)Index,
               (unsigned)CardId,
               DORCard_GetName(CardId),
               DORCardInfo_GetTotalCopyCount(&CardInfo));

        if (ExtendedInfo) {
            PrintCopySlots(&CardInfo);
        }
    }
}

int main(int argc, char** ppArgv)
{
    TestAppOptions Options;
    PSUArchive* pArchive = NULL;
    DORSave* pSave = NULL;
    PSUEntryInfo GameEntry;
    PSUStatus PsuStatus;
    DORStatus DorStatus;

    if (!ParseOptions(argc, ppArgv, &Options)) {
        PrintUsage(ppArgv[0]);
        return 2;
    }

    PsuStatus = PSUArchive_CreateFromFile(Options.pInputPath, &pArchive);
    if (PsuStatus != PSUStatusOk) {
        fprintf(stderr, "failed to read PSU: %s\n", PSUStatus_ToString(PsuStatus));
        return 1;
    }

    PrintPSUEntries(pArchive);

    DorStatus = DORSave_CreateFromPSUArchive(pArchive, &pSave);
    if (DorStatus != DORStatusOk) {
        fprintf(stderr, "failed to read DOR save: %s\n", DORStatus_ToString(DorStatus));
        PSUArchive_Destroy(pArchive);
        return 1;
    }

    printf("DOR game data size: 0x%lx\n", (unsigned long)DORSave_GetSize(pSave));
    printf("DOR checksum: 0x%04x\n", (unsigned)DORSave_GetChecksum(pSave));

    if (Options.SetName) {
        DorStatus = DORSave_SetPlayerName(pSave, Options.pName);
        if (DorStatus != DORStatusOk) {
            fprintf(stderr, "failed to set player name: %s\n", DORStatus_ToString(DorStatus));
            DORSave_Destroy(pSave);
            PSUArchive_Destroy(pArchive);
            return 1;
        }

        PsuStatus = PSUArchive_FindFile(pArchive, "BASLUS-20515", &GameEntry);
        if (PsuStatus != PSUStatusOk) {
            fprintf(stderr, "failed to find DOR game data: %s\n", PSUStatus_ToString(PsuStatus));
            DORSave_Destroy(pSave);
            PSUArchive_Destroy(pArchive);
            return 1;
        }

        PsuStatus = PSUArchive_SetFileData(pArchive, &GameEntry, DORSave_GetBytes(pSave), DORSave_GetSize(pSave));
        if (PsuStatus != PSUStatusOk) {
            fprintf(stderr, "failed to update DOR game data: %s\n", PSUStatus_ToString(PsuStatus));
            DORSave_Destroy(pSave);
            PSUArchive_Destroy(pArchive);
            return 1;
        }

        PsuStatus = PSUArchive_WriteToFile(pArchive, Options.pOutputPath);
        if (PsuStatus != PSUStatusOk) {
            fprintf(stderr, "failed to write PSU: %s\n", PSUStatus_ToString(PsuStatus));
            DORSave_Destroy(pSave);
            PSUArchive_Destroy(pArchive);
            return 1;
        }

        printf("Wrote %s\n", Options.pOutputPath);
    }

    PrintDeck(pSave, Options.ExtendedInfo);

    DORSave_Destroy(pSave);
    PSUArchive_Destroy(pArchive);
    return 0;
}
