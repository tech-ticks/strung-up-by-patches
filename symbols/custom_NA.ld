/* !file arm9 */
SaveFileIdentifier = 0x0209cd3c;
TextboxSE = 0x02017D1C;

DrawHorizontalLine = 0x02025D50;
InitTeamMemberDataBasedOnLevel = 0x020544C8;
ApplyPartyChange = 0x2057464;
AddIndexedSlot = 0x2056554;

SetupDBoxFormat = 0x2027648; /* Sets the textbox format up */

/* Workaround for "undefined reference to 'memset'" */
memset = 0x2087308;
memcpy = 0x208729C;

GBA_JOYPAD_KEYS = 0x4000130;

GetColorCodePaletteOffset = 0x20265C4;

/* !file overlay10 */
GetTrapAnimationCallSite = 0x022bec40;

/* !file overlay29 */
ExecuteMoveEffectHookAddr = 0x0232F8A4;
ExecuteMoveEffectJumpAddr = 0x023326CC;
ApplyItemEffectHookAddr = 0x0231b9ac;
ApplyItemEffectJumpAddr = 0x0231CB14;

RunDungeonCallsite = 0x234bd68;
RunLeaderTurnCallsite = 0x022ebd34;
RunMonsterAiCallsite1 = 0x022ebe88;
RunMonsterAiCallsite2 = 0x022ebfe8;
RunMonsterAiCallsite3 = 0x022ec18c;
RunMonsterAiCallsite4 = 0x022ec564;
RunMonsterAiCallsite5 = 0x022ec6b0;
FUN_022fb538 = 0x022fb538;
HideMinimap = 0x0234d558;
MinimapRelated = 0x022EA428;
SpawnTeamCallSite = 0x22df6a4;
PrintDialogue = 0x0234D304;
InitPortraitData = 0x0234BAC0;
YesNoMenuWithStringPtr = 0x0234d4bc;
PlayEffectAnimationEntitySimple = 0x022e42e8;

DrawMinimapTileReturn = 0x023398c0;
MarkNonEnemySpawnsCallsite1 = 0x0233acc0;
MarkNonEnemySpawnsCallsite2 = 0x0233ad84;
ConvertWallsToChasmsCheck = 0x0233ae30;
DisplayChasmsAsWallsCheck = 0x2336564;

TryTriggerTrapCallsite1 = 0x22f3770;
TryTriggerTrapCallsite2 = 0x22feb54;
TryTriggerTrapCallsite3 = 0x2305800;
ApplyTrapEffectCallsite1 = 0x22ee2c0;
ApplyTrapEffectCallsite2 = 0x022eefa8;
ApplyTrapEffectCallsite3 = 0x022ef04c;
TrapMinimapGraphics = 0x02352FF4; /* Maps each trap to its tile value */
TargetingCheckFunction = 0x230175c; /* Related to targeting */

DisplayUiCallsite1 = 0x22e33c8;
DisplayUiCallsite2 = 0x22ea15c;

TalkBazaarPokemon = 0x02344544; /* Seems to be called when talking to Bazaar mons */
TalkBazaarPokemonCallsite = 0x22f5bc8;

CheckFixedRoomItem = 0x02344454;
CanMonsterUseItemCallsitePlayer = 0x22f537c; /* Called when the player uses an item */
FUN_0234b010 = 0x0234b010;
FUN_0234b010Callsite = 0x22dfb30; /* Called after transitioning to another floor */
SpawnStairsCallsite = 0x2340e34;

TrapTeamCheck = 0x02305790;
TransformFixedRoomCheck = 0x232b458;
DoMoveTransform = 0x232b44c;
DoMovePounce = 0x232ca4c;
GetMonsterIdFromSpawnEntryCallsite = 0x23172cc;
RestoreMovePPOnFaint1 = 0x2309ef8;
RestoreMovePPOnFaint2 = 0x0230a1c8;
RestoreMovePPOnFaint3 = 0x230a410;
WrapTrapTryWarpCall = 0x22ef278;

MoveLoggingRelated = 0x2322ddc;
PlayRockSmashAnimation = 0x022e66c4;
PlayRockSmashAnimationCallsite = 0x2337c98;
TalkToTeammateCanAttackInDirectionCheck = 0x22f87d0;

/* !file overlay31 */
QuicksaveQuestion = 0x023888d0;

/* * !file overlay36 */
IsKeyLost = 0x023dd29c;
RemoveKeyLost = 0x23dd308;