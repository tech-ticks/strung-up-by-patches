# Custom file formats

These files can be generated by editing the lists in `gen_custom_files.py` and then running `python3 gen_custom_files.py`.

## `SKETCHES/dunnpcs.bin`

A list of NPCs in dungeons. Up to 32 NPCs can be assigned to each dungeon.

### Header
The number of entries (4-byte unsigned integer)

### Entries
After the header, a list of entries follows with the following structure:

- `uint16 monster_id`
- `uint16 script_id`
- `uint8 dungeon_id`
- `uint8 floor`
- `uint8 npc_type`
- `(optional, 0 if unused) uint8 parameter1`
- `(optional, 0 if unused) uint16 parameter2`

The script ID references a script named `SKETCHES/DNSCRPT/npc_[id].dnscrpt` (with a width of three digits, e.g. `npc_001.dnscrpt`). Note that every NPC must use behavior type 16 (`BEHAVIOR_SECRET_BAZAAR_KIRLIA`) and the monster ID must match.

`npc_type` can be one of the following:
- 0: Execute the referenced script with no special behavior
- 1: Execute the script and make the NPC pushable if it returns with `end 1`. The target is a trap with ID `parameter1`.

Once pushable NPCs have arrived at their desired target position, the script `SKETCHES/DNSCRPT/npc_[id]_clear.dnscrpt` is executed. If the NPC is talked to again, the initial script is triggered.
