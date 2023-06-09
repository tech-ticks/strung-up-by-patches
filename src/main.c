#include <pmdsky.h>
#include <cot.h>
#include "extern.h"
#include "common.h"

#include "dungeon_script.h"
#include "dungeon_npcs.h"

// Whether the player has taken the stairs in the opposite direction
static bool GoneBackToPreviousFloor = false;

static bool IsCurrentDungeonAscending() {
  return DUNGEON_RESTRICTIONS[DUNGEON_PTR->id.val].f_dungeon_goes_up;
}

static void ReturnToPreviousFloor() {
  // There are two different sound effects based on whether you go up or down the stairs
  // (inverted here)
  int se_index = IsCurrentDungeonAscending() ? 4873 : 4874;
  PlaySe(se_index, 0x100);

  GoneBackToPreviousFloor = true;
  DUNGEON_PTR->floor -= 2;
  if (DUNGEON_PTR->floor < 0) {
    DUNGEON_PTR->floor = 0;
  }
  DUNGEON_PTR->end_floor_flag = true;
}

static char* FallbackStairsMenuEntryFunc(char* string_buffer, int option_num) {
  switch (option_num) {
    case 0:
      return "Go up to the previous floor";
    case 1:
      return "Go down to the next floor";
    case 2:
      return "Cancel";
    default:
      return "";
  }
}

// If the stairs in the opposite direction could not be spawned,
// show a menu to select whether to go up or down.
bool __attribute__((used)) CustomRunLeaderTurn(undefined param_1) {
  if (DUNGEON_PTR->stepped_on_stairs && LoadScriptVariableValue(NULL, VAR_PP_ZERO_TRAP_SPAWN_FAILED)) {
    struct advanced_menu_flags flags = {
      .a_accept = true,
      .b_cancel = true
    };
    int flags_int = *(int*) &flags;

    PrintDialogue(NULL, NULL, "You've found two staircases leading\nup and down!", true);
    
    HideMinimap();
    AdvanceFrame(0);
    AdvanceFrame(0);

    int menu_id = CreateAdvancedMenu(NULL, flags_int, NULL, (undefined*) FallbackStairsMenuEntryFunc, 3, 3);
    while (IsAdvancedMenuActive(menu_id)) {
      AdvanceFrame(0);
    }
    int choice = GetAdvancedMenuResult(menu_id);
    FreeAdvancedMenu(menu_id);

    switch (choice) {
      case 0:
        // Go up to the previous floor
        ReturnToPreviousFloor();
        break;
      case 1:
        // Go down to the next floor
        int se_index = IsCurrentDungeonAscending() ? 4874 : 4873;
        PlaySe(se_index, 0x100);
        DUNGEON_PTR->end_floor_flag = true;
        break;
      default:
        // Cancel
        DUNGEON_PTR->stepped_on_stairs = false;
        break;
    }
  }
  return RunLeaderTurn(param_1);
}

static void RunMonsterAiYoullAlwaysFindMe(struct entity* entity, struct monster* monster, undefined param_2) {
  struct entity* leader = GetLeader();
  if (!EntityIsValid(leader)) {
    return;
  }

  if (GetChebyshevDistance(&entity->pos, &leader->pos) <= 1) {
    // Use the normal AI if the player is adjacent to Amber
    return RunMonsterAi(entity, param_2);
  }

  // TODO: RunMonsterAi also has code that handles the Frozen status, which is omitted here
  if (HasStatusThatPreventsActing(entity)) {
    return;
  }

  FUN_022fb538(entity); // unknown
  
  ClearMonsterActionFields(&monster->action);
  // Always move towards the player
  enum direction_id dir = GetDirectionTowardsPosition(&entity->pos, &leader->pos);
  if (dir != DIR_NONE) {
    monster->action.direction.val = dir;
  }
  SetActionPassTurnOrWalk(&monster->action, monster->id.val);
  monster->target_pos.x = leader->pos.x;
  monster->target_pos.y = leader->pos.y;
}

