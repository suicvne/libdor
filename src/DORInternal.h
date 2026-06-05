#ifndef DOR_INTERNAL_H
#define DOR_INTERNAL_H

#include "common.h"

#define DORGameDataSize 0x100F0u        /** @brief Size of game data save in bytes:  65,776 bytes */
#define DORCardRecordSize 0x4Cu         /** @brief Size of a single card record: 76 bytes */
#define DORCardRecordsOffset 0x00000u   /** @brief Offset into save data where card records exist */
#define DORChecksumOffset 0x00002u      /** @brief Offset into save data where checksum exists */
#define DORDeckBlockOffset 0x0FD88u     /** @brief Offset into where the deck block exists (All 3 decks, Deck A starts here) */
#define DORDeckCardsOffset 0x0FD98u     /** @brief Offset into where first deck cards start */
#define DORDeckLeaderOffset 0x0FDE8u    /** @brief Offset into where deck leader data is */
#define DORProfileBlockOffset 0x0FF9Cu  /** @brief Offset into save where profile block is. Player name, etc. */
#define DORPlayerNameOffset 0x0FFC8u    /** @brief Offset into save where player name specifically is. */

struct DORSave {
    uint8_t* pBytes;
    size_t ByteCount;
};

/**
 * @brief Card ID -> static UTF8 name lookup.
 * @returns static UTF8 string representing name of card given ID.
 */
const char* DORCardNameLookup(uint16_t CardId);

/**
 * @brief Decodes a singular Duelists of the Roses character to a UTF8 char.
 * @param [in] Code DOR Char Code
 * @returns UTF8 char
 */
char DORText_DecodeCharacter(uint8_t Code);

/**
 * @brief Encodes singular UTF8 character into a DOR Char Code.
 * @param [in] Character UTF8 char code.
 * @param [out] pOutCode Pointer to store DOR Char code
 * @returns Status 1 = Success; 0 = Failure.
 */
int DORText_EncodeCharacter(char Character, uint8_t* pOutCode);

#endif
