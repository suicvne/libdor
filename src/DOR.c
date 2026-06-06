#include "DOR.h"
#include "DORInternal.h"

#include <stdlib.h>
#include <string.h>

// NOTE: DOR saves are encoded as little endian. No matter what.
//       All reads and writes have to be LE.

// ======================================== Little endian IO helpers ========================================

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

    if (pOutOffset == NULL || CardId > DORCardCount) {
        return 0;
    }

    RecordIndex = DORCardRecordIndexFromCardId(CardId);
    if (RecordIndex > DORCardCount) {
        return 0;
    }

    *pOutOffset = DORCardRecordsOffset + RecordIndex * DORCardRecordSize;
    return 1;
}

static int DORCopySlotHasKnownOccupiedMarker(const DORCopySlot* pSlot)
{
    return pSlot->Fields.Marker0 == 0x25u &&
           pSlot->Fields.Marker1 == 0x05u &&
           pSlot->Fields.Marker2 == 0x62u &&
           (pSlot->Fields.DeckLeaderState == DORCopySlotDeckLeaderStateNormal ||
            pSlot->Fields.DeckLeaderState == DORCopySlotDeckLeaderStateLeader);
}

static int DORCopySlotHasLeaderMarker(const DORCopySlot* pSlot)
{
    return DORCopySlotHasKnownOccupiedMarker(pSlot) &&
           pSlot->Fields.DeckLeaderState == DORCopySlotDeckLeaderStateLeader;
}

static int DORCopySlotIsEmpty(const DORCopySlot* pSlot)
{
    return pSlot->Fields.Marker0 == 0x00u &&
           pSlot->Fields.Marker1 == 0x00u &&
           pSlot->Fields.Marker2 == 0x00u &&
           pSlot->Fields.DeckLeaderState == 0x00u &&
           pSlot->Fields.Unknown04 == 0x00u &&
           pSlot->Fields.Unknown05 == 0x00u &&
           pSlot->Fields.Unknown06 == 0x00u &&
           (pSlot->Fields.StorageLocation == 0x00u ||
            pSlot->Fields.StorageLocation == DORCopySlotStorageLocationChest);
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

static int DORChecksum_ValidateDeltaSpan(const uint8_t* pOldBytes, const uint8_t* pNewBytes, size_t ByteCount)
{
    return ByteCount == 0u || (pOldBytes != NULL && pNewBytes != NULL);
}

uint16_t DORChecksum_CalculateDelta(
    uint16_t CurrentChecksum,
    const uint8_t* pOldProfileTokenBytes,
    const uint8_t* pNewProfileTokenBytes,
    size_t ProfileTokenByteCount,
    const uint8_t* pOldNameBytes,
    const uint8_t* pNewNameBytes,
    size_t NameByteCount)
{
    size_t Index;
    int Delta = 0;

    if (!DORChecksum_ValidateDeltaSpan(pOldProfileTokenBytes, pNewProfileTokenBytes, ProfileTokenByteCount) ||
            !DORChecksum_ValidateDeltaSpan(pOldNameBytes, pNewNameBytes, NameByteCount)) {
        return CurrentChecksum;
    }

    for (Index = 0; Index < ProfileTokenByteCount; Index++) {
        Delta += (int)pNewProfileTokenBytes[Index] - (int)pOldProfileTokenBytes[Index];
    }

    for (Index = 0; Index < NameByteCount; Index++) {
        Delta += (int)pNewNameBytes[Index] - (int)pOldNameBytes[Index];
    }

    return (uint16_t)(CurrentChecksum + Delta);
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

        memcpy(pOutInfo->CopySlots[SlotIndex].Bytes, pSlot, DORCardCopySlotSize);
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
        const DORCopySlot* pSlot = &pInfo->CopySlots[SlotIndex];

        if (DORCopySlotHasKnownOccupiedMarker(pSlot) &&
            pSlot->Fields.StorageLocation == DORCopySlotStorageLocationChest) {
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

    for (SlotIndex = 0; SlotIndex < DORCardCopySlotCount; SlotIndex++) {
        const DORCopySlot* pSlot = &pInfo->CopySlots[SlotIndex];

        if (DORCopySlotHasKnownOccupiedMarker(pSlot) &&
            (pSlot->Fields.StorageLocation == DORCopySlotStorageLocationDeckA ||
             pSlot->Fields.StorageLocation == DORCopySlotStorageLocationDeckB ||
             pSlot->Fields.StorageLocation == DORCopySlotStorageLocationDeckC)) {
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
        if (DORCopySlotHasLeaderMarker(&pInfo->CopySlots[SlotIndex])) {
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
        if (!DORCopySlotIsEmpty(&pInfo->CopySlots[SlotIndex])) {
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

DORStatus DORSave_GetRawPlayerNameBytes(const DORSave* pSave, const uint8_t** ppOutBytes, size_t* pOutByteCount)
{
    if (pSave == NULL || ppOutBytes == NULL || pOutByteCount == NULL) {
        return DORStatusInvalidArgument;
    }
    if (DORPlayerNameOffset + DORNameCharacterCount * 2u > pSave->ByteCount) {
        return DORStatusInvalidFormat;
    }

    *ppOutBytes = pSave->pBytes + DORPlayerNameOffset;
    *pOutByteCount = DORNameCharacterCount * 2u;
    return DORStatusOk;
}

DORStatus DORSave_GetProfileTokenBytes(const DORSave* pSave, const uint8_t** ppOutBytes, size_t* pOutByteCount)
{
    if (pSave == NULL || ppOutBytes == NULL || pOutByteCount == NULL) {
        return DORStatusInvalidArgument;
    }
    if (DORProfileBlockOffset + DORProfileTokenSize > pSave->ByteCount) {
        return DORStatusInvalidFormat;
    }

    *ppOutBytes = pSave->pBytes + DORProfileBlockOffset;
    *pOutByteCount = DORProfileTokenSize;
    return DORStatusOk;
}

DORStatus DORSave_SetPlayerName(DORSave* pSave, const char* pName)
{
    uint8_t NewNameBytes[DORNameCharacterCount * 2u];
    size_t NameLength;
    size_t Index;
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

    memcpy(NewNameBytes, pSave->pBytes + DORPlayerNameOffset, sizeof(NewNameBytes));
    for (Index = 0; Index < NameLength; Index++) {
        uint8_t Code;

        if (!DORText_EncodeCharacter(pName[Index], &Code)) {
            return DORStatusInvalidArgument;
        }

        NewNameBytes[Index * 2u] = Code;
        NewNameBytes[Index * 2u + 1u] = (Index + 1u == NameLength) ? 0xA0u : 0x20u;
    }

    Checksum = DORChecksum_CalculateDelta(
        DORReadU16LE(pSave->pBytes + DORChecksumOffset),
        pSave->pBytes + DORProfileBlockOffset,
        pSave->pBytes + DORProfileBlockOffset,
        DORProfileTokenSize,
        pSave->pBytes + DORPlayerNameOffset,
        NewNameBytes,
        sizeof(NewNameBytes));
    memcpy(pSave->pBytes + DORPlayerNameOffset, NewNameBytes, sizeof(NewNameBytes));

    DORWriteU16LE(pSave->pBytes + DORChecksumOffset, Checksum);

    return DORStatusOk;
}
