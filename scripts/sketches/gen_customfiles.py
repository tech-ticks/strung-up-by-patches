# Generates the "dunnpcs.bin" and "dunflags.bin" files

import struct

TRAP_MUD_TRAP = 1
TRAP_STICKY_TRAP = 2
TRAP_GRIMY_TRAP = 3
TRAP_SUMMON_TRAP = 4
TRAP_PITFALL_TRAP = 5
TRAP_WARP_TRAP = 6
TRAP_GUST_TRAP = 7
TRAP_SPIN_TRAP = 8
TRAP_SLUMBER_TRAP = 9
TRAP_SLOW_TRAP = 10
TRAP_SEAL_TRAP = 11
TRAP_POISON_TRAP = 12
TRAP_SELFDESTRUCT_TRAP = 13
TRAP_EXPLOSION_TRAP = 14
TRAP_PP_ZERO_TRAP = 15
TRAP_CHESTNUT_TRAP = 16
TRAP_WONDER_TILE = 17
TRAP_POKEMON_TRAP = 18
TRAP_SPIKED_TILE = 19
TRAP_STEALTH_ROCK = 20
TRAP_TOXIC_SPIKES = 21
TRAP_TRIP_TRAP = 22
TRAP_RANDOM_TRAP = 23
TRAP_GRUDGE_TRAP = 24
TRAP_NONE = 25

DUNGEON_REMEMBER_PLACE = 1
DUNGEON_DAUNTING_DOLDRUMS = 2
DUNGEON_POLYPHONIC_PLAYGROUND = 3
DUNGEON_SINKING_SHADOWS = 4
DUNGEON_LITTLE_DREAM = 5

NPC_TYPE_NORMAL = 0
NPC_TYPE_PUSH_TO_TRAP = 1
NPC_TYPE_PUSH_TO_POKEMON = 2
NPC_TYPE_SHOW_MOVE = 3

npc_data = [
    {
        "monster_id": 35, # Clefairy
        "script_id": 0,
        "dungeon_id": DUNGEON_POLYPHONIC_PLAYGROUND,
        "floor": 1,
        "npc_type": NPC_TYPE_SHOW_MOVE,
        "parameter1": 0,
        "parameter2": 330 # Metronome
    },
    {
        "monster_id": 488, # Munchlax
        "script_id": 3,
        "dungeon_id": DUNGEON_POLYPHONIC_PLAYGROUND,
        "floor": 2,
        "npc_type": NPC_TYPE_NORMAL,
        "parameter1": 0,
        "parameter2": 0
    },
    {
        "monster_id": 286, # Mudkip
        "script_id": 1,
        "dungeon_id": DUNGEON_POLYPHONIC_PLAYGROUND,
        "floor": 3,
        "npc_type": NPC_TYPE_PUSH_TO_TRAP,
        "parameter1": TRAP_MUD_TRAP,
        "parameter2": 0
    },
    {
        "monster_id": 539, # Litwick
        "script_id": 2,
        "dungeon_id": DUNGEON_POLYPHONIC_PLAYGROUND,
        "floor": 4,
        "npc_type": NPC_TYPE_PUSH_TO_POKEMON,
        "parameter1": 0,
        "parameter2": 63, # Abra
    },
    {
        "monster_id": 56, # Mankey
        "script_id": 4,
        "dungeon_id": DUNGEON_POLYPHONIC_PLAYGROUND,
        "floor": 5,
        "npc_type": NPC_TYPE_PUSH_TO_TRAP,
        "parameter1": TRAP_CHESTNUT_TRAP,
        "parameter2": 0,
    },
    {
        "monster_id": 498, # Finneon
        "script_id": 5,
        "dungeon_id": DUNGEON_POLYPHONIC_PLAYGROUND,
        "floor": 6,
        "npc_type": NPC_TYPE_PUSH_TO_POKEMON,
        "parameter1": 0,
        "parameter2": 143, # Snorlax
    },
    {
        "monster_id": 306, # Wingull
        "script_id": 6,
        "dungeon_id": DUNGEON_POLYPHONIC_PLAYGROUND,
        "floor": 7,
        "npc_type": NPC_TYPE_PUSH_TO_TRAP,
        "parameter1": 0, # Nothing
        "parameter2": 0,
    },
    {
        "monster_id": 551, # Odd Keystone
        "script_id": 256,
        "dungeon_id": DUNGEON_SINKING_SHADOWS,
        "floor": 1,
        "npc_type": NPC_TYPE_NORMAL,
        "parameter1": 0,
        "parameter2": 0
    },
    {
        "monster_id": 551, # Odd Keystone
        "script_id": 257,
        "dungeon_id": DUNGEON_SINKING_SHADOWS,
        "floor": 13,
        "npc_type": NPC_TYPE_NORMAL,
        "parameter1": 0,
        "parameter2": 0
    },
    {
        "monster_id": 28, # Sandslash (male)
        "script_id": 260,
        "dungeon_id": DUNGEON_DAUNTING_DOLDRUMS,
        "floor": 2,
        "npc_type": NPC_TYPE_NORMAL,
        "parameter1": 0,
        "parameter2": 0
    },
    {
        "monster_id": 28, # Sandslash (female)
        "script_id": 261,
        "dungeon_id": DUNGEON_DAUNTING_DOLDRUMS,
        "floor": 9,
        "npc_type": NPC_TYPE_NORMAL,
        "parameter1": 0,
        "parameter2": 0
    },
    {
        "monster_id": 27, # Sandshrew (male)
        "script_id": 262,
        "dungeon_id": DUNGEON_DAUNTING_DOLDRUMS,
        "floor": 8,
        "npc_type": NPC_TYPE_NORMAL,
        "parameter1": 0,
        "parameter2": 0
    },
    {
        "monster_id": 27, # Sandshrew (female)
        "script_id": 263,
        "dungeon_id": DUNGEON_DAUNTING_DOLDRUMS,
        "floor": 7,
        "npc_type": NPC_TYPE_NORMAL,
        "parameter1": 0,
        "parameter2": 0
    },

    {
        "monster_id": 537, # Smeargle
        "script_id": 42,
        "dungeon_id": DUNGEON_LITTLE_DREAM,
        "floor": 5,
        "npc_type": NPC_TYPE_PUSH_TO_POKEMON,
        "parameter1": 0,
        "parameter2": 538 # Doll
    },
]

# Open the file in binary write mode
with open("dunnpcs.bin", "wb") as f:
    # Write the number of entries
    f.write(struct.pack("<I", len(npc_data)))

    # Write each entry
    for npc in npc_data:
        # Pack the data as described in the markdown file
        entry_data = struct.pack("<HHBBBBH", npc["monster_id"], npc["script_id"], npc["dungeon_id"], npc["floor"], npc["npc_type"], npc.get("parameter1", 0), npc.get("parameter2", 0))

        # Write the entry data
        f.write(entry_data)
