#include "PSU.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PSUArchive {
    uint8_t* pBytes;
    size_t ByteCount;
    PSUEntryInfo* pEntries;
    size_t EntryCount;
};

static uint32_t PSUReadU32LE(const uint8_t* pBytes)
{
    return ((uint32_t)pBytes[0]) |
           ((uint32_t)pBytes[1] << 8) |
           ((uint32_t)pBytes[2] << 16) |
           ((uint32_t)pBytes[3] << 24);
}

static size_t PSURoundUp(size_t Value, size_t Alignment)
{
    return (Value + Alignment - 1u) & ~(Alignment - 1u);
}

static void PSUReadName(const uint8_t* pEntryBytes, char pOutName[256])
{
    size_t Length = 0;
    const uint8_t* pNameBytes = pEntryBytes + 0x40;

    while (Length < 255 && pNameBytes[Length] != 0) {
        pOutName[Length] = (char)pNameBytes[Length];
        Length++;
    }

    pOutName[Length] = '\0';
}

static int PSUEntryIsFile(const PSUEntryInfo* pEntry)
{
    return pEntry->Size > 0 && (pEntry->Mode & 0x1Fu) == 0x17u;
}

static PSUStatus PSUParseEntries(PSUArchive* pArchive)
{
    size_t Offset = 0;
    size_t Capacity = 8;

    pArchive->pEntries = (PSUEntryInfo*)calloc(Capacity, sizeof(PSUEntryInfo));
    if (pArchive->pEntries == NULL) {
        return PSUStatusOutOfMemory;
    }

    while (Offset + 0x200u <= pArchive->ByteCount) {
        PSUEntryInfo Entry;
        const uint8_t* pEntryBytes = pArchive->pBytes + Offset;

        memset(&Entry, 0, sizeof(Entry));
        Entry.EntryOffset = Offset;
        Entry.PayloadOffset = Offset + 0x200u;
        Entry.Mode = PSUReadU32LE(pEntryBytes + 0x00);
        Entry.Size = PSUReadU32LE(pEntryBytes + 0x04);
        Entry.Type = PSUReadU32LE(pEntryBytes + 0x10);
        PSUReadName(pEntryBytes, Entry.Name);

        if (pArchive->EntryCount == Capacity) {
            PSUEntryInfo* pNewEntries;
            Capacity *= 2;
            pNewEntries = (PSUEntryInfo*)realloc(pArchive->pEntries, Capacity * sizeof(PSUEntryInfo));
            if (pNewEntries == NULL) {
                return PSUStatusOutOfMemory;
            }
            pArchive->pEntries = pNewEntries;
        }

        pArchive->pEntries[pArchive->EntryCount++] = Entry;

        Offset += 0x200u;
        if (PSUEntryIsFile(&Entry)) {
            size_t PaddedSize = PSURoundUp((size_t)Entry.Size, 0x400u);
            if (Offset + PaddedSize > pArchive->ByteCount) {
                return PSUStatusInvalidFormat;
            }
            Offset += PaddedSize;
        }
    }

    return pArchive->EntryCount == 0 ? PSUStatusInvalidFormat : PSUStatusOk;
}

PSUStatus PSUArchive_CreateFromBytes(const uint8_t* pBytes, size_t ByteCount, PSUArchive** ppOutArchive)
{
    PSUArchive* pArchive;
    PSUStatus Status;

    if (pBytes == NULL || ByteCount == 0 || ppOutArchive == NULL) {
        return PSUStatusInvalidArgument;
    }

    *ppOutArchive = NULL;
    pArchive = (PSUArchive*)calloc(1, sizeof(PSUArchive));
    if (pArchive == NULL) {
        return PSUStatusOutOfMemory;
    }

    pArchive->pBytes = (uint8_t*)malloc(ByteCount);
    if (pArchive->pBytes == NULL) {
        free(pArchive);
        return PSUStatusOutOfMemory;
    }

    memcpy(pArchive->pBytes, pBytes, ByteCount);
    pArchive->ByteCount = ByteCount;

    Status = PSUParseEntries(pArchive);
    if (Status != PSUStatusOk) {
        PSUArchive_Destroy(pArchive);
        return Status;
    }

    *ppOutArchive = pArchive;
    return PSUStatusOk;
}

