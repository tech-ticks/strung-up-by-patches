#include "dungeon_npcs.h"

#define NPC_TYPE_NORMAL 0
#define NPC_TYPE_PUSH_TO_TRAP 1

struct dungeon_npc_entry DUNGEON_NPCS[32];

// Load dungeon NPC data for the given dungeon from `SKETCHES/dunnpcs.bin`
void LoadDungeonNpcs(enum dungeon_id dungeon_id) {
  MemZero(DUNGEON_NPCS, sizeof(DUNGEON_NPCS));

  struct file_stream file;
  DataTransferInit();
  FileInit(&file);
  FileOpen(&file, "SKETCHES/dunnpcs.bin");

  int npc_index_in_dungeon = 0;

  uint32_t entry_count;
  FileRead(&file, &entry_count, sizeof(entry_count));

  struct dungeon_npc_entry entry;
  
  for (uint32_t i = 0; i < entry_count; i++) {
    FileRead(&file, &entry, sizeof(entry));
    if (entry.dungeon_id == dungeon_id) {
      DUNGEON_NPCS[npc_index_in_dungeon] = entry;
      npc_index_in_dungeon++;

      if (npc_index_in_dungeon >= ARRAY_COUNT(DUNGEON_NPCS)) {
        break;
      }
    }
  }

  FileClose(&file);
  DataTransferStop();
}

// Get the dungeon NPC entry for the given monster ID on the current floor
bool FindDungeonNpcEntry(struct dungeon_npc_entry* entry, uint16_t monster_id) {
  for (int i = 0; i < ARRAY_COUNT(DUNGEON_NPCS); i++) {
    if (DUNGEON_NPCS[i].monster_id == monster_id && DUNGEON_NPCS[i].floor == DUNGEON_PTR->floor) {
      *entry = DUNGEON_NPCS[i];
      return true;
    }
  }
  return false;
}