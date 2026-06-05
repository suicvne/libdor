#include "DOR.h"
#include "DORInternal.h"

#include <stdlib.h>
#include <string.h>

// NOTE: DOR saves are encoded as little endian. No matter what.
//       All reads and writes have to be LE.

// ======================================== Little endian IO helpers ========================================

// TODO: These are not guaranteed AT ALL....
static const uint8_t DORCardCopySlotChest[DORCardCopySlotSize] = {
    0x25u, 0x05u, 0x62u, 0x67u, 0x00u, 0x00u, 0x00u, 0xC0u
};

static const uint8_t DORCardCopySlotDeck[DORCardCopySlotSize] = {
    0x25u, 0x05u, 0x62u, 0x67u, 0x00u, 0x00u, 0x00u, 0x40u
};

static const uint8_t DORCardCopySlotDeckA[DORCardCopySlotSize] = {
    0x25u, 0x05u, 0x62u, 0x67u, 0x25u, 0x05u, 0x62u, 0x67u
};

static uint16_t DORReadU16LE(const uint8_t* pBytes)
{
    return (uint16_t)(((uint16_t)pBytes[0]) | ((uint16_t)pBytes[1] << 8));
}

static uint32_t DORReadU32LE(const uint8_t* pBytes)
{
    return ((uint32_t)pBytes[0]) |
           ((uint32_t)pBytes[1] << 8) |
           ((uint32_t)pBytes[2] << 16) |
           ((uint32_t)pBytes[3] << 24);
}

static void DORWriteU16LE(uint8_t* pBytes, uint16_t Value)
{
    pBytes[0] = (uint8_t)(Value & 0xFFu);
    pBytes[1] = (uint8_t)((Value >> 8) & 0xFFu);
}

// ======================================== Little endian IO helpers ========================================

static size_t DORCardRecordIndexFromCardId(uint16_t CardId)
{
    return (size_t)CardId + 1u;
}

static int DORCardRecordOffsetFromCardId(uint16_t CardId, size_t* pOutOffset)
{
    size_t RecordIndex;

    if (pOutOffset == NULL || CardId >= DORCardCount) {
        return 0;
    }

    RecordIndex = DORCardRecordIndexFromCardId(CardId);
    if (RecordIndex >= DORCardCount) {
        return 0;
    }

    *pOutOffset = DORCardRecordsOffset + RecordIndex * DORCardRecordSize;
    return 1;
}

static int DORBytesEqual(const uint8_t* pA, const uint8_t* pB, size_t ByteCount)
{
    return memcmp(pA, pB, ByteCount) == 0;
}

static int DORCardCopySlotIsLeader(const uint8_t* pSlot)
{
    return pSlot[0] == 0x25u &&
           pSlot[1] == 0x05u &&
           pSlot[2] == 0x62u &&
           pSlot[3] == 0xE7u;
}

static int DORCardCopySlotIsEmpty(const uint8_t* pSlot)
{
    static const uint8_t EmptySlot[DORCardCopySlotSize] = {0};
    static const uint8_t EmptySlotC0[DORCardCopySlotSize] = {
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xC0u
    };

    return DORBytesEqual(pSlot, EmptySlot, sizeof(EmptySlot)) ||
           DORBytesEqual(pSlot, EmptySlotC0, sizeof(EmptySlotC0));
}

DORStatus DORSave_CreateFromBytes(const uint8_t* pBytes, size_t ByteCount, DORSave** ppOutSave)
{
    DORSave* pSave;

    if (pBytes == NULL || ppOutSave == NULL) {
        return DORStatusInvalidArgument;
    }
    if (ByteCount != DORGameDataSize) {
        return DORStatusInvalidFormat;
    }

    *ppOutSave = NULL;
    pSave = (DORSave*)calloc(1, sizeof(DORSave));
    if (pSave == NULL) {
        return DORStatusOutOfMemory;
    }

    pSave->pBytes = (uint8_t*)malloc(ByteCount);
    if (pSave->pBytes == NULL) {
        free(pSave);
        return DORStatusOutOfMemory;
    }

    memcpy(pSave->pBytes, pBytes, ByteCount);
    pSave->ByteCount = ByteCount;
    *ppOutSave = pSave;
    return DORStatusOk;
}

