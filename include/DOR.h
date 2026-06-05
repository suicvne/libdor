/**
 * @file DOR.h
 * @author Mike Santiago (admin@ignoresolutions.xyz)
 * @brief Top level header containing API definitions for interacting with
 *        Yu-Gi-Oh! Duelists of the Roses saves.
 * @copyright Copyright (c) 2026
 * @defgroup libdor DOR Structures
 * @{
 */

#ifndef LIBDOR_H
#define LIBDOR_H

#include "PSU.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum number of cards in the entire game. Also represents maximum ID. */
#define DORCardCount            854u

/** @brief Maximum number of cards in a single deck. */
#define DORDeckCardCount        40u

/** @brief Maximum number of chars in a save name */
#define DORNameCharacterCount   12u

/** @brief ID used to represent an empty or invalid card ID */
#define DOREmptyCardId          999u

/** @brief Name of the save data file name in save file for NTSC. */
#define DORSaveFileNameNTSC "BASLUS-20515"

/**
 * @brief DORStatus Status codes for the API and any associated calls.
 */
typedef enum DORStatus {
    DORStatusOk = 0,
    DORStatusInvalidArgument,
    DORStatusInvalidFormat,
    DORStatusNotFound,
    DORStatusOutOfMemory
} DORStatus;

/**
 * @brief DORRank Enumeration representing deck leader ranks.
 */
typedef enum DORRank {
    DORRankNCO = 0,
    DORRank2LT,
    DORRank1LT,
    DORRankCPT,
    DORRankMAJ,
    DORRankLTC,
    DORRankCOL,
    DORRankBG,
    DORRankRADM,
    DORRankVADM,
    DORRankADM,
    DORRankSADM,
    DORRankSD
} DORRank;

/**
 * @brief DORLeaderAbility Enumeration representing deck leader abilities.
 */
typedef enum DORLeaderAbility {
    DORLeaderAbilityNone = 0,

    DORLeaderAbilityDestroySpecificEnemyType,                 /** @brief Destroys enemy Monster cards of a specific Type when they enter Support Range. */
    DORLeaderAbilityDirectDamageHalved,                       /** @brief Halves LP damage from direct attacks. */
    DORLeaderAbilityExtendedSupportRange,                     /** @brief Increases the Deck Leader's Support Range by 1 square. */
    DORLeaderAbilityImprovedResistanceForSameTypeFriendlies,  /** @brief Protects same-Type friendly Monster Cards within Support Range from Spell and Trap effects. */
    DORLeaderAbilityIncreasedMovement,                        /** @brief Increases the Deck Leader's movement range by 1 square. */
    DORLeaderAbilityIncreasedStrengthForSameTypeFriendlies,   /** @brief Increases ATK/DEF of same-Type friendly Monster Cards within Support Range. */
    DORLeaderAbilityLevelCostReductionForSummoningSameType,   /** @brief Reduces the Summoning Cost of Monster cards of the same Type. */
    DORLeaderAbilityLPRecovery,                               /** @brief Recovers 50 LP at the end of the player's turn. */
    DORLeaderAbilityMovementBoostForSameTypeFriendlies,       /** @brief Grants Movement Bonus to same-Type friendly Monster Cards within Support Range. */
    DORLeaderAbilityOpenOpponentsCard,                        /** @brief Flips enemy cards face-up when they enter Support Range. */
    DORLeaderAbilitySpellbindSpecificEnemyType,               /** @brief Spellbinds enemy Monster Cards of a specific Type when they enter Support Range. */
    DORLeaderAbilityTerrainChange,                            /** @brief Transforms the traveled space into a specific terrain type when moving. */
    DORLeaderAbilityWeakenSpecificEnemyType,                  /** @brief Decreases ATK/DEF of enemy Monster Cards of a specific Type within Support Range. */
    DORLeaderAbilityBonusSlotReels,                           /** @brief Hidden ability that adds lines for three-in-a-rows in the Graveyard Slot Machine. */
    DORLeaderAbilityFind,                                     /** @brief Hidden ability that can obtain secret cards by moving to certain spaces. */
    DORLeaderAbilityDestinyDraw,                              /** @brief Hidden ability that changes the player's draw in certain situations. */
} DORLeaderAbility;

