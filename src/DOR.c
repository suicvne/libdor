#include "DOR.h"
#include "DORInternal.h"

#include <stdlib.h>
#include <string.h>

// NOTE: DOR saves are encoded as little endian. No matter what.
//       All reads and writes have to be LE.

static int DORDeckInfo_IsPresent(const DORDeckInfo* pInfo);

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

uint16_t DORChecksum_Calculate(const DORSave* pSave)
{
    /*
       This had to be reverse engineered from the game's disassembled MIPS code.
       The checksum routine is located at 0x00241190..0x002411F8

       How did I do this?
       I started off by taking the set of bytes representing the player name and finding
       all possible memory locations. Then, I started tracking reads/writes.

       What I found out is that it's not recalculated on overwrite, only recalculated when you
       load or save for the first time. The checksum gets calculated somewhere else.

       Eventually I came across 00241190

        00241190  lhu   a3,0x2(a0)        ; read stored checksum
        00241194  paddub a2,zero,zero     ; accumulator = 0
        00241198  paddub t0,zero,zero     ; word index/count = 0
        0024119C  paddub t1,a0,zero       ; pointer = save base
        002411A0  ori   v1,zero,0x8078    ; word count = 0x8078
        002411A4  beq   zero,zero,0x002411C8
        002411A8  sh    zero,0x2(a0)      ; zero checksum field

        002411AC  lhu   a1,0x0(t1)        ; read next u16
        002411B0  addiu v0,t0,0x1
        002411B4  andi  t0,v0,0xFFFF      ; count++
        002411B8  xor   v0,a2,a1          ; accumulator ^= word
        002411BC  addiu t1,t1,0x2         ; pointer += 2
        002411C0  andi  a2,v0,0xFFFF      ; accumulator &= 0xFFFF
        002411C4  nop

        002411C8  andi  v0,t0,0xFFFF
        002411CC  slt   v0,v0,v1          ; count < 0x8078?
        002411D0  bne   v0,zero,0x002411AC
        002411D4  nop

        002411D8  xori  v1,a2,0x4C6B      ; final checksum = accumulator ^ 0x4C6B
        002411DC  andi  v0,a3,0xFFFF      ; stored checksum
        002411E0  andi  a2,v1,0xFFFF      ; calculated checksum
        002411E4  bne   a2,v0,0x002411F4  ; compare calculated vs stored
        002411E8  sh    v1,0x2(a0)        ; restore/write calculated checksum

        002411EC  beq   zero,zero,0x002411F8
        002411F0  addiu v0,zero,0x1       ; return true

        002411F4  paddub v0,zero,zero     ; return false
        002411F8  jr    ra
        002411FC  nop
     */

    size_t Index;
    uint16_t Temp = 0x0000u;

    if (pSave == NULL || pSave->pBytes == NULL || pSave->ByteCount < DORGameDataSize) {
        return 0;
    }

    // Read every 2 bytes as a U16 and XOR it onto our Temp.
    for (Index = 0; Index < DORGameDataSize; Index += 2u) {
        if (Index == DORChecksumOffset) {
            continue;
        }

        Temp ^= DORReadU16LE(pSave->pBytes + Index);
    }

    // then at the end, XOR by our magic 0x4C6B
    return (uint16_t)(Temp ^ 0x4C6Bu);
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

    // pointer into record bytes
    pRecord = pSave->pBytes + Offset;

    // zero out structure, start filling.
    memset(pOutInfo, 0, sizeof(*pOutInfo));
    pOutInfo->CardId = CardId;
    pOutInfo->QuantityOrOwned = pRecord[0x00];
    pOutInfo->Flags = pRecord[0x01];
    pOutInfo->Experience = DORReadU16LE(pRecord + 0x02);
    pOutInfo->StateMarker = DORReadU32LE(pRecord + 0x04);
    pOutInfo->Unknown08 = DORReadU32LE(pRecord + 0x08);

    // copy record offset -- offset into the copy record array.
    CopyOffset = DORCardRecordsOffset + (size_t)CardId * DORCardRecordSize;
    if (CopyOffset + DORCardRecordSize > pSave->ByteCount) {
        return DORStatusInvalidFormat;
    }

    // pointer to copy records, fill copy records.
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

        if (!DORCopySlotIsEmpty(pSlot) &&
            DORCopySlot_GetStorageLocation(pSlot) == DORCopySlotStorageLocationChest) {
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
        DORCopySlotStorageLocation StorageLoc = DORCopySlot_GetStorageLocation(pSlot);

        if(!DORCopySlotIsEmpty(pSlot) && StorageLoc != DORCopySlotStorageLocationChest) {
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
        if (DORCopySlot_IsLeader(&pInfo->CopySlots[SlotIndex])) {
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

static int DORDeckInfo_IsPresent(const DORDeckInfo* pInfo)
{
    size_t Index;

    if (pInfo == NULL || pInfo->LeaderCardId == DOREmptyCardId) {
        return 0;
    }

    for (Index = 0; Index < DORDeckCardCount; Index++) {
        if (pInfo->Cards[Index] != DOREmptyCardId) {
            return 1;
        }
    }

    return 0;
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

    memcpy(pSave->pBytes + DORPlayerNameOffset, NewNameBytes, sizeof(NewNameBytes));

    Checksum = DORChecksum_Calculate(pSave);
    DORWriteU16LE(pSave->pBytes + DORChecksumOffset, Checksum);

    return DORStatusOk;
}

DORCopySlotStorageLocation DORCopySlot_GetStorageLocation(const DORCopySlot* pCopySlot)
{
    if(pCopySlot == NULL) { return DORCopySlotStorageLocationChest; }

    return pCopySlot->Fields.StorageLocation & 0xC0u;
}

int DORCopySlot_IsLeader(const DORCopySlot* pCopySlot)
{
    if (pCopySlot == NULL) {
        return 0;
    }

    return (pCopySlot->Fields.DeckLeaderState & 0x80u) != 0u;
}