DORStatus DORSave_CreateFromPSUArchive(const PSUArchive* pArchive, DORSave** ppOutSave)
{
    PSUEntryInfo Entry;
    PSUStatus PsuStatus;
    const uint8_t* pBytes;
    size_t ByteCount;

    if (pArchive == NULL || ppOutSave == NULL) {
        return DORStatusInvalidArgument;
    }

    PsuStatus = PSUArchive_FindFile(pArchive, "BASLUS-20515", &Entry);
    if (PsuStatus != PSUStatusOk) {
        return DORStatusNotFound;
    }

    PsuStatus = PSUArchive_GetFileData(pArchive, &Entry, &pBytes, &ByteCount);
    if (PsuStatus != PSUStatusOk) {
        return DORStatusInvalidFormat;
    }

    return DORSave_CreateFromBytes(pBytes, ByteCount, ppOutSave);
}

void DORSave_Destroy(DORSave* pSave)
{
    if (pSave != NULL) {
        free(pSave->pBytes);
        free(pSave);
    }
}

size_t DORSave_GetSize(const DORSave* pSave)
{
    return pSave == NULL ? 0 : pSave->ByteCount;
}

const uint8_t* DORSave_GetBytes(const DORSave* pSave)
{
    return pSave == NULL ? NULL : pSave->pBytes;
}

uint16_t DORSave_GetChecksum(const DORSave* pSave)
{
    if (pSave == NULL || DORChecksumOffset + 2u > pSave->ByteCount) {
        return 0;
    }

    return DORReadU16LE(pSave->pBytes + DORChecksumOffset);
}

DORStatus DORSave_GetCardInfo(const DORSave* pSave, uint16_t CardId, DORCardInfo* pOutInfo)
{
    size_t Offset;
    size_t CopyOffset;
    size_t SlotIndex;
    const uint8_t* pRecord;
    const uint8_t* pCopyRecord;

    if (pSave == NULL || pOutInfo == NULL) {
        return DORStatusInvalidArgument;
    }
    if (!DORCardRecordOffsetFromCardId(CardId, &Offset) || Offset + DORCardRecordSize > pSave->ByteCount) {
        return DORStatusInvalidArgument;
    }

    pRecord = pSave->pBytes + Offset;
    memset(pOutInfo, 0, sizeof(*pOutInfo));
    pOutInfo->CardId = CardId;
    pOutInfo->QuantityOrOwned = pRecord[0x00];
    pOutInfo->Flags = pRecord[0x01];
    pOutInfo->Experience = DORReadU16LE(pRecord + 0x02);
    pOutInfo->StateMarker = DORReadU32LE(pRecord + 0x04);
    pOutInfo->Unknown08 = DORReadU32LE(pRecord + 0x08);

    CopyOffset = DORCardRecordsOffset + (size_t)CardId * DORCardRecordSize;
    if (CopyOffset + DORCardRecordSize > pSave->ByteCount) {
        return DORStatusInvalidFormat;
    }

    pCopyRecord = pSave->pBytes + CopyOffset;
    for (SlotIndex = 0; SlotIndex < DORCardCopySlotCount; SlotIndex++) {
        const uint8_t* pSlot = pCopyRecord + DORCardCopySlotOffset + SlotIndex * DORCardCopySlotSize;

        memcpy(pOutInfo->CopySlots[SlotIndex], pSlot, DORCardCopySlotSize);
    }

    return DORStatusOk;
}

uint8_t DORCardInfo_GetChestCopyCount(const DORCardInfo* pInfo)
{
    size_t SlotIndex;
    uint8_t Count = 0;

    if (pInfo == NULL) {
        return 0;
    }

    for (SlotIndex = 0; SlotIndex < DORCardCopySlotCount; SlotIndex++) {
        if (DORBytesEqual(pInfo->CopySlots[SlotIndex], DORCardCopySlotChest, DORCardCopySlotSize)) {
            Count++;
        }
    }

    return Count;
}

