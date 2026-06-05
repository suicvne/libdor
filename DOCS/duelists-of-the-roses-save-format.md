# Yu-Gi-Oh! The Duelists of the Roses Save Format Notes

Samples examined:

`/Users/mike/Desktop/SLUS-20515 Yu-Gi-Oh! DOR (2EE9A403).psu`

`/Users/mike/Desktop/SLUS-20515 Yu-Gi-Oh! DOR (CF1F9BC9) Mike12345678.psu`

`/Users/mike/Desktop/SLUS-20515 Yu-Gi-Oh! DOR (80C2ED98) LAST SYMBOLS.psu`

`/Users/mike/Desktop/SLUS-20515 Yu-Gi-Oh! DOR LAST SYMBOLS WHITE ROSE START.psu`

`/Users/mike/Desktop/SLUS-20515 Yu-Gi-Oh! DOR (68A3508B) 100 COMPLETE.psu`

This is an observation log from PSU exports. Offsets below use little-endian values unless noted.

## External Card Numbering Anchor

The public Duelists of the Roses card list says the game has 854 playable cards numbered `000` through `853`, plus two unusable story cards. It also identifies `078` as `Magician of Black Chaos`.

Sources:

- https://yugioh.fandom.com/wiki/List_of_Yu-Gi-Oh%21_The_Duelists_of_the_Roses_cards
- https://yugioh.fandom.com/wiki/Magician_of_Black_Chaos_%28DOR%29

Those facts match this save: record/index `78` has the strongest deck-leader-looking mutation, and one tail card list includes card id `78`.

## PSU Container Layout

The `.psu` is not a raw game save. It is a PS2 memory-card export containing directory entries and file payloads.

Each PSU entry record is `0x200` bytes. File payloads immediately follow the file entry and are padded to a `0x400` boundary for the next entry.

Observed entries:

| PSU offset | Kind | Name | Size | Payload offset | Notes |
|---:|---|---|---:|---:|---|
| `0x00000` | dir | `BASLUS-20515` | `7` | n/a | save directory |
| `0x00200` | dir | `.` | `0` | n/a | directory entry |
| `0x00400` | dir | `..` | `0` | n/a | directory entry |
| `0x00600` | file | `icon.sys` | `0x03C4` | `0x00800` | PS2 save icon metadata |
| `0x00C00` | file | `icon00.ico` | `0xE2A8` | `0x00E00` | memory-card icon model |
| `0x0F200` | file | `icon02.ico` | `0xE6A8` | `0x0F400` | memory-card icon model |
| `0x1DC00` | file | `icon01.ico` | `0xE2A8` | `0x1DE00` | memory-card icon model |
| `0x2C200` | file | `BASLUS-20515` | `0x100F0` | `0x2C400` | game data member |

The rest of this document uses offsets relative to the `BASLUS-20515` game data member payload at PSU offset `0x2C400`.

## Game Data Member Overview

Game member size: `0x100F0` bytes.

High-level layout:

| Relative offset | Size | Observed purpose | Confidence |
|---:|---:|---|---|
| `0x00000` | `0x0FD88` | `854 * 0x4C` fixed-size card-index records, including nine per-card copy slots | high |
| `0x0FD88` | about `0x190` | saved deck lists and related card slots | high for saved decks A/B/C |
| `0x0FE90` | about `0x10C` | long `0x07` fill / unknown packed state | low |
| `0x0FF9C` | about `0x64` | profile/deck-leader metadata and custom name | high for name anchor |
| `0x10000` | `0x0F0` | mostly zero, small flags at `0x1006A` | low |

## Card-Index Record Region

The first `0x0FD88` bytes divide exactly into 854 records:

```text
raw_record_offset = raw_record_index * 0x4C
record_size   = 0x4C
raw_record_index range = 0..853
```

Copy-slot counts match public card IDs when read from `card_id * 0x4C`. The older `DORSave_GetCardInfo` leader XP/marker path uses the pre-existing library mapping of `card_id + 1`; that offset difference is preserved for backward compatibility and still needs a dedicated explanation.

Common record shape:

| Record offset | Size | Common value / notes |
|---:|---:|---|
| `+0x00` | 1 | card header byte. This is not a reliable chest count; values in the complete save are mostly `0x08` and `0x25` |
| `+0x01` | 1 | flags/status byte. Purpose still unknown |
| `+0x02` | 2 | little-endian leader/card experience anchor for known leaders |
| `+0x04..+0x4B` | `9 * 8` | nine per-card copy state slots |