static bool TryRockSmashTile(struct monster* monster, int move_index, int x, int y, enum direction_id dir) {
  struct tile* tile = GetTile(x, y);
  if (tile != NULL && !tile->f_unbreakable && tile->terrain_type == TERRAIN_WALL) {
    ClearMonsterActionFields(&monster->action);
    SetActionUseMoveAi(&monster->action, move_index, dir);
    return true;
  }
  return false;
}

static void RunMonsterAiRockSmash(struct entity* entity, struct monster* monster, undefined param_2, int move_index) {
  if (TryRockSmashTile(monster, move_index, entity->pos.x + 1, entity->pos.y, DIR_RIGHT)) {
    return;
  }
  if (TryRockSmashTile(monster, move_index, entity->pos.x - 1, entity->pos.y, DIR_LEFT)) {
    return;
  }
  if (TryRockSmashTile(monster, move_index, entity->pos.x, entity->pos.y - 1, DIR_UP)) {
    return;
  }
  if (TryRockSmashTile(monster, move_index, entity->pos.x, entity->pos.y + 1, DIR_DOWN)) {
    return;
  }

  return RunMonsterAi(entity, param_2);
}

void __attribute__((used)) CustomRunMonsterAi(struct entity* entity, undefined param_2) {  
  if (!IsMonster(entity)) {
    return RunMonsterAi(entity, param_2);
  }
  
  struct monster* monster = entity->info;

  if (monster->statuses.monster_behavior.val == BEHAVIOR_SECRET_BAZAAR_KIRLIA) {
    // For some reason, NPCs with the Kirlia behavior start moving back and forth after
    // their position has changed. This is a workaround to prevent that.
    ClearMonsterActionFields(&monster->action);
    return;
  }

  if (monster->tactic.val == TACTIC_GROUP_SAFETY) { // You'll always find me
    return RunMonsterAiYoullAlwaysFindMe(entity, monster, param_2);
  }

  if (monster->is_not_team_member && !monster->ai_targeting_enemy) {
    for (int i = 0; i < 4; i++) {
      if (monster->moves[i].id.val == MOVE_ROCK_SMASH) {
        if (DungeonRandInt(2) == 0) {
          return RunMonsterAiRockSmash(entity, monster, param_2, i);
        }
      }
    }
  }

  return RunMonsterAi(entity, param_2);
}

static bool ShouldUseLayoutWithoutHallways() {
  return DUNGEON_PTR->id.val == DUNGEON_DESTINY_TOWER && DUNGEON_PTR->floor == 99;
}

void __attribute__((naked)) TrampolineCallOriginalCreateHallway(int x0, int y0, int x1, int y1, bool vertical, int x_mid, int y_mid) {
  asm("stmdb sp!,{r3,r4,r5,r6,r7,r8,r9,r10,r11,lr}"); // Replaced instruction
  asm("b CreateHallway+4");
}

void __attribute__((used)) HookCreateHallway(int x0, int y0, int x1, int y1, bool vertical, int x_mid, int y_mid) {
  if (ShouldUseLayoutWithoutHallways()) {
    return;
  }

  TrampolineCallOriginalCreateHallway(x0, y0, x1, y1, vertical, x_mid, y_mid);
}

bool __attribute__((naked)) TrampolineCallOriginalStairsAlwaysReachable(int x_stairs, int y_stairs, bool mark_unreachable) {
  asm("stmdb sp!,{r4,r5,r6,r7,r8,r9,r10,lr}"); // Replaced instruction
  asm("b StairsAlwaysReachable+4");
}

bool __attribute__((used)) HookStairsAlwaysReachable(int x_stairs, int y_stairs, bool mark_unreachable) {
  if (ShouldUseLayoutWithoutHallways()) {
    // Disable the check for whether the stairs are reachable to avoid the one room monster house
    return true;
  }

  return TrampolineCallOriginalStairsAlwaysReachable(x_stairs, y_stairs, mark_unreachable);
}