/** @brief DORDeckID Enumeration representing which saved deck to query. */
typedef enum DORDeckID {
    A, B, C
} DORDeckID;

/**
 * @brief DORSave Opaque structure representing a loaded save file.
 */
typedef struct DORSave DORSave;

/**
 * @brief DORCardInfo Represents a singular card information entry in the save data.
 *        There should be one for each card in the game in a save.
 */
typedef struct DORCardInfo {
    uint16_t CardId;                /**< ID of the card from 000 - 854 */
    uint8_t  QuantityOrOwned;       /**< 0 if not owned, quantity otherwise 0 - 9 */
    uint8_t  Flags;                 /**< Flags specific to this card, these are still relatively unknown. */
    uint16_t Experience;            /**< The amount of experience points this card has. 0 - 65535 */
    uint32_t StateMarker;           /**< State marker unique to each save/name. */
    uint32_t Unknown08;             /**< Currently unknown, padding bytes? */
} DORCardInfo;

/**
 * @brief DORDeckInfo Represents a singular deck. There are 3 per save.
 *        Cards are stored as IDs. Empty or incomplete saved decks may use
 *        DOREmptyCardId in card and leader slots.
 */
typedef struct DORDeckInfo {
    uint16_t Cards[DORDeckCardCount]; /**< ID of each card in the deck, or DOREmptyCardId. */
    uint16_t LeaderCardId;            /**< Card ID of the leader, or DOREmptyCardId. */
    uint16_t StoredDeckCost;          /**< Save's stored deck cost for this deck. Probably to be validated against for anti-tampering. */
} DORDeckInfo;

// ======================================= DORSave Ctors/Dtors =======================================

/**
 * @brief Creates a DORSave structure from a pointer to a PSUArchive.
 *        .psu file -> DORSave structure.
 * @param [in] pArchive The PSUArchive to read the save entry from.
 * @param [out] ppOutSave Pointer to place the loaded DORSave structure.
 * @returns DORStatus representing if the save was loaded ok.
 */
DORStatus DORSave_CreateFromPSUArchive(const PSUArchive* pArchive, DORSave** ppOutSave);

/**
 * @brief Creates a DORSave structure from a pointer to loose save bytes.
 *        The .psu load path uses this once the actual save file structure bytes are found
 *        in the .psu archive.
 * @param [in] pBytes Lump of bytes representing the save.
 * @param [in] ByteCount Number of bytes in pBytes
 * @param [out] ppOutSave Pointer to place the loaded DORSave structure.
 */
DORStatus DORSave_CreateFromBytes(const uint8_t* pBytes, size_t ByteCount, DORSave** ppOutSave);

/**
 * @brief Destructs a given DORSave, freeing any allocated memory as needed.
 *        pSave will also be free'd. Use caution if this was a stack allocated save.
 * @param [in] pSave The save to destroy.
 */
void DORSave_Destroy(DORSave* pSave);

// ======================================= DORSave Ctors/Dtors =======================================

// ========================================= DORSave Members =========================================

/**
 * @param [in] pSave Pointer to the save structure.
 * @returns Size of save in bytes.
 */
size_t DORSave_GetSize(const DORSave* pSave);

/**
 * @param [in] pSave Pointer to the save structure.
 * @returns Raw bytes for the save.
 */
const uint8_t* DORSave_GetBytes(const DORSave* pSave);

/**
 * @brief Returns the calculated checksum for the save.
 *        Seems to be calculated based on player name.
 * @param [in] pSave Pointer to the save structure.
 * @returns The 16-byte checksum for this save.
 */
uint16_t DORSave_GetChecksum(const DORSave* pSave);

