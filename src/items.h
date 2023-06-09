#pragma once

#include <pmdsky.h>
#include <cot.h>

// Dungeon mode item utilities
bool HasItem(enum item_id item);
bool HasItemAmount(enum item_id item, int quantity);
bool GiveItem(enum item_id item, int quantity);
bool TakeItem(enum item_id item);
int GetBagItemCount();