uint8_t DORCardInfo_GetDeckCopyCount(const DORCardInfo* pInfo)
{
    size_t SlotIndex;
    uint8_t Count = 0;

    if (pInfo == NULL) {
        return 0;
    }

    // TODO: Determine how we can distinguish between what deck this copy is in.

    // iterate 9 slots
    for (SlotIndex = 0; SlotIndex < DORCardCopySlotCount; SlotIndex++) {
        const uint8_t* pSlot = pInfo->CopySlots[SlotIndex];

        if (DORBytesEqual(pSlot, DORCardCopySlotDeck, DORCardCopySlotSize) ||
            DORBytesEqual(pSlot, DORCardCopySlotDeckA, DORCardCopySlotSize)) {
            Count++;
        }
    }

    return Count;
}

uint8_t DORCardInfo_GetLeaderMarkerCount(const DORCardInfo* pInfo)
{
    size_t SlotIndex;
    uint8_t Count = 0;

    if (pInfo == NULL) {
        return 0;
    }

    for (SlotIndex = 0; SlotIndex < DORCardCopySlotCount; SlotIndex++) {
        if (DORCardCopySlotIsLeader(pInfo->CopySlots[SlotIndex])) {
            Count++;
        }
    }

    return Count;
}

uint8_t DORCardInfo_GetTotalCopyCount(const DORCardInfo* pInfo)
{
    size_t SlotIndex;
    uint8_t Count = 0;

    if (pInfo == NULL) {
        return 0;
    }

    for (SlotIndex = 0; SlotIndex < DORCardCopySlotCount; SlotIndex++) {
        if (!DORCardCopySlotIsEmpty(pInfo->CopySlots[SlotIndex])) {
            Count++;
        }
    }

    return Count;
}

DORStatus DORSave_GetDeckInfo(const DORSave* pSave, DORDeckID DeckID, DORDeckInfo* pOutInfo)
{
    size_t DeckOffset;
    size_t CardsOffset;
    size_t LeaderOffset;
    size_t Index;

    if (pSave == NULL || pOutInfo == NULL) {
        return DORStatusInvalidArgument;
    }
    if (DeckID != A && DeckID != B && DeckID != C) {
        return DORStatusInvalidArgument;
    }

    DeckOffset = (size_t)DeckID * DORDeckRecordSize;
    CardsOffset = DORDeckCardsOffset + DeckOffset;
    LeaderOffset = DORDeckLeaderOffset + DeckOffset;

    if (LeaderOffset + 4u > pSave->ByteCount) {
        return DORStatusInvalidFormat;
    }

    memset(pOutInfo, 0, sizeof(*pOutInfo));
    for (Index = 0; Index < DORDeckCardCount; Index++) {
        pOutInfo->Cards[Index] = DORReadU16LE(pSave->pBytes + CardsOffset + Index * 2u);
    }

    pOutInfo->LeaderCardId = DORReadU16LE(pSave->pBytes + LeaderOffset);
    pOutInfo->StoredDeckCost = DORReadU16LE(pSave->pBytes + LeaderOffset + 2u);
    return DORStatusOk;
}

const char* DORCard_GetName(uint16_t CardId) { return DORCardNameLookup(CardId); }

const char* DORStatus_ToString(DORStatus Status)
{
    switch (Status) {
    case DORStatusOk: return "Ok";
    case DORStatusInvalidArgument: return "InvalidArgument";
    case DORStatusInvalidFormat: return "InvalidFormat";
    case DORStatusNotFound: return "NotFound";
    case DORStatusOutOfMemory: return "OutOfMemory";
    default: return "Unknown";
    }
}