/**
 * @brief Attempts to grab card information for a given card ID from the save.
 * @param [in] pSave Pointer to the save structure.
 * @param [in] CardId The Card ID to retrieve information for.
 * @param [out] pOutInfo Pointer to place DORCardInfo structure information in.
 * @returns Status indicating if the card could be retrieved or not. Will return an error
 *          code if the card doesn't exist.
 */
DORStatus DORSave_GetCardInfo(const DORSave* pSave, uint16_t CardId, DORCardInfo* pOutInfo);

/**
 * @brief Attempts to grab deck information from the save.
 * @param [in] pSave Pointer to the save structure.
 * @param [in] DeckID Which deck should we query?
 * @param [out] pOutInfo Pointer to place DORDeckInfo structure in.
 * @returns Status indicating if the deck information was retrieavable or not.
 */
DORStatus DORSave_GetDeckInfo(const DORSave* pSave, DORDeckID DeckID, DORDeckInfo* pOutInfo);

/**
 * @brief Attempts to grab the players name as an ASCII C string.
 *        The player's name has its own encoding in Duelists of the Roses. This translates it
 *        to ASCII for you.
 *
 *        NOTE: Max player character count is 12.
 *        NOTE: Not all characters have been mapped.
 * @param [in] pSave Pointer to the save structure.
 * @param [out] pOutBuffer Pointer to a char buffer to place ASCII player name in.
 * @param [in] OutBufferSize Size of the buffer pointed to by pOutBuffer.
 * @returns Status indicating if the player's name was able to be written out or not.
 */
DORStatus DORSave_GetPlayerName(const DORSave* pSave, char* pOutBuffer, size_t OutBufferSize);

/**
 * @brief Attempts to set the player's name on the save given an ASCII C string.
 *        The player's name will be translated to Duelists of the Roses encoding.
 *
 *        Simultaneously, this will update the checksum of the save.
 */
DORStatus DORSave_SetPlayerName(DORSave* pSave, const char* pName);

// ========================================= DORSave Members =========================================

// ========================================= Misc Members =========================================

/**
 * @brief Returns ASCII name of a card based on its card ID.
 * @param [in] CardId ID of the card to lookup.
 * @returns Static ASCII C string of the card name, NULL otherwise.
 */
const char* DORCard_GetName(uint16_t CardId);

/**
 * @brief Stringifies DORStatus.
 * @param [in] Status Status to stringify
 * @returns Static ASCII string representing the status.
 */
const char* DORStatus_ToString(DORStatus Status);

/**
 * @brief Stringifies DORRank.
 * @param [in] Rank Rank to stringify
 * @returns Static ASCII string representing the rank.
 */
const char* DORRank_ToString(DORRank Rank);

/**
 * @brief Calculates rank based on experience.
 * @param [in] Experience Experience points for a given card.
 * @returns Rank calculated from experience.
 */
DORRank DORRank_FromExperience(uint16_t Experience);

/**
 * @brief Decodes DOR encoded text out to an ASCII C string.
 * @param [in] pEncodedText Pointer to DOR encoded text.
 * @param [in] CharacterCount Len of chars in pEncodedText.
 * @returns Allocated ASCII C string containing the decoded text.
 */
const char* DORText_Decode(const uint16_t* pEncodedText, size_t CharacterCount);

/**
 * @brief Decodes DOR encoded text to a specified buffer.
 * @param [in] pEncodedText Pointer to DOR encoded text.
 * @param [in] CharacterCount Len of chars in pEncodedText.
 * @param [in,out] pOutBuffer Pointer to char buffer to place decoded text.
 * @param [in] OutBufferSize Size of buffer provided by pOutBuffer.
 */
DORStatus DORText_DecodeToBuffer(const uint16_t* pEncodedText, size_t CharacterCount, char* pOutBuffer, size_t OutBufferSize);

// ========================================= Misc Members =========================================


#ifdef __cplusplus
}
#endif

/** @} */

#endif // LIBDOR_H
