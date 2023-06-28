#pragma once

#include <pmdsky.h>
#include <cot.h>

extern int TextboxSE;
extern uint16_t GBA_JOYPAD_KEYS;

void InitTeamMemberDataBasedOnLevel(struct ground_monster* mentry, int level, undefined unknown);
void ApplyPartyChange(uint8_t unk);
void AddIndexedSlot(int index);

void DrawHorizontalLine(int box_id, int x, int y, int width, int color);
void PrintDialogue(undefined unknown, struct portrait_box* portrait_box, char* string, bool wait_before_closing);
int YesNoMenuWithStringPtr(struct portrait_box* portrait_ptr, char* string, int default_option, int unknown, int maybe_also_default_option);
void HideMinimap();
void InitPortraitData(struct portrait_box* portrait_ptr, enum monster_id pokemon_id, int face_id);
void TalkBazaarPokemon(undefined4 unknown, struct entity* entity);
void MinimapRelated(undefined unk1, undefined unk2);
void FUN_022fb538(struct entity* entity); // unknown
void FUN_0234b010(undefined param_1);
void PlayEffectAnimationEntitySimple(struct entity* entity, int effect_id);
int TargetingCheckFunction(struct entity* user, struct entity* target, int param_3, int param_4);
uint8_t MoveLoggingRelated(struct entity* entity, struct move* move, char* maybe_move_name, uint32_t param_4, int param_5, uint8_t param_6);
int PlayRockSmashAnimation(struct position* pos);

// From a custom ASM patch
bool IsKeyLost(int key_id);
void RemoveKeyLost(int key_id);

static inline void ShowMinimap() {
  MinimapRelated(0, 0);
}

struct advanced_menu_flags {
  bool a_accept: 1;
  bool b_cancel: 1;
  bool accept_button: 1;
  bool up_down_buttons: 1;
  bool se_on: 1;
  bool set_choice: 1;
  bool unknown_6: 1;
  bool unknown_7: 1;
  bool unknown_8: 1;
  bool broken_first_choice: 1;
  bool custom_height: 1;
  bool menu_title: 1;
  bool menu_lower_bar: 1;
  bool list_button: 1;
  bool search_button: 1;
  bool unknown_15: 1;
  bool first_last_page_buttons: 1;
  bool up_down: 1;
  bool unknown_18: 1;
  bool unknown_19: 1;
  bool y_pos_end: 1;
  bool x_pos_end: 1;
  bool partial_menu: 1;
  bool no_cursor: 1;
  bool no_up_down: 1;
  bool no_left_right: 1;
  bool invisible_cursor: 1;
  bool only_list: 1;
  bool no_accept_button: 1;
  bool unknown_29: 1;
  bool unknown_30: 1;
  bool unknown_31: 1;
};

struct advanced_menu_additional_info {
  int header_height;
  int item_height;
  int string_id;
  int unknown_12;
};

struct advanced_menu_layout {
  undefined* update_fn;
  uint8_t x_offset;
  uint8_t y_offset;
  uint8_t width;
  uint8_t height;
  bool top_screen;
  uint8_t frame_type;
  undefined* unknown_c;
};

undefined* SetupDBoxFormat(struct advanced_menu_layout* layout, undefined unk);
