#pragma once

#include <pmdsky.h>
#include <cot.h>

#include "common.h"

#define NPC_TYPE_NORMAL 0
#define NPC_TYPE_PUSH_TO_TRAP 1

struct dungeon_npc_entry {
  uint16_t monster_id;
  uint16_t script_id;
  uint8_t dungeon_id;
  uint8_t floor;
  uint8_t npc_type;
  uint8_t parameter1;
  uint16_t parameter2;
};

extern struct dungeon_npc_entry DUNGEON_NPCS[32];

// Load dungeon NPC data for the given dungeon from `SKETCHES/dunnpcs.bin`
void LoadDungeonNpcs(enum dungeon_id dungeon_id);

// Get the dungeon NPC entry for the given monster ID on the current floor
bool FindDungeonNpcEntry(struct dungeon_npc_entry* entry, uint16_t monster_id);