# libdor
A C library for interacting with Yu-Gi-Oh! Duelists of the Roses PS2 saves stored in .psu files. 

## Overview
By itself, `libdor` exists only to provide a library to read (and, eventually write) save file data for the PS2 game Yu-Gi-Oh! Duelists of the Roses.
The intention is that this library can be glued into other tools relating to the game, or even glued into a game that mimics Duelists of the Roses, and then allow direct use of save data.

Please note, we have currently only tested the US SKU SLUS 20515.

## Pending
- Determining where deck leader ability information is stored
- Determining where rose card count is stored.
- Determining where "newcomer sort" metadata is stored.
- Writing card data
- Purpose of `Flags` on `DORCardInfo`
- Purpose of `UnknownAfterLeader` on `DORDeckInfo`.
- Independence from psu format.

## How it works
`libdor` compiles to a static C23 library, and exposes a series of functions to allow reading save files. PS2 save files have a variety of containers they are held within, and `libdor` includes support for the [.psu format](https://www.psdevwiki.com/ps2/PSU). This is also the format that `uLaunchELF` and other tools, such as `myMC` are able to read and write individual PS2 saves to.

As an end-user, it is recommended that you focus on `DOR.h`. This is the top-level API for loading and interacting with DOR save files. To open a DOR save file from a .psu file on your disk, use the `DORSave_CreateFromPSUArchive` method.

```c
PSUArchive* pArchive = NULL;
DORSave* pSave = NULL;

// don't forget error checking here.
PSUArchive_CreateFromFile("/path/to/DORSave.psu", &pArchive);
DORSave_CreateFromPSUArchive(pArchive, &pSave);

// dtors:
DORSave_Destroy(pSave);
PSUArchive_Destroy(pArchive);
```

Once you've loaded in the save file, you can utilize the `DORSave_` functions to read/write data to the save. 

If you want to write the save file back out, you can use the following methods to write the data back out to the PSU entry and write the PSU file out:
```c
// find matching entry.
PSUEntryInfo GameEntry;
PSUArchive_FindFile(pArchive, "BASLUS-20515", &GameEntry);

// Write DOR save bytes into PSU entry.
PSUArchive_SetFileData(pArchive, &GameEntry, DORSave_GetBytes(pSave), DORSave_GetSize(pSave));

PSUArchive_WriteToFile(pArchive, "/path/to/DORSave.psu");
```

## Test Application
Bundled in the repository is a simple test application. It can read a save and print information about it including size of game data, checksum, player name, and Deck A contents. 

There's also a command to test changing the player name, using the `--set-name` argument, the player's name will change and be written back out to disk.

### Test Application Usage

```
$ dorsave-read /path/to/DORSave.psu
PSU entries: 8
   0  off=0x00000 payload=0x00200 size=0x00000007 mode=0x00008427 name=BASLUS-20515
   1  off=0x00200 payload=0x00400 size=0x00000000 mode=0x00008427 name=.
   2  off=0x00400 payload=0x00600 size=0x00000000 mode=0x00008427 name=..
   3  off=0x00600 payload=0x00800 size=0x000003c4 mode=0x00008417 name=icon.sys
   4  off=0x00c00 payload=0x00e00 size=0x0000e2a8 mode=0x00008417 name=icon00.ico
   5  off=0x0f200 payload=0x0f400 size=0x0000e698 mode=0x00008417 name=icon02.ico
   6  off=0x1dc00 payload=0x1de00 size=0x0000e2a8 mode=0x00008417 name=icon01.ico
   7  off=0x2c200 payload=0x2c400 size=0x000100f0 mode=0x00008497 name=BASLUS-20515
DOR game data size: 0x100f0
DOR checksum: 0x94c7
Player name: Julian
Player token: B7 72 CF 4D 
Deck leader: 612 - Maiden of the Aqua
Leader XP: 500 (2LT), TotalCopies=1
Stored deck cost: 823
Deck cards:
   0: 005 - Koumori Dragon; Copies: 1
   1: 100 - Aqua Madoor; Copies: 1
   2: 331 - Megirus Light; Copies: 1
   3: 336 - Bio Plant; Copies: 1
   4: 452 - Krokodilus; Copies: 1
   5: 463 - White Dolphin; Copies: 1
   6: 473 - Man-eating Black Shark; Copies: 1
   7: 476 - Kairyu-shin; Copies: 1
   8: 477 - Takriminos; Copies: 2
   9: 477 - Takriminos; Copies: 2
  10: 503 - Mechanical Snail; Copies: 1
  11: 547 - Bolt Penguin; Copies: 1
  12: 550 - Fiend Kraken; Copies: 1
  13: 555 - Akihiron; Copies: 1
  14: 556 - The Melting Red Shadow; Copies: 1
  15: 558 - Turtle Tiger; Copies: 1
  16: 562 - Twin Long Rods #1; Copies: 1
  17: 564 - Hitodenchak; Copies: 1
  18: 568 - The Furious Sea King; Copies: 1
  19: 571 - Psychic Kappa; Copies: 1
  20: 572 - Flying Penguin; Copies: 1
  21: 583 - Giant Red Seasnake; Copies: 1
  22: 586 - Kanikabuto; Copies: 1
  23: 587 - Zarigun; Copies: 2
  24: 587 - Zarigun; Copies: 2
  25: 588 - Sea Kamen; Copies: 2
  26: 588 - Sea Kamen; Copies: 2
  27: 589 - Ameba; Copies: 1
  28: 590 - Yado Karu; Copies: 1
  29: 591 - Turtle Raccoon; Copies: 1
  30: 593 - Star Boy; Copies: 1
  31: 600 - Penguin Soldier; Copies: 1
  32: 601 - Liquid Beast; Copies: 1
  33: 606 - Night Lizard; Copies: 2
  34: 606 - Night Lizard; Copies: 2
  35: 669 - Tentacle Plant; Copies: 1
  36: 718 - The Eye of Truth; Copies: 1
  37: 742 - Aqua Chorus; Copies: 1
  38: 777 - Power of Kaishin; Copies: 1
  39: 804 - Tears of a Mermaid; Copies: 1
```