static bool TryAdjustSpawnPosition(int x_delta, int y_delta) {
  int x = DUNGEON_PTR->gen_info.team_spawn_pos.x + x_delta;
  int y = DUNGEON_PTR->gen_info.team_spawn_pos.y + y_delta;
  struct tile* tile = GetTile(x, y);
  if (!tile) {
    return false;
  }
  if (tile->terrain_type != TERRAIN_NORMAL || tile->monster != NULL) {
    return false;
  }
  
  DUNGEON_PTR->gen_info.team_spawn_pos.x = x;
  DUNGEON_PTR->gen_info.team_spawn_pos.y = y;
  return true;
}

// Try to spawn the descending stairs (very hard)
static bool TrySpawnDescendingStairs(struct position pos) {
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }

  // This might fail, try it in several positions

  pos.x += 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.x -= 1;

  pos.x -= 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.x += 1;

  pos.y += 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.y -= 1;

  pos.y -= 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.y += 1;

  pos.x += 1;
  pos.y += 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.x -= 1;
  pos.y -= 1;

  pos.x -= 1;
  pos.y += 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.x += 1;
  pos.y -= 1;

  pos.x += 1;
  pos.y -= 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.x -= 1;
  pos.y += 1;

  pos.x -= 1;
  pos.y -= 1;
  if (TrySpawnTrap(&pos, TRAP_PP_ZERO_TRAP, 0, 1)) {
    return true;
  }
  pos.x += 1;
  pos.y += 1;

  return false;
}

void __attribute__((used)) CustomSpawnTeam(undefined param_1) {
  SaveScriptVariableValue(NULL, VAR_PP_ZERO_TRAP_SPAWN_FAILED, 0); // Reset "failed to spawn stairs" flag

  struct position trap_spawn_pos = {
    .x = DUNGEON_PTR->gen_info.team_spawn_pos.x,
    .y = DUNGEON_PTR->gen_info.team_spawn_pos.y,
  };

  bool is_descending_dungeon = !IsCurrentDungeonAscending();

  if (GoneBackToPreviousFloor) {
    // If we've descended a floor, spawn the team at the stairs
    DUNGEON_PTR->gen_info.team_spawn_pos.x = DUNGEON_PTR->gen_info.stairs_pos.x;
    DUNGEON_PTR->gen_info.team_spawn_pos.y = DUNGEON_PTR->gen_info.stairs_pos.y;
  } else if (is_descending_dungeon) {
    // Try avoiding spawning the team on the stairs
    bool on_top_of_stairs = DUNGEON_PTR->gen_info.team_spawn_pos.x == DUNGEON_PTR->gen_info.stairs_pos.x
      && DUNGEON_PTR->gen_info.team_spawn_pos.y == DUNGEON_PTR->gen_info.stairs_pos.y;
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(0, -1);
    }
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(-1, 0);
    }
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(0, 1);
    }
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(1, 0);
    }
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(1, 1);
    }
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(-1, -1);
    }
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(1, -1);
    }
    if (on_top_of_stairs) {
      on_top_of_stairs = !TryAdjustSpawnPosition(-1, 1);
    }

    trap_spawn_pos.x = DUNGEON_PTR->gen_info.team_spawn_pos.x;
    trap_spawn_pos.y = DUNGEON_PTR->gen_info.team_spawn_pos.y;
  }

  SpawnTeam(param_1);
  if (DUNGEON_PTR->floor > 1 && is_descending_dungeon) {
    bool spawned = TrySpawnDescendingStairs(trap_spawn_pos);

    if (!spawned) {
      COT_LOG(COT_LOG_CAT_DEFAULT, "Failed to spawn stairs in the opposite direction");
      SaveScriptVariableValue(NULL, VAR_PP_ZERO_TRAP_SPAWN_FAILED, 1);
    }
  }

  GoneBackToPreviousFloor = false;
  SaveScriptVariableValue(NULL, VAR_HIGHEST_DUNGEON_FLOOR, DUNGEON_PTR->floor);
}

