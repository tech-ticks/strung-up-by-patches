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
    default:
      return false;
  }
}
