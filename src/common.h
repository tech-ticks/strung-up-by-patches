#pragma once

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Set to the highest floor in the current dungeon
#define VAR_HIGHEST_DUNGEON_FLOOR VAR_RECYCLE_COUNT

// Set to 1 if the PP Zero Trap stairs could not be spawned
#define VAR_PP_ZERO_TRAP_SPAWN_FAILED VAR_SUB30_PROJECTP