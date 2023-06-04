#pragma once

extern int TextboxSE;
extern uint16_t GBA_JOYPAD_KEYS;

void DrawHorizontalLine(int box_id, int x, int y, int width, int color);

static void ShowMinimap() {
  MinimapRelated(0, 0);
}