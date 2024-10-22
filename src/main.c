#include <pmdsky.h>
#include <cot.h>
#include "extern.h"
#include "common.h"

#include "dungeon_script.h"
#include "dungeon_npcs.h"

// Whether the player has taken the stairs in the opposite direction
static bool GoneBackToPreviousFloor = false;
static bool FreezePlayer = false; // Prevent moving in the dungeon

static bool IsCurrentDungeonAscending() {
  return DUNGEON_RESTRICTIONS[DUNGEON_PTR->id.val].f_dungeon_goes_up;
}

static bool ShouldEnableStairsInOtherDirection() {
  return !IsCurrentDungeonAscending() && DUNGEON_PTR->id.val != DUNGEON_BEACH_CAVE_PIT /* Daunting Doldrums */
    && !(DUNGEON_PTR->id.val == DUNGEON_DRENCHED_BLUFF /* Polyphonic Playground */ && DUNGEON_PTR->floor == 8);
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
    default:
      return "Cancel";
  }
}

// Helper for making NPCs stationary and executing the script `npc_[num]_clear.dnscrpt`
// when a quest objective is met
static void ClearNPCQuest(struct dungeon_npc_entry* npc, struct entity* npc_entity, bool disable_pushing) {
  struct monster* monster = npc_entity->info;
  monster->statuses.monster_behavior.val = BEHAVIOR_SECRET_BAZAAR_KIRLIA;
  if (disable_pushing) {
    monster->joined_at.val = 0; // Disable pushing again
  }
  npc->npc_type = NPC_TYPE_NORMAL;

  struct entity* leader = GetLeader();
  if (EntityIsValid(leader)) {
    // Ensure that the NPC is facing the leader
    enum direction_id dir = GetDirectionTowardsPosition(&npc_entity->pos, &leader->pos);
    npc_entity->graphical_direction.val = dir;
    npc_entity->graphical_direction_mirror0.val = dir;
    npc_entity->graphical_direction_mirror1.val = dir;
  }

  // If the camera's too far away, move it closer
  if (GetChebyshevDistance(&DUNGEON_PTR->display_data.camera_pos, &npc_entity->pos) > 2) {
    PointCameraToMonster(npc_entity, 1);
  }
  
  char script_name_buffer[0x10];
  Snprintf(script_name_buffer, 0x10, "npc_%03d_clear", npc->script_id);
  RunDungeonScript(script_name_buffer, npc_entity);
}

// Check if NPC quests with the `NPC_TYPE_SHOW_MOVE` type where completed
static void CheckUsedMoveForNpc(void) {
  int move_id = -1;
  struct entity* npc_entity = NULL;
  struct dungeon_npc_entry* npc = NULL;
  for (int i = 0; i < 20; i++) {
    struct entity* entity = DUNGEON_PTR->entity_table.header.active_monster_ptrs[i];
    if (!EntityIsValid(entity)) {
      continue;
    }

    struct monster* monster = entity->info;
    npc = FindDungeonNpcEntry(monster->id.val);
    if (npc != NULL && npc->npc_type == NPC_TYPE_SHOW_MOVE && HasMonsterBeenAttackedInDungeons(monster->id.val)) {
      npc_entity = entity;
      move_id = npc->parameter2;
    }
  }

  if (move_id == -1 || npc_entity == NULL) {
    return;
  }

  struct entity* leader = GetLeader();
  if (EntityIsValid(leader)) {
    struct monster* monster = leader->info;
    for (int i = 0; i < 4; i++) {
      struct move* move = &monster->moves[i];
      if (move->id.val == move_id && move->f_last_used) {
        ClearNPCQuest(npc, npc_entity, true);
        break;
      }
    }
  }
}

