/* !file arm9 */
DrawHorizontalLine = 0x02025D50;
SaveFileIdentifier = 0x0209cd3c;
TextboxSE = 0x02017D1C;

/* Workaround for "undefined reference to 'memset'" */
memset = 0x2087308;

GBA_JOYPAD_KEYS = 0x4000130;

/* !file overlay10 */
GetTrapAnimationCallSite = 0x022bec40;

/* !file overlay29 */
ExecuteMoveEffectHookAddr = 0x0232F8A4;
ExecuteMoveEffectJumpAddr = 0x023326CC;
ApplyItemEffectHookAddr = 0x0231b9ac;
ApplyItemEffectJumpAddr = 0x0231CB14;

RunDungeonCallsite = 0x234bd68;
RunLeaderTurnCallsite = 0x22ebd34;
RunMonsterAiCallSite = 0x022ebe88;
RunMonsterAiCallSite2 = 0x022ec18c;
FUN_022fb538 = 0x022fb538;
FUN_022e4964 = 0x022e4964;
HideMinimap = 0x0234d558;
MinimapRelated = 0x022EA428;
SpawnTeamCallSite = 0x22df6a4;
PrintDialogue = 0x0234D304;

DrawMinimapTileReturn = 0x023398c0;
MarkNonEnemySpawnsCallsite1 = 0x0233acc0;
MarkNonEnemySpawnsCallsite2 = 0x0233ad84;

TryTriggerTrapCallsite1 = 0x22f3770;
TryTriggerTrapCallsite2 = 0x22feb54;
TryTriggerTrapCallsite3 = 0x2305800;

DisplayUiCallsite1 = 0x22e33c8;
DisplayUiCallsite2 = 0x22ea15c;
