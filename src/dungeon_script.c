#include "dungeon_script.h"

#include <pmdsky.h>
#include <cot.h>

#include "common.h"
#include "extern.h"
#include "items.h"

#define DNOPCODE_END 0x00
#define DNOPCODE_DIALOGUE 0x01
#define DNOPCODE_DIALOGUE_WAIT 0x02
#define DNOPCODE_EFFECT 0x03
#define DNOPCODE_EFFECT_WAIT 0x04
#define DNOPCODE_WAIT 0x05
#define DNOPCODE_PLAY_SFX 0x06
#define DNOPCODE_PLAY_ME 0x07
#define DNOPCODE_JUMP 0x08
#define DNOPCODE_JUMP_EQ 0x09
#define DNOPCODE_JUMP_NE 0x0A

#define DNOPCODE_YES_NO 0x10

#define DNOPCODE_CHECK_ITEM 0x20
#define DNOPCODE_GIVE_ITEM 0x21
#define DNOPCODE_TAKE_ITEM 0x22
#define DNOPCODE_CHECK_BAG_COUNT 0x23
#define DNOPCODE_CHECK_MOVE 0x2A

#define DNOPCODE_CHECK_TALK_FLAG 0x30
#define DNOPCODE_SET_TALK_FLAG 0x31

#define DNOPCODE_WAS_ATTACKED 0x42
#define DNOPCODE_CHECK_KEY_LOST 0x43
#define DNOPCODE_REMOVE_KEY_LOST 0x44
#define DNOPCODE_WARP 0x45

#define LOG_CATEGORY "dungeon_script"

struct ScriptEngineState {
  void* ip; // Instruction pointer
  struct entity* npc_monster;
  bool condition_flag;
};

#define SCRIPT_BUFFER_SIZE 0x800
static uint8_t SCRIPT_BUFFER[SCRIPT_BUFFER_SIZE];

struct ScriptEngineString {
  uint16_t length;
  char chars[];
} __attribute__((packed));

static bool LoadDungeonScriptToBuffer(char* name) {
  struct file_stream file;
  char file_name_buffer[0x40];
  Snprintf(file_name_buffer, 0x40, "SKETCHES/DNSCRPT/%s.dnscrpt", name);
  DataTransferInit();
  FileInit(&file);
  FileOpen(&file, file_name_buffer);

  int size = FileGetSize(&file);
  if (size > SCRIPT_BUFFER_SIZE) {
    COT_ERRORFMT(LOG_CATEGORY, "Dungeon script too large: '%s'", file_name_buffer);
    FileClose(&file);
    DataTransferStop();
    return false;
  }

  int read_bytes = FileRead(&file, (void*)SCRIPT_BUFFER, size);
  FileClose(&file);
  DataTransferStop();
  if (read_bytes <= 0) {
    COT_ERRORFMT(LOG_CATEGORY, "Failed to load dungeon script '%s'", file_name_buffer);
    return false;
  }

  return true;
}

static struct entity* GetScriptCharacterByIndex(struct ScriptEngineState* state, uint8_t index) {  
  if (index == 255 && state->npc_monster != NULL) {
    return state->npc_monster;
  }