static void WarpAbra(struct entity* entity, struct monster* monster, struct entity* litwick) {
  WaitFrames(30);
  EndNegativeStatusCondition(entity, entity, false, false, true);
  LogMessage(entity, "[CS:G]Abra[CR] used [CS:K]Teleport[CR]!", true);
  WaitFrames(30);
  TryWarp(entity, entity, WARP_RANDOM, NULL);

  enum direction_id dir = GetDirectionTowardsPosition(&litwick->pos, &entity->pos);
  litwick->graphical_direction.val = dir;
  litwick->graphical_direction_mirror0.val = dir;
  litwick->graphical_direction_mirror1.val = dir;

  struct monster* litwick_monster = litwick->info;
  PlayEffectAnimationEntitySimple(litwick, 91);
  struct portrait_box portrait_box;
  InitPortraitData(&portrait_box, litwick_monster->id.val, 12);
  PrintDialogue(NULL, &portrait_box, "[CS:G]Abra[CR],[W:10] nooooooooo[VS:2:0]ooooooooooooo![VR][W:10]\nWhere did you go?[W:15]\n[FACE:7]You can't just leave me like this!", true);
  
  // Turn Abra back into an enemy
  if (!monster->is_team_leader) { // but not Smeargle transformed into Abra
    monster->statuses.monster_behavior.val = BEHAVIOR_NORMAL_ENEMY_0x0;
    monster->joined_at.val = 0;
    monster->is_ally = false;
  }
}

static void CheckWantedPokemonAdjacent(struct entity* entity, struct monster* monster) {
  // Ensure that the leader is somewhat close so that quests aren't completed by chance off-screen
  struct entity* leader = GetLeader();
  if (!EntityIsValid(leader) || GetChebyshevDistance(&entity->pos, &leader->pos) > 6) {
    return;
  }

  struct dungeon_npc_entry* entry = FindDungeonNpcEntry(monster->id.val);
  if (entry != NULL && entry->npc_type == NPC_TYPE_PUSH_TO_POKEMON) {
    for (int i = 0; i < 20; i++) {
      struct entity* other_entity = DUNGEON_PTR->entity_table.header.active_monster_ptrs[i];
      if (!EntityIsValid(other_entity)) {
        continue;
      }

      struct monster* other_monster = other_entity->info;

      // The monster ID might be changed to the female form
      int other_monster_id = FemaleToMaleForm(other_monster->apparent_id.val);
      if (AreEntitiesAdjacent(entity, other_entity)
          && other_monster_id == entry->parameter2
          && HasMonsterBeenAttackedInDungeons(other_monster_id)) {
        ClearNPCQuest(entry, entity, true);
        if (monster->is_not_team_member) {
          if (!other_monster->is_team_leader) {
            other_monster->statuses.monster_behavior.val = BEHAVIOR_SECRET_BAZAAR_KIRLIA;
            other_monster->joined_at.val = DUNGEON_JOINED_AT_BIDOOF;
            other_monster->is_ally = true;
          }

          // Abra go warp
          if (FemaleToMaleForm(other_monster_id) == MONSTER_ABRA) {
            WarpAbra(other_entity, other_monster, entity);
            return;
          }
        }
      }
    }
  }
}

static void CheckPushToPokemonQuestCompletion(void) {
  for (int i = 0; i < 20; i++) {
    struct entity* entity = DUNGEON_PTR->entity_table.header.active_monster_ptrs[i];
    if (!EntityIsValid(entity)) {
      continue;
    }

    struct monster* monster = entity->info;
      if (monster->statuses.monster_behavior.val == BEHAVIOR_ALLY) {
      // If the NPC wants to be pushed to a Pokémon, check if they're adjacent
      // to a monster with the correct ID
      CheckWantedPokemonAdjacent(entity, monster);
    }
  }
}