Each copy slot is 8 bytes:

```text
0x04 + copy_index * 0x08
copy_index range = 0..8
```

Observed copy slot states:

| Slot bytes | Observed meaning | Confidence |
|---|---|---|
| `00 00 00 00 00 00 00 C0` | empty/unowned copy slot, observed in a low-inventory save | high |
| `25 05 62 67 00 00 00 C0` | occupied slot state in the 100% complete save; exact chest/deck meaning is not universal | medium |
| `25 05 62 67 00 00 00 40` | occupied slot state, observed on cards assigned to Deck B in the 100% complete save | medium |
| `25 05 62 67 25 05 62 67` | occupied slot state, observed on cards assigned to Deck A in the 100% complete save | medium |
| `25 05 62 E7 00 00 00 40` | observed leader-marker variant, confirmed on Exodia as Deck B leader | medium |

The complete save was reported to have `7595` cards in the chest. Counting all `25 05 62 67 00 00 00 C0` slots across `854 * 9` slots gives exactly `7595` in that save, but a low-inventory save shows the actual empty slot pattern is `00 00 00 00 00 00 00 C0`. Therefore `25 05 62 67 00 00 00 C0` is an occupied state in the complete save, not a universally confirmed "chest copy" state.

`DORCardInfo_GetTotalCopyCount` currently counts slots that are not one of the observed empty states:

```text
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 C0
```

This matches the checksum-test save where most cards have one copy, and known Deck A duplicates have two occupied slots.

Example complete-save anchors using raw copy-slot records:

| Card id | Card | Known state | Observed slots |
|---:|---|---|---|
| `1` | Seiyaryu | 8 copies in chest | 8 chest slots, 1 deck-assigned slot |
| `2` | Blue-Eyes Ultimate Dragon | 7 copies in chest, 2 in Deck B | 7 chest slots, 2 deck-assigned slots |
| `3` | Kaiser Dragon | 9 copies in chest | 9 chest slots |
| `4` | Blackland Fire Dragon | 9 copies in chest | 9 chest slots |
| `5` | Koumori Dragon | 8 copies in chest, 1 in Deck A | 8 chest slots, 1 deck-assigned slot |
| `58` | Exodia the Forbidden One | 8 copies in chest, 1 assigned to Deck B as leader | 8 chest slots, 1 `E7` leader-marker slot |

Example checksum-test anchors:

| Card id | Card | Known state | Observed occupied slots |
|---:|---|---|---|
| `26` | Petit Dragon | 2 copies assigned to Deck A | 2 |
| `133` | Moon Envoy | 2 copies assigned to Deck A | 2 |
| `185` | Masaki the Legendary Swordsman | 2 copies assigned to Deck A | 2 |
| `244` | Master & Expert | 2 copies assigned to Deck A | 2 |
| `370` | Orion the Battle King | 2 copies assigned to Deck A | 2 |
| `373` | Spirit of the Harp | 2 copies assigned to Deck A | 2 |
| `559` | Arma Knight | 2 copies assigned to Deck A | 2 |

### Deck Leader Anchor: Card 078

Card record `78` begins at:

```text
0x1728 = 78 * 0x4C
```

Bytes:

```text
00 40 00 00 25 05 62 E7 00 00 00 00 00 00 00 00
00 00 00 C0 00 00 00 00 00 00 00 C0 ...
```

Compared with the common marker `25 05 62 67`, this record has `25 05 62 E7`. Since the sample's deck leader is known to be `Magician of Black Chaos`, and that card is public id `078`, this is currently the best deck-leader/rank-state anchor.

Hypothesis:

- `record[card_id=78] + 0x07 == 0xE7` marks a deck leader copy or related leader-state bit.
- `+0x08..+0x0B == 00 00 00 00` may participate in the same leader-state field.
- `SD` rank is likely represented somewhere in this record or the tail profile block, but with one sample it is not separable from "active leader".

## Tail Deck And Card-ID Lists

Starting around `0x0FD88`, there are contiguous little-endian `u16` values that decode as valid card ids.

The second sample clarifies the saved deck boundary. There are three saved decks, each `0x54` bytes:

