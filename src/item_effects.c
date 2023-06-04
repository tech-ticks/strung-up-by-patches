#include <pmdsky.h>
#include <cot.h>

#include "common.h"
#include "extern.h"

// This has to be a global variable to make it accessible in `MenuEntryFn`
static struct ether_menu_state {
  struct monster* target_monster;
  int menu_id;
} ether_menu_state = {
  .target_monster = NULL,
  .menu_id = -1,
};

static char* MenuEntryFn(char* string_buffer, int option_num) {
  struct move* move = &ether_menu_state.target_monster->moves[option_num];
  char* move_name = GetMoveName(move->id.val);

  if (ether_menu_state.menu_id >= 0 && option_num != 0) {
    DrawHorizontalLine(ether_menu_state.menu_id, 12, option_num * 16 + 14, 130, 0x17);
  }

  Sprintf(string_buffer, "[CS:K]%s[CLUM_SET:111]%2d[CLUM_SET:123]/[CLUM_SET:128]%2d[CR]", move_name, move->pp, GetMaxPp(move));
  return string_buffer;
}

static void ItemEther(struct entity* user, struct entity* target) {
  if (!IsMonster(user) || !IsMonster(target)) {
    return;
  }

  int move_index = 0;

  struct monster* user_monster = user->info;
  struct monster* target_monster = target->info;

  if (!user_monster->is_not_team_member) {
    // If an ally is using the item, show a menu to select a move.

    HideMinimap();
    AdvanceFrame(0);
    AdvanceFrame(0);

    int active_move_count = 0;
    for (int i = 0; i < 4; i++) {
      if (target_monster->moves[i].f_exists) {
        active_move_count++;
      }
    }

    struct advanced_menu_flags flags = {
      .a_accept = true,
      .menu_title = true,
      .custom_height = true,
    };
  
    struct advanced_menu_additional_info additional_info = {
      .item_height = 16,
      .string_id = 17901, // "Restore PP of which move?"
      .unknown_12 = 0x10
    };

    struct advanced_menu_layout layout = {
      .update_fn = (undefined*) 0x0202BD64, // No idea where this is from
      .x_offset = 2,
      .y_offset = 2,
      .width = 18,
      .height = 0,
      .top_screen = false,
      .frame_type = 0xFF
    };

    int flags_int = *(int*) &flags;
    // The target monster is accessed in `MenuEntryFn`, which is called by `CreateAdvancedMenu` and later
    ether_menu_state.target_monster = target_monster;

    int menu_id = CreateAdvancedMenu((undefined*) &layout, flags_int, (undefined*) &additional_info, (undefined*) MenuEntryFn, active_move_count, active_move_count);
    ether_menu_state.menu_id = menu_id; // Save the menu ID to draw lines in the menu
    while (IsAdvancedMenuActive(menu_id)) {
      AdvanceFrame(0);
    }
    move_index = GetAdvancedMenuResult(menu_id);

    FreeAdvancedMenu(menu_id);
    ether_menu_state.menu_id = -1;
    ether_menu_state.target_monster = NULL;

    AdvanceFrame(0);
    AdvanceFrame(0);
    ShowMinimap();
  } else {
    // Not any ally, select the move with the lowest PP
    int lowest_pp = target_monster->moves[0].pp;
    int lowest_pp_move_index = 0;

    for (int i = 1; i < 4; i++) {
      int pp = target_monster->moves[i].pp;
      if (pp < lowest_pp && pp < GetMaxPp(&target_monster->moves[i])) {
        lowest_pp = pp;
        lowest_pp_move_index = i;
      }
    }

    move_index = lowest_pp_move_index;
  }

  if (move_index >= 0 && move_index <= 4) {
    struct move* move = &target_monster->moves[move_index];
    int max_pp = GetMaxPp(move);
    int new_pp = move->pp + 10;
    if (new_pp > max_pp) {
      new_pp = max_pp;
    }

    if (new_pp > move->pp) {
      move->pp = new_pp;
      FUN_022e4964(target); // Play animation?
      SubstitutePlaceholderStringTags(0, target, 0);
      LogMessageByIdWithPopupCheckUserTarget(user, target, 3507);
    } else {
      SubstitutePlaceholderStringTags(0, target, 0);
      LogMessageByIdWithPopupCheckUserTarget(user, target, 3508);
    }
  }
}

// Called when using items. Should return true if a custom effect was applied.
bool CustomApplyItemEffect(
  struct entity* user, struct entity* target, struct item* item, bool is_thrown
) {
  switch (item->id.val) {
    case ITEM_MAX_ELIXIR:
      ItemEther(user, target);
      return true;
    default:
      return false;
  }
}
