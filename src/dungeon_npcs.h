#pragma once

#include <pmdsky.h>
#include <cot.h>

#include "common.h"

#define NPC_TYPE_NORMAL 0
#define NPC_TYPE_PUSH_TO_TRAP 1
#define NPC_TYPE_PUSH_TO_POKEMON 2
#define NPC_TYPE_SHOW_MOVE 3
#define NPC_TYPE_ALWAYS_PUSHABLE 4

struct dungeon_npc_entry {
  uint16_t monster_id;
  uint16_t script_id;
  uint8_t dungeon_id;
  uint8_t floor;
  uint8_t npc_type;
  uint8_t parameter1;
  uint16_t parameter2;
};

#define MAX_DUNGEON_NPCS 32
extern struct dungeon_npc_entry DUNGEON_NPCS[MAX_DUNGEON_NPCS];

// Load dungeon NPC data for the given dungeon from `SKETCHES/dunnpcs.bin`
void LoadDungeonNpcs();

// Get the dungeon NPC entry for the given monster ID on the current floor
struct dungeon_npc_entry* FindDungeonNpcEntry(uint16_t monster_id);