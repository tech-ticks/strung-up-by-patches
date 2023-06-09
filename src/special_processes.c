#include <pmdsky.h>
#include <cot.h>
#include "extern.h"
#include "common.h"

// SP 100: Set partner's tactic to Group Safety (replaced with custom tactic "You'll always find me")
static void SpSetPartnerTacticGroupSafety() {
  // Change the tactic of the active team member
  int partner_index = GetPartnerMemberIdx();
  if (partner_index > 0) {
    // `GetPartnerMemberIdx` should always return 1, unless the partner is not in the team.
    struct team_member* partner = GetActiveTeamMember(partner_index);
    if (partner != NULL) {
      partner->tactic.val = TACTIC_GROUP_SAFETY;
    }
  }

  // Also apply it to the inactive member ("mentry") for consistency
  struct ground_monster* ground_monster_partner = GetPartner();
  ground_monster_partner->tactic.val = TACTIC_GROUP_SAFETY;
}

// SP 101: Reset dungeon-related script engine globals
void SpResetDungeonGlobals() {
  SaveScriptVariableValue(NULL, VAR_PP_ZERO_TRAP_SPAWN_FAILED, 0);
  SaveScriptVariableValue(NULL, VAR_HIGHEST_DUNGEON_FLOOR, 0);
}

static void RemoveParty() {
  // Remove all active party members
  for (int i = 0; i < 4; i++) {
    struct team_member* team_member = GetActiveTeamMember(i);
    if (team_member != NULL) {
      team_member->f_is_valid = false;
    }
  }
}

// https://github.com/Adex-8x/EoS-ASM-Effects/blob/main/special_processes/monster_entry/add_mentry_to_party.asm
static int AddMentryToParty(int id) {
  AddIndexedSlot(id);
  ApplyPartyChange(*(((char* )&TEAM_MEMBER_TABLE_PTR) + 0x9877));
  return 0;
}

// SP 102: Time skip
void SpTimeSkip() {
  PLAY_TIME_SECONDS += 99 + 99 * 60 + 99 * 60 * 60;

  // Increase the level
  RemoveParty();
  struct ground_monster* ground_monster_hero = GetHero();
  InitTeamMemberDataBasedOnLevel(ground_monster_hero, 36, 0);
  AddMentryToParty(0);
}

// Called for special process IDs 100 and greater.
//
// Set return_val to the return value that should be passed back to the game's script engine. Return true,
// if the special process was handled.
bool CustomScriptSpecialProcessCall(undefined4* unknown, uint32_t special_process_id, short arg1, short arg2, int* return_val) {
  switch (special_process_id) {
    case 100:
      SpSetPartnerTacticGroupSafety();
      *return_val = 0;
      return true;
    case 101:
      SpResetDungeonGlobals();
      *return_val = 0;
      return true;
    case 102:
      SpTimeSkip();
      *return_val = 0;
      return true;
    default:
      return false;
  }
}
