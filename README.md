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
   7  off=0x2c200 payload=0x2c400 size=0x000100f0 mode=0x00008417 name=BASLUS-20515
DOR game data size: 0x100f0
DOR checksum: 0x0413
Player name: Atem
Deck leader: 78 - Magician of Black Chaos
Leader XP: 4230 (MAJ), marker=0x67620525
Unknown after leader: 792
Deck cards:
   0: 478 - Aqua Dragon
   1: 674 - Slate Warrior
   2:  53 - The Unhappy Maiden
   3: 704 - Red Medicine
   4:  87 - Dark Magician Girl
   5:  81 - Witch's Apprentice
   6: 191 - Swordsman from a Foreign Land
   7: 614 - Wings of Wicked Flame
   8:  26 - Petit Dragon
   9:  36 - Time Wizard
  10:  43 - Magician of Faith
  11: 661 - Griggle
  12: 853 - Dark Magic Ritual
  13: 297 - Kuriboh
  14: 394 - Dancing Elf
  15: 734 - Magical Neutralizing Force
  16: 694 - Yami
  17: 762 - Black Pendant
  18:  95 - Injection Fairy Lily
  19:  60 - Dark Magician
  20: 685 - Monster Reborn
  21: 377 - Hourglass Of Life
  22: 699 - Dark Hole
  23: 801 - Spellbinding Circle
  24: 705 - Goblin's Secret Remedy
  25: 712 - Ookazi
  26: 716 - Dark-Piercing Light
  27: 625 - Rock Ogre Grotto #1
  28: 146 - Swordstalker
  29: 754 - Dark Energy
  30: 498 - Cannon Soldier
  31: 795 - Paralyzing Potion
  32: 796 - Cursebreaker
  33: 417 - Man-Eater Bug
  34: 803 - Mesmeric Control
  35: 804 - Tears of a Mermaid
  36: 780 - Megamorph
  37: 823 - Magic Jammer
  38: 824 - White Hole
  39: 828 - Negate Attack
```
