#include "dungeon_npcs.h"

struct dungeon_npc_entry DUNGEON_NPCS[MAX_DUNGEON_NPCS];

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
struct dungeon_npc_entry* FindDungeonNpcEntry(uint16_t monster_id) {
  for (int i = 0; i < ARRAY_COUNT(DUNGEON_NPCS); i++) {
    char buffer[0x100];
    Snprintf(buffer, sizeof(buffer), "DUNGEON_NPCS[%d].monster_id = %d, DUNGEON_NPCS[%d].floor = %d", i, DUNGEON_NPCS[i].monster_id, i, DUNGEON_NPCS[i].floor);
    DebugPrint(0, buffer);
    if (FemaleToMaleForm(DUNGEON_NPCS[i].monster_id) == FemaleToMaleForm(monster_id) && DUNGEON_NPCS[i].floor == DUNGEON_PTR->floor) {
      return &DUNGEON_NPCS[i];
    }
  }
  return NULL;
}