| Relative offset | Type | Observed meaning |
|---:|---|---|
| `0x0FD88` | `u16` | unknown header/status |
| `0x0FD8A` | `u16` | unknown header/status |
| `0x0FD8C..0x0FD97` | `u16[6]` | pre-deck slots or prior state; values can be `999` sentinels |
| `0x0FD98 + deck_index * 0x54` | `u16[40]` | saved deck's 40-card list |
| `0x0FDE8 + deck_index * 0x54` | `u16` | saved deck's leader card id |
| `0x0FDEA + deck_index * 0x54` | `u16` | saved deck's stored deck cost |

Deck index range is `0..2` for decks A, B, and C.

`999` / `0x03E7` appears to be an empty-slot sentinel in the tail deck/list area.

### Active Deck: Atem / Magician of Black Chaos

```text
deck @ 0x0FD98:
478, 674, 53, 704, 87, 81, 191, 614, 26, 36,
43, 661, 853, 297, 394, 734, 694, 762, 95, 60,
685, 377, 699, 801, 705, 712, 716, 625, 146, 754,
498, 795, 796, 417, 803, 804, 780, 823, 824, 828

leader @ 0x0FDE8:
78

unknown @ 0x0FDEA:
792
```

### Active Deck: Mike12345678 / Airknight Parshath

Airknight Parshath is public DOR card id `392`.

```text
deck @ 0x0FD98:
26, 26, 33, 50, 88, 133, 133, 172, 184, 185,
185, 207, 232, 244, 244, 275, 277, 278, 283, 370,
370, 373, 373, 374, 378, 383, 391, 393, 463, 542,
543, 559, 559, 657, 661, 691, 735, 763, 781, 804

leader @ 0x0FDE8:
392

unknown @ 0x0FDEA:
822
```

Notes:

- Saved deck and leader fields are anchored by independent samples: leader `78` for Magician of Black Chaos, leader `392` for Airknight Parshath, leader `612` for Maiden of the Aqua, and leader `58` for Exodia the Forbidden One.
- Most actual deck entries are in the valid playable-card range `0..853`.
- `999` is outside the playable-card range and is currently interpreted as an empty-slot sentinel.

### Complete Save Decks

The 100% complete sample has:

```text
Deck A:
leader=612 (Maiden of the Aqua), rank=RADM, cost=823
reported abilities:
  - Extended Support Range
  - Increased strength for same type friendlies

Deck B:
leader=58 (Exodia the Forbidden One), rank=RADM, cost=1764
reported abilities:
  - Increased strength for same type friendlies

Deck C:
empty, leader=999, cost=0
```

The leader ability list is not yet proven to be stored directly in the save. Current evidence points toward ability availability being derived from card id plus rank/experience through static game data, while the save stores the leader card id, experience, and copy-slot state.

## Long `0x07` Region

From roughly `0x0FEB0` to `0x0FF9B`, the file is dominated by byte `0x07`.

Observed as `u16`, this appears as repeated `0x0707` (`1799` decimal).

Possible interpretations:

- packed card-count or unlock-state bytes;
- a fixed fill/sentinel region;
- a maximum value field, since the sample is described as having at least one of every card.

This is not confirmed. The region is too short to be a simple one-byte-per-card inventory table for 854 cards, but it is large enough for some packed representation.

## Profile / Name Block

At `0x0FF9C`, a compact metadata block begins.

Raw bytes:

```text
0x0FF9C: 42 29 CF 0D 1C 00 00 00 00 00 00 01 03 01 00 00
0x0FFAC: 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00
0x0FFBC: 00 00 00 00 00 00 00 00 56 20 86 20 77 20 7F A0
0x0FFCC: 1E 00 1E 00 1E 00 1E 00 1E 00 1E 00 1E 00 1E 00
0x0FFDC: 01 00 00 00 ...
```

The custom leader/player name `Atem` appears at `0x0FFC8` in a game-specific character encoding:

| Character | Encoded bytes | Low byte |
|---|---:|---:|
| `A` | `56 20` | `0x56` |
| `t` | `86 20` | `0x86` |
| `e` | `77 20` | `0x77` |
| `m` | `7F A0` | `0x7F` |

The second sample name `Mike12345678` appears at the same offset:

| Character | Encoded bytes | Low byte |
|---|---:|---:|
| `M` | `62 20` | `0x62` |
| `i` | `7B 20` | `0x7B` |
| `k` | `7D 20` | `0x7D` |
| `e` | `77 20` | `0x77` |
| `1` | `8F 20` | `0x8F` |
| `2` | `90 20` | `0x90` |
| `3` | `91 20` | `0x91` |
| `4` | `92 20` | `0x92` |
| `5` | `93 20` | `0x93` |
| `6` | `94 20` | `0x94` |
| `7` | `95 20` | `0x95` |
| `8` | `96 A0` | `0x96` |