void __attribute__((used)) CustomApplyPpZeroTrapEffect(struct entity* attacker, struct entity* defender) {
  if (!IsMonster(defender)) {
    return;
  }

  struct monster* monster = defender->info;
  if (monster->is_team_leader) {
    if (YesNoMenu(0, 17902, 0, 0) == 1) { // "Return to the previous floor?"
      ReturnToPreviousFloor();
    } else {
      ShowMinimap();
    }
  }
}

uint16_t __attribute__((used)) CustomGetTrapAnimation(enum trap_id trap_id) {
  return trap_id == TRAP_PP_ZERO_TRAP ? 0 : GetTrapAnimation(trap_id);
}

static int MinimapTrapTileX = -1;
static int MinimapTrapTileY = -1;

void __attribute__((naked)) TrampolineCallOriginalDrawMinimapTile(int x, int y) {
  asm("stmdb sp!,{r3,r4,r5,r6,r7,r8,r9,r10,r11,lr}"); // Replaced instruction
  asm("b DrawMinimapTile+4");
}

// Replaces the call to `GetTile` in `CustomMinimapGetTile`.
// Pretends that the PP Zero Trap is the hidden stairs.
void __attribute__((used)) CustomDrawMinimapTile(int x, int y) {
  MinimapTrapTileX = -1;
  MinimapTrapTileY = -1;

  struct tile* tile = GetTile(x, y);
  struct entity* entity = tile->object;
  if (EntityIsValid(entity) && entity->type == ENTITY_TRAP) {
    struct trap* trap = entity->info;
    if (trap->id.val == TRAP_PP_ZERO_TRAP) {
      entity->type = ENTITY_HIDDEN_STAIRS;
      MinimapTrapTileX = x;
      MinimapTrapTileY = y;
    }
  }
  TrampolineCallOriginalDrawMinimapTile(x, y);
}

// Turns the PP Zero Trap back into a trap after the minimap is drawn.
void __attribute__((used)) RestorePPZeroTrapTile() {
  if (MinimapTrapTileX == -1 || MinimapTrapTileY == -1) {
    return;
  }

  struct tile* tile = GetTile(MinimapTrapTileX, MinimapTrapTileY);
  struct entity* entity = tile->object;
  if (EntityIsValid(entity)) {
    entity->type = ENTITY_TRAP;
  }
}

void __attribute__((naked)) TrampolineDrawMinimapTileReturn() {
  asm("ldmia sp!,{r3,r4,r5,r6,r7,r8,r9,r10,r11,lr}"); // Replaced instruction (loads into `lr` instead of `pc`)
  asm("b RestorePPZeroTrapTile");
}

// Special handling for the customized PP Zero trap ("downwards stairs")
// Bypasses the sound effect played when triggering the trap and possible IQ skill effects.
void __attribute__((used)) CustomTryTriggerTrap(struct entity* entity, struct position* pos, undefined param_3, undefined param_4) {
  struct tile* tile = GetTileSafe(pos->x, pos->y);
  struct entity* entity_at_pos = tile->object;
  if (!EntityIsValid(entity_at_pos) || entity_at_pos->type != ENTITY_TRAP) {
    return;
  }

  struct trap* trap = entity_at_pos->info;
  if (trap->id.val != TRAP_PP_ZERO_TRAP || !IsMonster(entity)) {
    // Call the original function for other traps
    TryTriggerTrap(entity, pos, param_3, param_4);
    return;
  }

  struct monster* monster = entity->info;
  if (!monster->is_team_leader) {
    // Only the team leader can trigger the fake stairs
    return;
  }

  CustomApplyPpZeroTrapEffect(entity, entity);
}