  if (index > 0 && index <= 20) {
    struct entity* entity = DUNGEON_PTR->entity_table.header.active_monster_ptrs[index - 1];
    if (EntityIsValid(entity)) {
      return entity;
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

// Note: This function returns the exit code, while all other
// functions return the number of bytes consumed.
static int HandleOpEnd(struct ScriptEngineState* state) {
  struct OpEnd {
    uint8_t exit_code;
  } __attribute__((packed));

  struct OpEnd* op = (struct OpEnd*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpEnd: exit_code=%d", op->exit_code);

  return op->exit_code;
}

static int HandleOpDialogue(struct ScriptEngineState* state, bool wait) {
  struct OpDialogue {
    uint8_t chara;
    uint8_t portrait;
    struct ScriptEngineString string;
  } __attribute__((packed));

  struct OpDialogue* op = (struct OpDialogue*)state->ip;

  if (wait) {
    COT_LOGFMT(LOG_CATEGORY, "OpDialogueWait: chara=%d portrait=%d string=%s", op->chara, op->portrait, op->string.chars);
  } else {
    COT_LOGFMT(LOG_CATEGORY, "OpDialogue: chara=%d portrait=%d string=%s", op->chara, op->portrait, op->string.chars);
  }

  struct entity* entity = GetScriptCharacterByIndex(state, op->chara);

  if (entity != NULL) {
    struct monster* monster = entity->info;
    struct portrait_box portrait_box;
    InitPortraitData(&portrait_box, monster->id.val, op->portrait);
    PrintDialogue(NULL, &portrait_box, op->string.chars, wait);
  } else {
    PrintDialogue(NULL, NULL, op->string.chars, wait);
  }

  return sizeof(struct OpDialogue) + op->string.length;
}

static int HandleOpEffect(struct ScriptEngineState* state, bool wait) {
  struct OpEffect {
    uint8_t chara;
    uint16_t effect;
  } __attribute__((packed));

  struct OpEffect* op = (struct OpEffect*)state->ip;

  if (wait) {
    COT_LOGFMT(LOG_CATEGORY, "OpEffectWait: chara=%d effect=%d", op->chara, op->effect);
  } else {
    COT_LOGFMT(LOG_CATEGORY, "OpEffect: chara=%d effect=%d", op->chara, op->effect);
  }

  struct entity* entity = GetScriptCharacterByIndex(state, op->chara);
  int field_0x19 = GetEffectAnimationField0x19(op->effect);
  PlayEffectAnimationEntity(entity, op->effect, wait, field_0x19 && 0xff, 2, 0, -1, NULL);

  return sizeof(struct OpEffect);
}

static int HandleOpPlaySfx(struct ScriptEngineState* state) {
  struct OpPlaySfx {
    uint16_t sfx_id;
  } __attribute__((packed));

  struct OpPlaySfx* op = (struct OpPlaySfx*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpPlaySfx: sfx_id=%d", op->sfx_id);

  PlaySe(op->sfx_id, 0x100);

  return sizeof(struct OpPlaySfx);
}

static int HandleOpPlayMe(struct ScriptEngineState* state) {
  struct OpPlayMe {
    uint16_t me_id;
  } __attribute__((packed));

  struct OpPlayMe* op = (struct OpPlayMe*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpPlayMe: me_id=%d", op->me_id);

  PlayME(op->me_id, 0x100, 0);

  return sizeof(struct OpPlayMe);
}

static int HandleOpWait(struct ScriptEngineState* state) {
  struct OpWait {
    uint16_t wait_frames;
  } __attribute__((packed));

  struct OpWait* op = (struct OpWait*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpWait: wait_frames=%d", op->wait_frames);
  WaitFrames(op->wait_frames);

  return sizeof(struct OpWait);
}

static int HandleOpJump(struct ScriptEngineState* state) {
  struct OpJump {
    int32_t jump_offset;
  } __attribute__((packed));

  struct OpJump* op = (struct OpJump*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpJump: jump_offset=%d", op->jump_offset);

  return op->jump_offset - 1; // -1 because the ip has already been incremented
}

static int HandleOpJumpConditional(struct ScriptEngineState* state, bool condition) {
  struct OpJumpConditional {
    int32_t jump_offset;
  } __attribute__((packed));

  struct OpJumpConditional* op = (struct OpJumpConditional*)state->ip;

  if (condition) {
    COT_LOGFMT(LOG_CATEGORY, "OpJumpEq: jump_offset=%d", op->jump_offset);
  } else {
    COT_LOGFMT(LOG_CATEGORY, "OpJumpNe: jump_offset=%d", op->jump_offset);
  }

  if (state->condition_flag == condition) {
    return op->jump_offset - 1; // -1 because the ip has already been incremented
  } else {
    return sizeof(struct OpJumpConditional);
  }
}

static int HandleOpYesNo(struct ScriptEngineState* state) {
  struct OpYesNo {
    uint8_t chara;
    uint8_t portrait;
    uint8_t default_val;
    struct ScriptEngineString string;
  } __attribute__((packed));

  struct OpYesNo* op = (struct OpYesNo*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpYesNo: chara=%d portrait=%d default=%d string=%s", op->chara, op->portrait, op->default_val, op->string.chars);

  struct entity* entity = GetScriptCharacterByIndex(state, op->chara);
  if (entity != NULL) {
    struct monster* monster = entity->info;
    struct portrait_box portrait_box;
    InitPortraitData(&portrait_box, monster->id.val, op->portrait);
    bool res = YesNoMenuWithStringPtr(&portrait_box, op->string.chars, op->default_val, 0, op->default_val) == 1;
    state->condition_flag = res;
  } else {
    bool res = YesNoMenuWithStringPtr(NULL, op->string.chars, op->default_val, 0, op->default_val) == 1;
    state->condition_flag = res;
  }

  return sizeof(struct OpYesNo) + op->string.length;
}

static int HandleOpCheckItem(struct ScriptEngineState* state) {
  struct OpCheckItem {
    uint16_t item_id;
    uint8_t amount;
  } __attribute__((packed));

  struct OpCheckItem* op = (struct OpCheckItem*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpCheckItem: item_id=%d amount=%d", op->item_id, op->amount);

  state->condition_flag = HasItemAmount(op->item_id, op->amount);

  return sizeof(struct OpCheckItem);
}

static int HandleOpGiveItem(struct ScriptEngineState* state) {
  struct OpGiveItem {
    uint16_t item_id;
    uint8_t amount;
  } __attribute__((packed));

  struct OpGiveItem* op = (struct OpGiveItem*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpGiveItem: item_id=%d amount=%d", op->item_id, op->amount);

  state->condition_flag = GiveItem(op->item_id, op->amount);

  return sizeof(struct OpGiveItem);
}

static int HandleOpTakeItem(struct ScriptEngineState* state) {
  struct OpTakeItem {
    uint16_t item_id;
  } __attribute__((packed));

  struct OpTakeItem* op = (struct OpTakeItem*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpTakeItem: item_id=%d", op->item_id);

  state->condition_flag = TakeItem(op->item_id);

  return sizeof(struct OpTakeItem);
}

static int HandleOpCheckBagCount(struct ScriptEngineState* state) {
  struct OpCheckBagCount {
    uint8_t amount;
  } __attribute__((packed));

  struct OpCheckBagCount* op = (struct OpCheckBagCount*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpCheckBagCount: amount=%d", op->amount);

  state->condition_flag = GetBagItemCount() >= op->amount;

  return sizeof(struct OpCheckBagCount);
}

static int HandleOpCheckMove(struct ScriptEngineState* state) {
  struct OpCheckMove {
    uint8_t chara;
    uint16_t move_id;
  } __attribute__((packed));

  struct OpCheckMove* op = (struct OpCheckMove*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpCheckMove: chara=%d move_id=%d", op->chara, op->move_id);

  state->condition_flag = false;
  struct entity* entity = GetScriptCharacterByIndex(state, op->chara);
  if (entity) {
    struct monster* monster = entity->info;
    for (int i = 0; i < 4; i++) {
      struct move* move = &monster->moves[i];
      if (move->f_exists && move->id.val == op->move_id) {
        state->condition_flag = true;
        break;
      }
    }
  }

  return sizeof(struct OpCheckMove);
}

static int HandleOpCheckTalkFlag(struct ScriptEngineState* state) {
  struct OpCheckTalkFlag {
    uint8_t index;
  } __attribute__((packed));

  struct OpCheckTalkFlag* op = (struct OpCheckTalkFlag*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpCheckTalkFlag: flag=%d", op->index);

  state->condition_flag = LoadScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, op->index);

  return sizeof(struct OpCheckTalkFlag);
}

static int HandleOpSetTalkFlag(struct ScriptEngineState* state) {
  struct OpSetTalkFlag {
    uint8_t index;
    uint8_t value;
  } __attribute__((packed));

  struct OpSetTalkFlag* op = (struct OpSetTalkFlag*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpSetTalkFlag: flag=%d value=%d", op->index, op->value);

  SaveScriptVariableValueAtIndex(NULL, VAR_SCENARIO_TALK_BIT_FLAG, op->index, op->value == 1 ? 1 : 0);

  return sizeof(struct OpSetTalkFlag);
}

static int HandleOpWasAttacked(struct ScriptEngineState* state) {
  struct OpWasAttacked {
    uint8_t chara;
  } __attribute__((packed));

  struct OpWasAttacked* op = (struct OpWasAttacked*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpWasAttacked: chara=%d", op->chara);

  struct entity* entity = GetScriptCharacterByIndex(state, op->chara);
  if (entity) {
    struct monster* monster = entity->info;
    state->condition_flag = HasMonsterBeenAttackedInDungeons(monster->id.val);
  } else {
    state->condition_flag = false;
  }

  return sizeof(struct OpWasAttacked);
}

static int HandleOpCheckKeyLost(struct ScriptEngineState* state) {
  struct OpCheckKeyLost {
    uint8_t key_id;
  } __attribute__((packed));

  struct OpCheckKeyLost* op = (struct OpCheckKeyLost*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpCheckKeyLost: key_id=%d", op->key_id);

  state->condition_flag = IsKeyLost(op->key_id);

  return sizeof(struct OpCheckKeyLost);
}

static int HandleOpRemoveKeyLost(struct ScriptEngineState* state) {
  struct OpRemoveKeyLost {
    uint8_t key_id;
  } __attribute__((packed));

  struct OpRemoveKeyLost* op = (struct OpRemoveKeyLost*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpRemoveKeyLost: key_id=%d", op->key_id);

  RemoveKeyLost(op->key_id);

  return sizeof(struct OpRemoveKeyLost);
}

static int HandleOpWarp(struct ScriptEngineState* state) {
  struct OpWarp {
    uint8_t chara;
    uint8_t type;
    uint8_t x;
    uint8_t y;
  } __attribute__((packed));

  struct OpWarp* op = (struct OpWarp*)state->ip;

  COT_LOGFMT(LOG_CATEGORY, "OpWarp: chara=%d type=%d x=%d y=%d", op->chara, op->type, op->x, op->y);

  struct entity* entity = GetScriptCharacterByIndex(state, op->chara);

  if (entity) {
    struct position pos = {
      .x = op->x,
      .y = op->y,
    };
    TryWarp(entity, entity, op->type, &pos);
  }

  return sizeof(struct OpWarp);
}

uint8_t RunDungeonScript(char* name, struct entity* npc_monster) {
  if (!LoadDungeonScriptToBuffer(name)) {
    LogMessage(NULL, "Failed to load dungeon script", true);
    return 0;
  }

  struct ScriptEngineState state = {
    .ip = SCRIPT_BUFFER,
    .npc_monster = npc_monster,
    .condition_flag = false,
  };

  while (true) {
    uint8_t opcode = *(uint8_t*)state.ip;
    state.ip += 1;
    if ((uint8_t*) state.ip >= SCRIPT_BUFFER + 0x400) {
      COT_ERROR(LOG_CATEGORY, "Dungeon script out of bounds");
      WaitForever();
    }

    switch(opcode) {
      case DNOPCODE_END: {
        return HandleOpEnd(&state);
        break;
      }
      case DNOPCODE_DIALOGUE: {
        state.ip += HandleOpDialogue(&state, false);
        break;
      }
      case DNOPCODE_DIALOGUE_WAIT: {
        state.ip += HandleOpDialogue(&state, true);
        break;
      }
      case DNOPCODE_EFFECT: {
        state.ip += HandleOpEffect(&state, false);
        break;
      }
      case DNOPCODE_EFFECT_WAIT: {
        state.ip += HandleOpEffect(&state, true);
        break;
      }
      case DNOPCODE_WAIT: {
        state.ip += HandleOpWait(&state);
        break;
      }
      case DNOPCODE_JUMP: {
        state.ip += HandleOpJump(&state);
        break;
      }
      case DNOPCODE_PLAY_SFX: {
        state.ip += HandleOpPlaySfx(&state);
        break;
      }
      case DNOPCODE_PLAY_ME: {
        state.ip += HandleOpPlayMe(&state);
        break;
      }
      case DNOPCODE_JUMP_EQ: {
        state.ip += HandleOpJumpConditional(&state, true);
        break;
      }
      case DNOPCODE_JUMP_NE: {
        state.ip += HandleOpJumpConditional(&state, false);
        break;
      }

      case DNOPCODE_YES_NO: {
        state.ip += HandleOpYesNo(&state);
        break;
      }

      case DNOPCODE_CHECK_ITEM: {
        state.ip += HandleOpCheckItem(&state);
        break;
      }
      case DNOPCODE_GIVE_ITEM: {
        state.ip += HandleOpGiveItem(&state);
        break;
      }
      case DNOPCODE_TAKE_ITEM: {
        state.ip += HandleOpTakeItem(&state);
        break;
      }
      case DNOPCODE_CHECK_BAG_COUNT: {
        state.ip += HandleOpCheckBagCount(&state);
        break;
      }
      case DNOPCODE_CHECK_MOVE: {
        state.ip += HandleOpCheckMove(&state);
        break;
      }

      case DNOPCODE_CHECK_TALK_FLAG: {
        state.ip += HandleOpCheckTalkFlag(&state);
        break;
      }
      case DNOPCODE_SET_TALK_FLAG: {
        state.ip += HandleOpSetTalkFlag(&state);
        break;
      }

      case DNOPCODE_WAS_ATTACKED: {
        state.ip += HandleOpWasAttacked(&state);
        break;
      }
      case DNOPCODE_CHECK_KEY_LOST: {
        state.ip += HandleOpCheckKeyLost(&state);
        break;
      }
      case DNOPCODE_REMOVE_KEY_LOST: {
        state.ip += HandleOpRemoveKeyLost(&state);
        break;
      }
      case DNOPCODE_WARP: {
        state.ip += HandleOpWarp(&state);
        break;
      }

      default: {
        COT_ERRORFMT(LOG_CATEGORY, "Unknown dungeon script opcode %d", opcode);
        WaitForever();
        break;
      }
    }
  }
}