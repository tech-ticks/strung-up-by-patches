# Dungeon scripting engine

A simple dialogue scripting engine in dungeons. You can compile a script with `python3 assembler.py [input] -o [output]`. The flag `-d` can be used to disassemble a compiled script for debugging purposes, e.g. `python3 assembler.py -d [file]`.

## Comments

Single-line comments that start with '#' are supported.

## Labels

Lines that end with ':' are treated as labels. You can reference labels with the `jump`, `jumpeq` and `jumpne` instructions:

```
loop:
dialoguewait 0 0 "Hello world"
jump @loop
```

## Instructions

Note that
- some instructions take a character ("char") as an argument. 1-4 are the player and team, 255 is context-dependent.
- strings are prefixed with a 2-byte length (including the null terminator)

### 0x0 End

End script execution, with a context-dependent exit code (usually 0).
Usage: `end [uint8 exit_code]`

### 0x1 Dialogue (4 bytes + string)

Show a dialogue box.
Usage: `dialogue [uint8 char] [uint8 portrait] [string]`

### 0x2 DialogueWait (4 bytes + string)

Show a dialogue box and wait until it's finished.
Usage: `dialoguewait [uint8 char] [uint8 emotion] [string]`

### 0x3 Effect (4 bytes)

Play an effect.
Usage: `effect [uint8 char] [uint16 effect]`

### 0x4 EffectWait (4 bytes)

Play an effect and wait until it's finished.
Usage: `effectwait [uint8 char] [uint16 effect]`

### 0x5 Wait (3 bytes)

Wait for the given number of frames.
Usage: `wait [uint16 frames]`

### 0x6 PlaySfx (3 bytes)

Play a sound effect.
Usage: `playsfx [uint16 sfx_id]`

### 0x7 PlayMe (3 bytes)

Play a jingle.
Usage: `playme [uint16 me_id]`

### 0x8 Jump (5 bytes)

Jump to the given relative offset in the script (in bytes).
Usage: `jump [int32 target]`

### 0x9 JumpEq (5 bytes)

Jump to the given relative offset in the script (in bytes) if the condition flag is true.
Usage: `jumpeq [int32 target]`

### 0xA JumpNe (5 bytes)

Jump to the given relative offset in the script (in bytes) if the condition flag is false.
Usage: `jumpne [int32 target]`

### 0x10 YesNo (4 bytes + string)

Show a dialogue box with a Yes/No option.
Sets the condition flag based on the result.
Usage: `dialogue [uint8 char] [uint8 portrait] [uint8 default_value] [string]`

### 0x20 CheckItem (4 bytes)

Check whether the player has at least the given amount of the specified item (the amount is ignored if the item doesn't stack).
Sets the condition flag based on the result.
Usage: `checkitem [uint16 item_id] [uint8 amount]`

### 0x21 GiveItem (4 bytes)

Give an item with the specified quantity to the player. Sets the condition flag to false if the inventory is already full and to true otherwise. If the item is stackable, the amount will not be added to an existing item; a new slot is used instead.
Usage: `giveitem [uint16 item_id] [uint8 amount]`

### 0x22 TakeItem (3 bytes)

Remove an item from the bag. Sets the condition flag to false if the item was not found in the bag and to true otherwise.
Usage: `takeitem [uint16 item_id]`

### 0x23 CheckBagCount (2 bytes)

Check if the player has at least the given amount of items in the bag (stackable items count as one per stack).
Sets the condition flag based on the result.
Usage: `checkbagcount [uint8 amount]`

### 0x2A CheckMove (4 bytes)

Check if a character has learned the move with the given ID.
Sets the condition flag based on the result.
Usage: `takeitem [uint8 char] [uint16 move_id]`

### 0x30 CheckTalkFlag (2 bytes)
Check if `SCENARIO_TALK_BIT_FLAG` on the given index is set.
Sets the condition flag based on the result.
Usage: `checktalkflag [uint8 idx]`

### 0x31 SetTalkFlag (3 bytes)
Sets `SCENARIO_TALK_BIT_FLAG` on the given index to the specified value (0 or 1).
Usage: `settalkflag [uint8 idx] [uint8 value]`

### 0x42 CheckAttacked (2 bytes)
Check whether the given character has been attacked in dungeons.
Sets the condition flag based on the result.
Usage: `checkattacked [uint8 char]`

### 0x43 CheckKeyLost (2 bytes)
Checks whether the key with the given ID is absent from the inventory or has been used on a key door.
Sets the condition flag based on the result.
Usage: `checkkeylost [uint8 key_id]`

### 0x44 RemoveKeyLost (2 bytes)
Removes the numbered key if it's present somewhere on the floor.
Usage: `removekeylost [uint8 key_id]`

### 0x45 Warp (2 bytes)
Warps a character. `type` can be one of the following (see the `warp_type` enum in pmdsky-debug):
- 0: Warp to a random position
- 1: Warp within 2 tiles of the stairs
- 2: Warp within 2 tiles of a specified position
- 3: Warp to an exact position
- 4: Warp within 3 tiles of the stairs
- 5: Warp within 2 tiles of the leader
Usage: `warp [uint8 chara] [uint8 type] [uint8 x_or_target_chara] [uint8 y]`

### 0x46 CheckApparentSpecies (4 bytes)
Checks if the given character has a specific species. The apparent species is checked, which is different from the actual species when using Transform.
Sets the condition flag based on the result.
Usage: `checkapparentspecies [uint8 chara] [uint16 species]`

### 0x47 SetNextFloorFlag (1 bytes)
Sets the flag to advance to the next floor.
Usage: `setnextfloorflag`