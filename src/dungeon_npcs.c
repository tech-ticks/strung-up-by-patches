#include "dungeon_npcs.h"

static int DUNGEON_NPC_COUNT = 0;
struct dungeon_npc_entry DUNGEON_NPCS[MAX_DUNGEON_NPCS];

// Load dungeon NPC data from `SKETCHES/dunnpcs.bin`
void LoadDungeonNpcs() {
  MemZero(DUNGEON_NPCS, sizeof(DUNGEON_NPCS));

  struct file_stream file;
  DataTransferInit();
  FileInit(&file);
  FileOpen(&file, "SKETCHES/dunnpcs.bin");

  FileRead(&file, &DUNGEON_NPC_COUNT, sizeof(DUNGEON_NPC_COUNT));
  if (DUNGEON_NPC_COUNT > MAX_DUNGEON_NPCS) {
    DUNGEON_NPC_COUNT = MAX_DUNGEON_NPCS;
  }
  FileRead(&file, &DUNGEON_NPCS, sizeof(struct dungeon_npc_entry) * DUNGEON_NPC_COUNT);
  FileClose(&file);
  DataTransferStop();
}

// Get the dungeon NPC entry for the given monster ID on the current floor
struct dungeon_npc_entry* FindDungeonNpcEntry(uint16_t monster_id) {
  for (int i = 0; i < DUNGEON_NPC_COUNT; i++) {
    char buffer[0x100];
    Snprintf(buffer, sizeof(buffer), "DUNGEON_NPCS[%d].monster_id = %d, DUNGEON_NPCS[%d].floor = %d", i, DUNGEON_NPCS[i].monster_id, i, DUNGEON_NPCS[i].floor);
    DebugPrint(0, buffer);
    if (FemaleToMaleForm(DUNGEON_NPCS[i].monster_id) == FemaleToMaleForm(monster_id) && DUNGEON_NPCS[i].dungeon_id == DUNGEON_PTR->id.val && DUNGEON_NPCS[i].floor == DUNGEON_PTR->floor) {
      return &DUNGEON_NPCS[i];
    }
  }
  return NULL;
}