Hypothesis:

- Character codes are stored in the low byte.
- The high byte `0x20` marks a normal character.
- The final character has high byte `0xA0`, possibly an end-of-name/final-character flag.
- The following `1E 00` fill values may be blank character slots or name-field padding.

This is not ASCII or Shift-JIS.

## Rose / Campaign Side State

Two controlled start saves have the same name, starting deck, leader, leader rank, and deck cost, but different campaign side:

```text
red side start:   8 red rose cards, 0 white rose cards
white side start: 0 red rose cards, 8 white rose cards
```

The clean differing regions are in the profile/footer state, not the saved deck block:

```text
profile candidate: 0x0FF9C..0x0FFB3
footer candidate:  0x10068..0x1006E
```

Footer bytes:

```text
red side start   @ 0x10068: 00 00 01 01 00 00
white side start @ 0x10068: 0B 00 0C 0B 00 02
100% complete    @ 0x10068: 14 00 0B 0B 0B 01 05
```

No byte sequence has been confirmed as a literal red/white rose count. The observed data does not contain a clean `08`, `08 08`, or `16` counter matching the UI facts. Current best interpretation is that these regions store campaign side/progression/reincarnation state from which the game derives rose UI state.

The complete save also has reincarnation available, so the `0x10068..0x1006E` cluster must not be treated as rose-only until controlled before/after reincarnation samples are compared.

## Newcomer Sort State

The save's user-facing "newcomer sort" orders cards by acquisition order. The exact field is not confirmed.

Current candidates:

- card record header bytes `+0x00..+0x03`;
- ordering/state inside the nine copy slots;
- another profile/footer sequence not yet isolated.

The observed copy-slot structure narrows the search, but a before/after acquisition sample is still required to identify newcomer sort metadata.

## Remaining Footer

From `0x10000` through the end (`0x100F0`) the file is mostly zero.

Only one small nonzero cluster was seen:

```text
relative 0x1006A: 01 01
```

Purpose unknown.

## Current Confidence Summary

High confidence:

- `.psu` container entries and payload boundaries.
- Game save member is the inner `BASLUS-20515` payload at PSU offset `0x2C400`, size `0x100F0`.
- The first `0x0FD88` bytes are 854 fixed-size records of `0x4C` bytes each.
- Record index/card-id anchors match DOR card numbers at least for ids `078` and `392`.
- Each card record contains nine 8-byte copy slots at `+0x04..+0x4B`.
- `00 00 00 00 00 00 00 C0` is an empty/unowned copy slot in the checksum-test save.
- `25 05 62 67 00 00 00 C0` is an occupied slot state in the 100% complete save; counting it gives the reported `7595` chest total in that save, but it is not universal enough to call a standalone chest marker.
- Three saved deck records are spaced by `0x54` bytes, with deck cards, leader id, and stored deck cost.
- Custom names are stored at relative offset `0x0FFC8` in a game-specific two-byte-per-character encoding.

Medium confidence:

- `25 05 62 67 00 00 00 40` and `25 05 62 67 25 05 62 67` mark deck-assigned copies.
- `25 05 62 E7 ...` marks a leader-copy state or related leader marker.
- Leader abilities are likely derived from card id plus rank/experience rather than stored as a direct save-field list.

Low confidence:

- Exact meaning of card-record fields `+0x00..+0x03`.
- Exact meaning of `SD` rank and where rank is represented.
- Exact red/white rose card count storage.
- Exact newcomer sort metadata.
- Exact checksum/hash behavior. Repeated sentinels `42 29 CF 0D` and `25 05 62 67` may be constants, checksums, float-like runtime values, or a mixture.

## Next Samples To Confirm

The next best diffs would be:

1. Same save with only the custom name changed.
2. Same save with the same leader but a different leader rank.
3. Same save with a different deck leader.
4. Same save before/after acquiring one known card.
5. Same save before/after reincarnation becomes available, and before/after using reincarnation.
6. Same save with one card added/removed from a saved deck.

Those would isolate name encoding, leader/rank flags, newcomer sort metadata, reincarnation/progression state, and remaining deck assignment variants.
