#pragma once

extern int TextboxSE;
extern uint16_t GBA_JOYPAD_KEYS;

void DrawHorizontalLine(int box_id, int x, int y, int width, int color);
void PrintDialogue(undefined unknown, struct portrait_box* portrait_box, char* string, bool wait_before_closing);
void HideMinimap();

static void ShowMinimap() {
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
