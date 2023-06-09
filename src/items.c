#include "extern.h"
#include "common.h"

#include "items.h"

bool HasItem(enum item_id item_id) {
    int capacity = GetCurrentBagCapacity();
    for (int i = 0; i < capacity; i++) {
        struct item* item = &BAG_ITEMS_PTR[i];
        if (item->f_exists && item->id.val == item_id) {
            return true;
        }
    }
    return false;
}

bool HasItemAmount(enum item_id item_id, int quantity) {
    int capacity = GetCurrentBagCapacity();
    for (int i = 0; i < capacity; i++) {
        struct item* item = &BAG_ITEMS_PTR[i];
        if (item->f_exists && item->id.val == item_id && item->quantity >= quantity) {
            return true;
        }
    }
    return false;
}

bool GiveItem(enum item_id item_id, int quantity) {
    int capacity = GetCurrentBagCapacity();
    for (int i = 0; i < capacity; i++) {
        struct item* item = &BAG_ITEMS_PTR[i];

        if (!item->f_exists) {
            InitItem(item, item_id, quantity, false);
            return true;
        }
    }
    return false;
}

bool TakeItem(enum item_id item_id) {
    int capacity = GetCurrentBagCapacity();
    for (int i = 0; i < capacity; i++) {
        struct item* item = &BAG_ITEMS_PTR[i];
        if (item->f_exists && item->id.val == item_id) {
            item->f_exists = false;
            item->id.val = ITEM_NOTHING;
            item->quantity = 0;
            return true;
        }
    }
    return false;
}

int GetBagItemCount() {
    int capacity = GetCurrentBagCapacity();
    int count = 0;
    for (int i = 0; i < capacity; i++) {
        struct item* item = &BAG_ITEMS_PTR[i];
        if (item->f_exists) {
            count++;
        }
    }
    return count;
}
