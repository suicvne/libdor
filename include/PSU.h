/**
 * @file PSU.h
 * @author Mike Santiago (admin@ignoresolutions.xyz)
 * @brief Top level header containing API definitions for interacting with
 *        PSU save archives.
 * @copyright Copyright (c) 2026
 * @defgroup libdor_psu PSU IO
 * @{
 */


#ifndef LIBDOR_PSU_H
#define LIBDOR_PSU_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PSUStatus Reprents possible status codes for a PSU save operation.
 */
typedef enum PSUStatus {
    PSUStatusOk = 0,
    PSUStatusInvalidArgument,
    PSUStatusInvalidFormat,
    PSUStatusNotFound,
    PSUStatusOutOfMemory,
    PSUStatusIoError
} PSUStatus;

/**
 * @brief PSUArchive Opaque structure representing a loaded .psu archive file.
 */
typedef struct PSUArchive PSUArchive;

/**
 * @brief PSUEntryInfo Represents a single entry in a .psu archive.
 */
typedef struct PSUEntryInfo {
    uint32_t Mode;          /**< mode of the entry */
    uint32_t Size;          /**< byte size of entry */
    uint32_t Type;          /**< type of entry */
    size_t EntryOffset;     /**< offset of entry  */
    size_t PayloadOffset;   /**< data payload offset */
    char Name[256];         /**< Friendly name of the entry */
} PSUEntryInfo;

// =========================================== PSUArchive Ctors/Dtors ===========================================

/**
 * @brief Loads a PSU archive from a given file path.
 * @param [in] pPath Path to a psu file on disk.
 * @param [out] ppOutArchive Pointer to where to place the loaded PSUArchive structure.
 * @returns Status code indicating whether we were able to load the .psu file.
 */
PSUStatus PSUArchive_CreateFromFile(const char* pPath, PSUArchive** ppOutArchive);

/**
 * @brief Loads a PSU archive from a given lump of bytes.
 * @param [in] pBytes Lump of bytes representing a psu archive.
 * @param [in] ByteCount Number of bytes in pBytes.
 * @param [out] ppOutArchive Pointer to where to place the loaded PSUArchive structure.
 * @returns Status code indicating whether we were able to load the .psu archive.
 */
PSUStatus PSUArchive_CreateFromBytes(const uint8_t* pBytes, size_t ByteCount, PSUArchive** ppOutArchive);

/**
 * @brief Destroys a loaded PSU archive, freeing any associated memory
 * @param [in] pArchive Archive to free. pArchive itself is not free'd.
 */
void PSUArchive_Destroy(PSUArchive* pArchive);

// =========================================== PSUArchive Ctors/Dtors ===========================================

// ============================================= PSUArchive Members =============================================

/**
 * @brief Returns the number of entries inside of a given PSUArchive.
 *        Similar to entries in a zip file.
 * @param [in] pArchive Pointer to PSUArchive.
 * @returns Number of entries found in this PSUArchive.
 */
size_t PSUArchive_GetEntryCount(const PSUArchive* pArchive);

/**
 * @brief Attempts to grab the entry info for a given entry from its index.
 * @param [in] pArchive Pointer to PSUArchive.
 * @param [in] Index Index of which entry to grab. Must be <EntryCount.
 * @param [out] pOutInfo Pointer to where to place the corresponding PSUEntryInfo.
 * @returns Status indicating if the entry was successfully retrieved or not.
 */
PSUStatus PSUArchive_GetEntryInfo(const PSUArchive* pArchive, size_t Index, PSUEntryInfo* pOutInfo);

/**
 * @brief Iterates entries in the archive attempting to find one matching the given pName.
 * @param [in] pArchive Pointer to PSUArchive.
 * @param [in] pName Name of the file to try and find.
 * @param [out] pOutInfo Pointer to where to place the corresponding PSUEntryInfo.
 * @returns Status indicating if the entry was successfully retrieved or not.
 */
PSUStatus PSUArchive_FindFile(const PSUArchive* pArchive, const char* pName, PSUEntryInfo* pOutInfo);

/**
 * @brief Returns the raw byte data for a given entry in the PSUArchive.
 * @param [in] pArchive Pointer to PSUArchive.
 * @param [in] pEntry Pointer to PSUEntryInfo. Where are we retrieving the bytes from?
 * @param [out] ppOutBytes Pointer to where we can allocate and place the read data.
 * @param [out] pOutByteCount Pointer to where to place the amount of bytes that were allocated in *ppOutBytes.
 * @return Status indicating if we were able to retrieve the file data or not.
 */
PSUStatus PSUArchive_GetFileData(const PSUArchive* pArchive, const PSUEntryInfo* pEntry, const uint8_t** ppOutBytes, size_t* pOutByteCount);

/**
 * @brief Attempts to set raw byte data for a given entry in the PSUArchive.
 * @param [in] pArchive Pointer to PSUArchive.
 * @param [in] pEntry Pointer to PSUEntryInfo. Where are we retrieving the bytes from?
 * @param [in] pBytes Pointer to bytes to write into the archive.
 * @param [in] ByteCount Number of bytes in pBytes.
 * @returns Status indicating if we were able to write the data or not.
 */
PSUStatus PSUArchive_SetFileData(PSUArchive* pArchive, const PSUEntryInfo* pEntry, const uint8_t* pBytes, size_t ByteCount);

/**
 * @brief Attempts to write the PSUArchive out to a .psu file.
 * @param [in] pArchive Pointer to PSUArchive.
 * @param [in] pPath Path to write the file to.
 * @returns Status indicating if we were able to write the data or not.
 */
PSUStatus PSUArchive_WriteToFile(const PSUArchive* pArchive, const char* pPath);

/**
 * @brief Stringifies a PSUStatus code.
 * @returns Static UTF8 encoded C-string representing the given PSUStatus
 */
const char* PSUStatus_ToString(PSUStatus Status);

// ============================================= PSUArchive Members =============================================

#ifdef __cplusplus
}
#endif

/** @} */

#endif // LIBDOR_PSU_H