PSUStatus PSUArchive_CreateFromFile(const char* pPath, PSUArchive** ppOutArchive)
{
    FILE* pFile;
    long Size;
    uint8_t* pBytes;
    PSUStatus Status;

    if (pPath == NULL || ppOutArchive == NULL) {
        return PSUStatusInvalidArgument;
    }

    pFile = fopen(pPath, "rb");
    if (pFile == NULL) {
        return PSUStatusIoError;
    }

    if (fseek(pFile, 0, SEEK_END) != 0) {
        fclose(pFile);
        return PSUStatusIoError;
    }

    Size = ftell(pFile);
    if (Size <= 0) {
        fclose(pFile);
        return PSUStatusIoError;
    }

    if (fseek(pFile, 0, SEEK_SET) != 0) {
        fclose(pFile);
        return PSUStatusIoError;
    }

    pBytes = (uint8_t*)malloc((size_t)Size);
    if (pBytes == NULL) {
        fclose(pFile);
        return PSUStatusOutOfMemory;
    }

    if (fread(pBytes, 1, (size_t)Size, pFile) != (size_t)Size) {
        free(pBytes);
        fclose(pFile);
        return PSUStatusIoError;
    }

    fclose(pFile);
    Status = PSUArchive_CreateFromBytes(pBytes, (size_t)Size, ppOutArchive);
    free(pBytes);
    return Status;
}

void PSUArchive_Destroy(PSUArchive* pArchive)
{
    if (pArchive != NULL) {
        free(pArchive->pEntries);
        free(pArchive->pBytes);
        free(pArchive);
    }
}

size_t PSUArchive_GetEntryCount(const PSUArchive* pArchive)
{
    return pArchive == NULL ? 0 : pArchive->EntryCount;
}

PSUStatus PSUArchive_GetEntryInfo(const PSUArchive* pArchive, size_t Index, PSUEntryInfo* pOutInfo)
{
    if (pArchive == NULL || pOutInfo == NULL) {
        return PSUStatusInvalidArgument;
    }
    if (Index >= pArchive->EntryCount) {
        return PSUStatusNotFound;
    }

    *pOutInfo = pArchive->pEntries[Index];
    return PSUStatusOk;
}

PSUStatus PSUArchive_FindFile(const PSUArchive* pArchive, const char* pName, PSUEntryInfo* pOutInfo)
{
    size_t Index;

    if (pArchive == NULL || pName == NULL || pOutInfo == NULL) {
        return PSUStatusInvalidArgument;
    }

    for (Index = 0; Index < pArchive->EntryCount; Index++) {
        const PSUEntryInfo* pEntry = &pArchive->pEntries[Index];
        if (PSUEntryIsFile(pEntry) && strcmp(pEntry->Name, pName) == 0) {
            *pOutInfo = *pEntry;
            return PSUStatusOk;
        }
    }

    return PSUStatusNotFound;
}

PSUStatus PSUArchive_GetFileData(const PSUArchive* pArchive, const PSUEntryInfo* pEntry, const uint8_t** ppOutBytes, size_t* pOutByteCount)
{
    if (pArchive == NULL || pEntry == NULL || ppOutBytes == NULL || pOutByteCount == NULL) {
        return PSUStatusInvalidArgument;
    }

    if (!PSUEntryIsFile(pEntry) || pEntry->PayloadOffset + pEntry->Size > pArchive->ByteCount) {
        return PSUStatusInvalidFormat;
    }

    *ppOutBytes = pArchive->pBytes + pEntry->PayloadOffset;
    *pOutByteCount = pEntry->Size;
    return PSUStatusOk;
}

PSUStatus PSUArchive_SetFileData(PSUArchive* pArchive, const PSUEntryInfo* pEntry, const uint8_t* pBytes, size_t ByteCount)
{
    if (pArchive == NULL || pEntry == NULL || pBytes == NULL) {
        return PSUStatusInvalidArgument;
    }
    if (!PSUEntryIsFile(pEntry) || ByteCount != pEntry->Size ||
            pEntry->PayloadOffset + pEntry->Size > pArchive->ByteCount) {
        return PSUStatusInvalidFormat;
    }

    memcpy(pArchive->pBytes + pEntry->PayloadOffset, pBytes, ByteCount);
    return PSUStatusOk;
}

PSUStatus PSUArchive_WriteToFile(const PSUArchive* pArchive, const char* pPath)
{
    FILE* pFile;

    if (pArchive == NULL || pPath == NULL) {
        return PSUStatusInvalidArgument;
    }

    pFile = fopen(pPath, "wb");
    if (pFile == NULL) {
        return PSUStatusIoError;
    }

    if (fwrite(pArchive->pBytes, 1, pArchive->ByteCount, pFile) != pArchive->ByteCount) {
        fclose(pFile);
        return PSUStatusIoError;
    }

    if (fclose(pFile) != 0) {
        return PSUStatusIoError;
    }

    return PSUStatusOk;
}

const char* PSUStatus_ToString(PSUStatus Status)
{
    switch (Status) {
    case PSUStatusOk: return "Ok";
    case PSUStatusInvalidArgument: return "InvalidArgument";
    case PSUStatusInvalidFormat: return "InvalidFormat";
    case PSUStatusNotFound: return "NotFound";
    case PSUStatusOutOfMemory: return "OutOfMemory";
    case PSUStatusIoError: return "IoError";
    default: return "Unknown";
    }
}
