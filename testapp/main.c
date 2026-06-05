#include "DOR.h"
#include "PSU.h"

#include <stdio.h>
#include <string.h>

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

static void PrintDeck(const DORSave* pSave)
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
        printf("Leader XP: %u (%s), marker=0x%08x\n",
               (unsigned)LeaderInfo.Experience,
               DORRank_ToString(DORRank_FromExperience(LeaderInfo.Experience)),
               (unsigned)LeaderInfo.StateMarker);
    }
    printf("Unknown after leader: %u\n", Deck.UnknownAfterLeader);

    printf("Deck cards:\n");
    for (Index = 0; Index < DORDeckCardCount; Index++) {
        uint16_t CardId = Deck.Cards[Index];
        printf("  %2lu: %3u - %s\n",
               (unsigned long)Index,
               (unsigned)CardId,
               DORCard_GetName(CardId));
    }
}

int main(int argc, char** ppArgv)
{
    PSUArchive* pArchive = NULL;
    DORSave* pSave = NULL;
    PSUEntryInfo GameEntry;
    PSUStatus PsuStatus;
    DORStatus DorStatus;

    if (argc != 2 && argc != 5) {
        fprintf(stderr, "usage: %s <save.psu>\n", ppArgv[0]);
        fprintf(stderr, "       %s --set-name <input.psu> <output.psu> <name>\n", ppArgv[0]);
        return 2;
    }

    PsuStatus = PSUArchive_CreateFromFile(argc == 2 ? ppArgv[1] : ppArgv[2], &pArchive);
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

    if (argc == 5) {
        if (strcmp(ppArgv[1], "--set-name") != 0) {
            fprintf(stderr, "unknown command: %s\n", ppArgv[1]);
            DORSave_Destroy(pSave);
            PSUArchive_Destroy(pArchive);
            return 2;
        }

        DorStatus = DORSave_SetPlayerName(pSave, ppArgv[4]);
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

        PsuStatus = PSUArchive_WriteToFile(pArchive, ppArgv[3]);
        if (PsuStatus != PSUStatusOk) {
            fprintf(stderr, "failed to write PSU: %s\n", PSUStatus_ToString(PsuStatus));
            DORSave_Destroy(pSave);
            PSUArchive_Destroy(pArchive);
            return 1;
        }

        printf("Wrote %s\n", ppArgv[3]);
    }

    PrintDeck(pSave);

    DORSave_Destroy(pSave);
    PSUArchive_Destroy(pArchive);
    return 0;
}
