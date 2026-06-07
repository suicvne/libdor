#ifndef DOR_INTERNAL_H
#define DOR_INTERNAL_H

#include "common.h"

#define DORGameDataSize 0x100F0u        /**< Size of game data save in bytes:  65,776 bytes */
#define DORCardRecordSize 0x4Cu         /**< Size of a single card record: 76 bytes */
#define DORCardCopySlotOffset 0x04u     /**< Offset inside a card record where copy slots begin */
#define DORCardCopySlotSize 0x08u       /**< Size of one per-card copy state slot */
#define DORCardRecordsOffset 0x00000u   /**< Offset into save data where card records exist */
#define DORChecksumOffset 0x00002u      /**< Offset into save data where checksum exists */
#define DORDeckBlockOffset 0x0FD88u     /**< Offset into where the deck block exists (All 3 decks, Deck A starts here) */
#define DORDeckCardsOffset 0x0FD98u     /**< Offset into where first deck cards start */
#define DORDeckLeaderOffset 0x0FDE8u    /**< Offset into where deck leader data is */
#define DORDeckRecordSize 0x54u         /**< Size of one saved deck record: 40 card IDs, leader ID, and unknown value */
#define DORProgressRecentCardIdsOffset 0x0FD8Cu /**< Offset into pre-deck recent/acquired card IDs. */
#define DORProfileBlockOffset 0x0FF9Cu  /**< Offset into save where profile block is. Player name, etc. */
#define DORProfileTokenSize 0x04u       /**< Size of the profile validation token. */
#define DORPlayerNameOffset 0x0FFC8u    /**< Offset into save where player name specifically is. */
#define DORProgressProfileStateOffset 0x0FF9Cu /**< Offset into observed profile/campaign state bytes. */
#define DORProgressFooterStateOffset 0x10068u  /**< Offset into observed footer campaign/progression state bytes. */

struct DORSave {
    uint8_t* pBytes;  /**< ptr to raw bytes of DOR Save */
    size_t ByteCount; /**< length of bytes in pBytes */
};

/**
 * @brief Card ID -> static ASCII name lookup.
 * @returns static ASCII string representing name of card given ID.
 */
const char* DORCardNameLookup(uint16_t CardId);

/**
 * @brief Decodes a singular Duelists of the Roses character to an ASCII char.
 * @param [in] Code DOR Char Code
 * @returns ASCII char
 */
char DORText_DecodeCharacter(uint8_t Code);

/**
 * @brief Encodes singular ASCII character into a DOR Char Code.
 * @param [in] Character ASCII char code.
 * @param [out] pOutCode Pointer to store DOR Char code
 * @returns Status 1 = Success; 0 = Failure.
 */
int DORText_EncodeCharacter(char Character, uint8_t* pOutCode);

#endif