bool __attribute__((used)) CustomApplyTrapEffect(struct trap* trap, struct entity* user, struct entity* target,
                                          struct tile* tile, struct position* pos, enum trap_id trap_id, bool random_trap) {
  bool result = ApplyTrapEffect(trap, user, target, tile, pos, trap_id, random_trap);

  if (IsMonster(target)) {
    struct monster* monster = target->info;

    struct dungeon_npc_entry entry;
    if (FindDungeonNpcEntry(&entry, monster->id.val)) {
      if (entry.npc_type == NPC_TYPE_PUSH_TO_TRAP && trap_id == entry.parameter1) {
        monster->statuses.monster_behavior.val = BEHAVIOR_SECRET_BAZAAR_KIRLIA;
        monster->joined_at.val = 0; // Disable pushing again

        struct entity* leader = GetLeader();
        if (EntityIsValid(leader)) {
          // Ensure that the NPC is facing the leader
          enum direction_id dir = GetDirectionTowardsPosition(&target->pos, &leader->pos);
          target->graphical_direction.val = dir;
          target->graphical_direction_mirror0.val = dir;
          target->graphical_direction_mirror1.val = dir;
        }

        char script_name_buffer[0x10];
        Snprintf(script_name_buffer, 0x10, "npc_%03d_clear", entry.script_id);
        RunDungeonScript(script_name_buffer, monster);
      }
    }
  }

  return result;
}

// Record the highest floor and adjust item density accordingly before spawning items
void __attribute__((used)) CustomMarkNonEnemySpawns(struct floor_properties* floor_props, bool empty_monster_house) {
  int highest_floor = LoadScriptVariableValue(NULL, VAR_HIGHEST_DUNGEON_FLOOR);
  if (floor_props->floor_number < highest_floor) {
    floor_props->item_density = 0;
  }
  MarkNonEnemySpawns(floor_props, empty_monster_house);
}

// Reset global variables before calling the actual `RunDungeon` function.
void __attribute__((used)) CustomRunDungeon(struct dungeon_init* dungeon_init_data, struct dungeon* dungeon) {
  GoneBackToPreviousFloor = false;

  LoadDungeonNpcs(dungeon_init_data->id.val);
  RunDungeon(dungeon_init_data, dungeon);
}

// Hide the UI after the player has descended to a previous floor
// to avoid the wrong floor number being displayed.
void __attribute__((used)) CustomDisplayUi() {
  if (!GoneBackToPreviousFloor) {
    DisplayUi();
  }
}

void __attribute__((used)) CustomTalkBazaarPokemon(undefined4 unknown, struct entity* entity) {
  if (!IsMonster(entity)) {
    return TalkBazaarPokemon(unknown, entity);
  }
  struct monster* monster = entity->info;
  if (monster->statuses.monster_behavior.val != BEHAVIOR_SECRET_BAZAAR_KIRLIA) {
    return TalkBazaarPokemon(unknown, entity);
  }

  struct dungeon_npc_entry entry;
  if (!FindDungeonNpcEntry(&entry, monster->id.val)) {
    return TalkBazaarPokemon(unknown, entity);
  }

  char script_name_buffer[0x10];
  Snprintf(script_name_buffer, 0x10, "npc_%03d", entry.script_id);

  int result = RunDungeonScript(script_name_buffer, monster);
  switch (entry.npc_type) {
    case NPC_TYPE_NORMAL:
      break;
    case NPC_TYPE_PUSH_TO_TRAP:
      if (result == 1) {
        monster->joined_at.val = DUNGEON_CLIENT; // Enable pushing (why)
        monster->is_ally = true;
        monster->statuses.monster_behavior.val = BEHAVIOR_ALLY;
        monster->tactic.val = TACTIC_WAIT_THERE;

        // Stupid hack: ensure that the NPC is actually affected by the trap
        for (int i = 0; i < 64; i++) {
          struct trap* trap = &DUNGEON_PTR->traps[i];
          if (trap->id.val == entry.parameter1) {
            trap->team = 1;
          }
        }
      }
      break;
  }
}

bool __attribute__((used)) ShouldConvertWallsToChasms() {
  return DUNGEON_PTR->id.val == DUNGEON_BEACH_CAVE;
}