// If the stairs in the opposite direction could not be spawned,
// show a menu to select whether to go up or down.
bool __attribute__((used)) CustomRunLeaderTurn(bool is_first_loop) {
  if (FreezePlayer) {
    return false;
  }

  // Set all dungeon NPCs to the "Bazaar Host" behavior.
  // Allows assigning stats in SkyTemple by using another behavior (not possible by default)
  if (is_first_loop) {
    for (int i = 0; i < 20; i++) {
      struct entity* entity = DUNGEON_PTR->entity_table.header.active_monster_ptrs[i];
      if (!EntityIsValid(entity)) {
        continue;
      }

      struct monster* monster = entity->info;
      struct dungeon_npc_entry* entry = FindDungeonNpcEntry(monster->id.val);
      if (entry != NULL) {
        monster->statuses.monster_behavior.val = BEHAVIOR_SECRET_BAZAAR_KIRLIA;

        if (entry->npc_type == NPC_TYPE_ALWAYS_PUSHABLE) {
          monster->joined_at.val = DUNGEON_JOINED_AT_BIDOOF;
          monster->is_ally = true;
          monster->statuses.monster_behavior.val = BEHAVIOR_ALLY;
        }
      }

      if (DUNGEON_PTR->id.val == DUNGEON_MT_BRISTLE_PEAK /* Little Dream */ && FemaleToMaleForm(monster->id.val) == MONSTER_HITMONCHAN) {
        // Ensure that Hitmonchan in Little Dream have Rock Smash
        bool has_rock_smash = false;
        for (int i = 0; i < 4; i++) {
          struct move* move = &monster->moves[i];
          if (move->f_exists && move->id.val == MOVE_ROCK_SMASH) {
            has_rock_smash = true;
            break;
          }
        }

        if (!has_rock_smash) {
          InitMove(&monster->moves[0], MOVE_ROCK_SMASH);
        }
      }

      if (DUNGEON_PTR->id.val == DUNGEON_MT_BRISTLE_PEAK /* Little Dream */ && DUNGEON_PTR->floor == 4) {
        // Splash breaks Little Dream B4F
        for (int i = 0; i < 4; i++) {
          struct move* move = &monster->moves[i];
          if (move->f_exists && move->id.val == MOVE_SPLASH) {
            move->f_sealed = true;
            break;
          }
        }
      }
    }
  }

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

  // Increment the floor count based on player position in the Secret Room
  if (IsSecretRoom()) {
    int initial_floor = 1;
    struct entity* leader = GetLeader();
    if (EntityIsValid(leader)) {
      if (leader->pos.y < 10) {
        DUNGEON_PTR->floor = initial_floor + 6;
      } else if (leader->pos.y < 12) {
        DUNGEON_PTR->floor = initial_floor + 5;
      } else if (leader->pos.y < 14) {
        DUNGEON_PTR->floor = initial_floor + 4;
      } else if (leader->pos.y < 16) {
        DUNGEON_PTR->floor = initial_floor + 3;
      } else if (leader->pos.y < 18) {
        DUNGEON_PTR->floor = initial_floor + 2;
      } else if (leader->pos.y < 20) {
        DUNGEON_PTR->floor = initial_floor + 1;
      } else {
        DUNGEON_PTR->floor = initial_floor;
      }
    }
  }

  bool result = RunLeaderTurn(is_first_loop);
  CheckUsedMoveForNpc();
  CheckPushToPokemonQuestCompletion();
  return result;
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

  CheckPushToPokemonQuestCompletion();

  if (monster->statuses.monster_behavior.val == BEHAVIOR_SECRET_BAZAAR_KIRLIA) {
    // For some reason, NPCs with the Kirlia behavior start moving back and forth after
    // their position has changed. This is a workaround to prevent that.
    ClearMonsterActionFields(&monster->action);

    // Special behavior for Togetic on the first floor of Polyphonic Playground
    if (DUNGEON_PTR->id.val == DUNGEON_DRENCHED_BLUFF && DUNGEON_PTR->floor == 1
        && FemaleToMaleForm(monster->id.val) == MONSTER_TOGETIC) {
      struct entity* leader = GetLeader();
      if (leader->pos.y < 14) {
        if (entity->pos.y == 11) {
          PlayEffectAnimationEntitySimple(entity, 25);
          WaitFrames(30);
          FreezePlayer = true;
        } else if (entity->pos.y == 7) {
          union damage_source source = {
            .other = DAMAGE_SOURCE_WENT_AWAY
          };
          HandleFaint(entity, source, NULL);
          WaitFrames(40);
          FreezePlayer = false;
        }

        monster->action.direction.val = DIR_UP;
        SetActionPassTurnOrWalk(&monster->action, monster->id.val);
        monster->target_pos.x = monster->pos.x;
        monster->target_pos.y = 0;
      }
    }

    return;
  }

  if (monster->tactic.val == TACTIC_GROUP_SAFETY) { // You'll always find me
    return RunMonsterAiYoullAlwaysFindMe(entity, monster, param_2);
  }

  // The targeting flag seems to be always active on the first floor of Little Dream, so bypass it to progress
  if (monster->is_not_team_member && (!monster->ai_targeting_enemy || (DUNGEON_PTR->id.val == DUNGEON_MT_BRISTLE_PEAK && DUNGEON_PTR->floor == 1))) {
    for (int i = 0; i < 4; i++) {
      struct move* move = &monster->moves[i];
      if (move->f_exists) {
        if (move->id.val == MOVE_ROCK_SMASH) {
          if (DungeonRandInt(2) == 0) {
            return RunMonsterAiRockSmash(entity, monster, param_2, i);
          }
        }

        // Don't use Pounce right in front of the player
        if (move->id.val == MOVE_POUNCE) {
          struct entity* leader = GetLeader();
          if (!EntityIsValid(leader)) {
            continue;
          }

          bool adjacent = AreEntitiesAdjacent(entity, leader);
          move->f_enabled_for_ai = !adjacent;
          // enable/disable all other moves lol I'm tired
          for (int j = 0; j < 4; j++) {
            if (j != i) {
              monster->moves[j].f_enabled_for_ai = adjacent;
            }
          }
        }
      }
    }
  }

  return RunMonsterAi(entity, param_2);
}

