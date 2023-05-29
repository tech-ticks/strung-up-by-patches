#include <pmdsky.h>
#include <cot.h>
#include "extern.h"
#include "common.h"

static void RunMonsterAiYoullAlwaysFindMe(struct entity* entity, struct monster* monster, undefined param_2) {
  struct entity* leader = GetLeader();
  if (!EntityIsValid(leader)) {
    return;
  }

  if (GetChebyshevDistance(&entity->pos, &leader->pos) <= 2) {
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
    SetActionPassTurnOrWalk(&monster->action, monster->id.val);
  }
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
  if (TryRockSmashTile(monster,  move_index, entity->pos.x - 1, entity->pos.y, DIR_LEFT)) {
    return;
  }
  if (TryRockSmashTile(monster,  move_index, entity->pos.x, entity->pos.y - 1, DIR_UP)) {
    return;
  }
  if (TryRockSmashTile(monster,  move_index, entity->pos.x, entity->pos.y + 1, DIR_DOWN)) {
    return;
  }

  return RunMonsterAi(entity, param_2);
}

void CustomRunMonsterAi(struct entity* entity, undefined param_2) {
  struct monster* monster = entity->info;

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

bool ShouldUseLayoutWithoutHallways() {
  return DUNGEON_PTR->id.val == DUNGEON_DRENCHED_BLUFF && DUNGEON_PTR->floor == 1;
}

void HookCreateHallway(int x0, int y0, int x1, int y1, bool vertical, int x_mid, int y_mid) {
  if (ShouldUseLayoutWithoutHallways()) {
    return;
  }

  TrampolineCallOriginalCreateHallway(x0, y0, x1, y1, vertical, x_mid, y_mid);
}

void __attribute__((naked)) TrampolineCallOriginalCreateHallway(int x0, int y0, int x1, int y1, bool vertical, int x_mid, int y_mid) {
  asm("stmdb sp!,{r3,r4,r5,r6,r7,r8,r9,r10,r11,lr}"); // Replaced instruction
  asm("b CreateHallway+4");
}

bool HookStairsAlwaysReachable(int x_stairs, int y_stairs, bool mark_unreachable) {
  if (ShouldUseLayoutWithoutHallways()) {
    // Disable the check for whether the stairs are reachable to avoid the one room monster house
    return true;
  }

  TrampolineCallOriginalStairsAlwaysReachable(x_stairs, y_stairs, mark_unreachable);
}

void __attribute__((naked)) TrampolineCallOriginalStairsAlwaysReachable(int x_stairs, int y_stairs, bool mark_unreachable) {
  asm("stmdb sp!,{r4,r5,r6,r7,r8,r9,r10,lr}"); // Replaced instruction
  asm("b StairsAlwaysReachable+4");
}