const char* DORRank_ToString(DORRank Rank)
{
    switch (Rank) {
    case DORRankNCO: return "NCO";
    case DORRank2LT: return "2LT";
    case DORRank1LT: return "1LT";
    case DORRankCPT: return "CPT";
    case DORRankMAJ: return "MAJ";
    case DORRankLTC: return "LTC";
    case DORRankCOL: return "COL";
    case DORRankBG: return "BG";
    case DORRankRADM: return "RADM";
    case DORRankVADM: return "VADM";
    case DORRankADM: return "ADM";
    case DORRankSADM: return "SADM";
    case DORRankSD: return "SD";
    default: return "Unknown";
    }
}

DORRank DORRank_FromExperience(uint16_t Experience)
{
    if (Experience == 65535u) return DORRankSD;
    if (Experience >= 52000u) return DORRankSADM;
    if (Experience >= 40000u) return DORRankADM;
    if (Experience >= 30000u) return DORRankVADM;
    if (Experience >= 22000u) return DORRankRADM;
    if (Experience >= 16000u) return DORRankBG;
    if (Experience >= 11000u) return DORRankCOL;
    if (Experience >= 7000u) return DORRankLTC;
    if (Experience >= 4000u) return DORRankMAJ;
    if (Experience >= 2000u) return DORRankCPT;
    if (Experience >= 1000u) return DORRank1LT;
    if (Experience >= 500u) return DORRank2LT;
    return DORRankNCO;
}

DORStatus DORSave_GetPlayerName(const DORSave* pSave, char* pOutBuffer, size_t OutBufferSize)
{
    uint16_t Encoded[DORNameCharacterCount];
    size_t Index;

    if (pSave == NULL || pOutBuffer == NULL) {
        return DORStatusInvalidArgument;
    }
    if (DORPlayerNameOffset + DORNameCharacterCount * 2u > pSave->ByteCount) {
        return DORStatusInvalidFormat;
    }

    for (Index = 0; Index < DORNameCharacterCount; Index++) {
        Encoded[Index] = DORReadU16LE(pSave->pBytes + DORPlayerNameOffset + Index * 2u);
    }

    return DORText_DecodeToBuffer(Encoded, DORNameCharacterCount, pOutBuffer, OutBufferSize);
}

DORStatus DORSave_SetPlayerName(DORSave* pSave, const char* pName)
{
    uint8_t NewNameBytes[DORNameCharacterCount * 2u];
    size_t NameLength;
    size_t Index;
    int ChecksumDelta = 0;
    uint16_t Checksum;

    if (pSave == NULL || pName == NULL) {
        return DORStatusInvalidArgument;
    }
    if (DORPlayerNameOffset + sizeof(NewNameBytes) > pSave->ByteCount ||
            DORChecksumOffset + 2u > pSave->ByteCount) {
        return DORStatusInvalidFormat;
    }

    NameLength = strlen(pName);
    if (NameLength == 0 || NameLength > DORNameCharacterCount) {
        return DORStatusInvalidArgument;
    }

    memset(NewNameBytes, 0, sizeof(NewNameBytes));
    for (Index = 0; Index < NameLength; Index++) {
        uint8_t Code;

        if (!DORText_EncodeCharacter(pName[Index], &Code)) {
            return DORStatusInvalidArgument;
        }

        NewNameBytes[Index * 2u] = Code;
        NewNameBytes[Index * 2u + 1u] = (Index + 1u == NameLength) ? 0xA0u : 0x20u;
    }

    for (Index = 0; Index < sizeof(NewNameBytes); Index++) {
        ChecksumDelta += (int)NewNameBytes[Index] -
                         (int)pSave->pBytes[DORPlayerNameOffset + Index];
    }

    memcpy(pSave->pBytes + DORPlayerNameOffset, NewNameBytes, sizeof(NewNameBytes));

    Checksum = DORReadU16LE(pSave->pBytes + DORChecksumOffset);
    Checksum = (uint16_t)(Checksum + ChecksumDelta);
    DORWriteU16LE(pSave->pBytes + DORChecksumOffset, Checksum);

    return DORStatusOk;
}