static bool ShouldUseLayoutWithoutHallways() {
  // Use the layout without hallways on Little Dream B2F
  return DUNGEON_PTR->id.val == DUNGEON_MT_BRISTLE_PEAK && DUNGEON_PTR->floor == 2;
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
  if (ShouldUseLayoutWithoutHallways() || DUNGEON_PTR->floor_properties.fixed_room_id.val != 0) {
    // Disable the check for whether the stairs are reachable to avoid the one room monster house.
    // This is required in layouts without hallways because the check would always fail.
    // It's also bypassed in floors with fixed rooms because the only other way to disable the check
    // is via the "Is Free Layout" flag, which replaces the stairs with Warp Tiles.
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

  bool stairs_in_other_direction = ShouldEnableStairsInOtherDirection();

  if (GoneBackToPreviousFloor ||
      (DUNGEON_PTR->id.val ==  DUNGEON_BEACH_CAVE_PIT /* Daunting Doldrums */ && DUNGEON_PTR->floor == 5)) {
    // If we've descended a floor, spawn the team at the stairs
    // There's also a special case for Daunting Doldrums 5F, where the team is spawned at the stairs
    DUNGEON_PTR->gen_info.team_spawn_pos.x = DUNGEON_PTR->gen_info.stairs_pos.x;
    DUNGEON_PTR->gen_info.team_spawn_pos.y = DUNGEON_PTR->gen_info.stairs_pos.y;
  } else if (stairs_in_other_direction) {
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
  if (DUNGEON_PTR->floor > 1 && stairs_in_other_direction) {
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

    struct dungeon_npc_entry* entry = FindDungeonNpcEntry(monster->id.val);
    if (entry != NULL) {
      if (entry->npc_type == NPC_TYPE_PUSH_TO_TRAP && trap_id == entry->parameter1) {
        ClearNPCQuest(entry, target, true);
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

  if (DUNGEON_PTR->id.val == DUNGEON_DRENCHED_BLUFF /* Polyphonic Playground */ && DUNGEON_PTR->floor == 1 && !IsHiddenStairsFloor()) {
    // Add Hidden Stairs to the first floor of Polyphonic Playground
    uint8_t pos[2] = {
      11,
      7
    };
    SpawnStairs(pos, &DUNGEON_PTR->gen_info, HIDDEN_STAIRS_SECRET_ROOM);
  }
}

// Reset global variables before calling the actual `RunDungeon` function.
void __attribute__((used)) CustomRunDungeon(struct dungeon_init* dungeon_init_data, struct dungeon* dungeon) {
  GoneBackToPreviousFloor = false;

  LoadDungeonNpcs();
  
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

  struct dungeon_npc_entry* entry = FindDungeonNpcEntry(monster->id.val);
  if (entry == NULL) {
    return TalkBazaarPokemon(unknown, entity);
  }

  char script_name_buffer[0x10];
  Snprintf(script_name_buffer, 0x10, "npc_%03d", entry->script_id);

  int result = RunDungeonScript(script_name_buffer, entity);
  switch (entry->npc_type) {
    case NPC_TYPE_NORMAL:
      break;
    case NPC_TYPE_PUSH_TO_TRAP:
      // Stupid hack: ensure that the NPC is actually affected by the trap
      /*for (int i = 0; i < 64; i++) {
        struct trap* trap = &DUNGEON_PTR->traps[i];
        if (trap->id.val == entry->parameter1) {
          trap->team = 1;
        }
      }*/
      [[fallthrough]]
    case NPC_TYPE_PUSH_TO_POKEMON:
      if (result == 1) {
        monster->joined_at.val = DUNGEON_JOINED_AT_BIDOOF; // Enable pushing (why)
        monster->is_ally = true;
        monster->statuses.monster_behavior.val = BEHAVIOR_ALLY;
        monster->tactic.val = TACTIC_WAIT_THERE;
      }
      break;
  }
}

bool __attribute__((used)) CustomCanMonsterUseItem(struct entity* entity, struct item* item) {
  if (item->id.val == ITEM_KEY) {
    struct tile* maybe_key_door = GetTileSafe(entity->pos.x, entity->pos.y - 1);
    if (!maybe_key_door->f_key_door_key_locked) {
      LogMessageByIdWithPopupCheckUser(entity, 2964 /* "It can't be used here!" */);
      return false;
    }
  }
  return true;
}

// Remove custom `joined_at` values at the end of the floor because the game would
// get very funky on subsequent floors
void __attribute__((used)) OnFloorLoopOver(undefined param_1) {
  for (int i = 0; i < 20; i++) {
    struct entity* entity = DUNGEON_PTR->entity_table.header.active_monster_ptrs[i];
    if (!EntityIsValid(entity)) {
      break;
    }
    struct monster* monster = entity->info;
    if (monster->joined_at.val == DUNGEON_JOINED_AT_BIDOOF) {
      monster->joined_at.val = 0;
    }
  }
  FUN_0234b010(param_1); // Original function call
}

struct tile* __attribute__((used)) HookHandleFaint(struct entity* entity) {
  // Don't kick the player out of the dungeon when an NPC faints
  struct monster* monster = entity->info;
  if (monster->joined_at.val == DUNGEON_JOINED_AT_BIDOOF) {
    monster->joined_at.val = 0;
  }
  return GetTileAtEntity(entity); // Original function call
}

void __attribute__((used)) CustomSpawnStairs(uint8_t* pos, struct dungeon_generation_info* gen_info, enum hidden_stairs_type stairs_type) {
  if ((DUNGEON_PTR->floor == 7 && DUNGEON_PTR->id.val == DUNGEON_DRENCHED_BLUFF) || (DUNGEON_PTR->floor == 5 && DUNGEON_PTR->id.val == DUNGEON_MT_BRISTLE_PEAK)) {
    // There's no going down further in the seventh floor of Polyphonic Playground and the fifth floor of Little Dream
    return;
  }
  SpawnStairs(pos, gen_info, stairs_type);
}

struct entity* entity_to_transform_into = NULL;

bool  __attribute__((used)) CustomDoMoveTransform(struct entity* attacker, struct entity* defender, struct move* move, enum item_id item_id) {
if (IsMonster(defender)) {
    struct monster* monster = defender->info;
    if (!HasMonsterBeenAttackedInDungeons(monster->id.val) && FemaleToMaleForm(monster->id.val) != 537 /* Smeargle (different slot) */) {
      LogMessage(attacker, "You can't transform into a [CS:K]Figment[CR]!", true);
      return false;
    }
  }
  // The Pokémon who uses the move is the one transforming, so we need to pass it twice
  // for the correct behavior
  entity_to_transform_into = defender;
  TryTransform(attacker, attacker);
  return true;
}

enum monster_id  __attribute__((used)) GetTransformMonsterId(struct entity* entity) {
  if (entity_to_transform_into != NULL) {
    struct monster* monster = entity_to_transform_into->info;
    return monster->id.val;
  }
  return MONSTER_DECOY;
}

struct entity* __attribute__((naked)) TrampolineCallOriginalSpawnMonster(struct spawned_monster_data* monster_data, bool cannot_be_asleep) {
  asm("stmdb sp!,{r3,r4,r5,r6,r7,lr}"); // Replaced instruction
  asm("b SpawnMonster+4");
}

struct entity* __attribute__((used)) HookSpawnMonster(struct spawned_monster_data* monster_data, bool cannot_be_asleep) {
  if (FemaleToMaleForm(monster_data->monster_id.val) == 542 /* Doll used in Little Dream B5F */) {
    monster_data->behavior.val = BEHAVIOR_ALLY;
  }
  return TrampolineCallOriginalSpawnMonster(monster_data, cannot_be_asleep);
}

int __attribute__((naked)) TrampolineCallOriginalSetupDBoxFormat(struct advanced_menu_layout* layout, undefined unk) {
  asm("stmdb sp!,{r4,lr}"); // Replaced instruction
  asm("b SetupDBoxFormat+4");
}

int __attribute__((used)) HookSetupDBoxFormat(struct advanced_menu_layout* layout, undefined unk) {
  if (LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, 252) != 0 && !layout->top_screen) {
    layout->frame_type = 0xfa; // Transparent
  }
  return TrampolineCallOriginalSetupDBoxFormat(layout, unk);
}

bool __attribute__((used)) CustomDoMovePounce(struct entity* attacker, struct entity* defender, struct move* move,
                  enum item_id item_id) {
  TryPounce(attacker, attacker, DIR_CURRENT); // The user must be the one pouncing
  if (!DealDamage(attacker, defender, move, 0x100, item_id)) {
    return false;
  }

  LowerSpeed(attacker, defender, 1, true);
  return true;
}

void __attribute__((used)) CustomWarpTrap(struct entity* user, struct entity* target, enum warp_type warp_type,
             struct position* position) {
  if (DUNGEON_PTR->id.val == DUNGEON_MT_BRISTLE_PEAK /* Little Dream */ && DUNGEON_PTR->floor == 4) {
    // Warp to a specific position
    struct position pos = { .x = 8, .y = 9 };
    TryWarp(user, target, WARP_POSITION_EXACT, &pos);
  } else {
    TryWarp(user, target, warp_type, position);
  }
}

int __attribute__((used)) ShowQuicksaveMessage(undefined param_1, int message_id, int default_option, undefined param_4) {
  return YesNoMenu(param_1, message_id, default_option, param_4); // Original function call

  // Show a message when the player tries to quicksave (unused in the updated version)
  // PrintDialogue(NULL, NULL, "Quicksaving is not supported in this game.", true);
  // HideMinimap();
  // return false;
}

uint8_t __attribute__((naked)) TrampolineCallOriginalMoveLoggingRelated(struct entity* entity, struct move* move, char* maybe_move_name, uint32_t param_4, int param_5, uint8_t param_6) {
  asm("stmdb  sp!,{r3,r4,r5,r6,r7,r8,r9,r10,r11,lr}"); // Replaced instruction
  asm("b MoveLoggingRelated+4");
}

uint8_t __attribute__((used)) HookMoveLoggingRelated(struct entity* entity, struct move* move, char* maybe_move_name, uint32_t param_4, int param_5, uint8_t param_6)
{
  if (move != NULL && move->id.val == MOVE_ROCK_SMASH
      && GetChebyshevDistance(&entity->pos, &DUNGEON_PTR->display_data.camera_pos) > 5) {
    return 0; // Don't log Rock Smash if the camera is too far away
  }
  return TrampolineCallOriginalMoveLoggingRelated(entity, move, maybe_move_name, param_4, param_5, param_6);
}

int __attribute__((used)) CustomPlayRockSmashAnimation(struct position* pos) {
  // Play the Rock Smash animation only if the camera is close enough
  if (GetChebyshevDistance(pos, &DUNGEON_PTR->display_data.camera_pos) <= 5) {
    return PlayRockSmashAnimation(pos);
  }
  return 0;
}

// Similar to the original function, but it allows talking to
// Amber while they're inside a wall.
struct entity* __attribute__((used)) CustomGetTargetableMonsterInFacingDirection(struct entity* entity) {
  struct monster* monster = entity->info;
  enum direction_id direction = monster->action.direction.val;

  struct tile* tile = GetTile(entity->pos.x + DIRECTIONS_XY[direction][0],
                              entity->pos.y + DIRECTIONS_XY[direction][1]);

  // The original function checks if the monster on the tile actually has the type
  // ENTITY_MONSTER, which seems redundant but let's keep it anyway to be safe.
  if (tile != NULL && IsMonster(tile->monster)) {
    struct monster* target_monster = tile->monster->info;
    if (CanAttackInDirection(entity, direction) || target_monster->joined_at.val == DUNGEON_BEACH /* Remember Place (Amber) */) {
      return tile->monster;
    }
  }
  return NULL;
}

// Same as the original function
static int GetColorCodePaletteOffsetOriginal(int index) {
  switch(index) {
    case 'A':
      return 0x26;
    case 'B':
      return 0x23;
    case 'C':
      return 0x19;
    case 'E':
      return 0x18;
    case 'F':
      return 0x1d;
    case 'G':
      return 0x29;
    case 'H':
      return 0x2c;
    case 'I':
      return 0x26;
    case 'J':
      return 0x2d;
    case 'K':
      return 0x1f;
    case 'L':
      return 0x2b;
    case 'M':
      return 0x20;
    case 'N':
      return 0x1e;
    case 'O':
      return 0x2e;
    case 'P':
      return 0x21;
    case 'Q':
      return 0x25;
    case 'R':
      return 0x2a;
    case 'S':
      return 0x24;
    case 'T':
      return 0;
    case 'U':
      return 0x27;
    case 'V':
      return 0x22;
    case 'W':
      return 0x1a;
    case 'X':
      return 0x1b;
    case 'Y':
      return 0x1c;
    case 'Z':
      return 0x28;
    case 0x6a:
      return 0x21;
    case 0x72:
      return 0x15;
  }
  return 0x17;
}

static int GetColorCodePaletteOffsetGrayscale(uint8_t index) {
   // Intentionally overflow into the `FONT/markfont.dat` palette
  // (thanks to Irdkwia for the cursed knowledge)

  #define DARK_GRAY 0x83
  #define GRAY 0x84
  #define LIGHT_GRAY 0x85

  switch (index) {
    case 'A':
    case 'C':
    case 'F':
    case 'K':
    case 'L':
    case 'M':
    case 'J':
    case 'R':
    case 'X':
      return LIGHT_GRAY;
    case 'B':
    case 'P':
    case 'Q':
    case 'S':
      return DARK_GRAY;
    case 'E':
    case 'G':
    case 'H':
    case 'I':
    case 'N':
    case 'O':
    case 'Y':
    case 'Z':
      return GRAY;
    case 'T':
      return 0;
  }
  return 0x17; // White
}

int __attribute__((used)) CustomGetColorCodePaletteOffset(int index) {
  if (LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, 253) != 0) {
    return GetColorCodePaletteOffsetGrayscale(index);
  }
  
  return GetColorCodePaletteOffsetOriginal(index);
}
