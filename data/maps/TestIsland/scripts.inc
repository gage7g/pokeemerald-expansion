TestIsland_MapScripts::
	.byte 0

@ Test hooks. Walking onto the trigger tile in Pallet Town sends you here, and back.
TestIsland_EventScript_WarpIn::
	warp MAP_TEST_ISLAND, 9, 7
	waitstate
	end

TestIsland_EventScript_WarpOut::
	warp MAP_PALLET_TOWN, 5, 10
	waitstate
	end

@ Test: proves Gen 4 and up Pokemon and moves exist in this build. Stepping on the tile gives a Gen 9 Pokemon.
TestIsland_EventScript_GiveTestMon::
	lockall
	setvar VAR_TEMP_E, 1
	givemon SPECIES_SPRIGATITO, 5, move1=MOVE_FLOWER_TRICK, move2=MOVE_SCRATCH
	goto_if_eq VAR_RESULT, 2, TestIsland_EventScript_NoRoom
	msgbox TestIsland_Text_GotMon, MSGBOX_DEFAULT
	releaseall
	end

TestIsland_EventScript_NoRoom::
	msgbox TestIsland_Text_NoRoom, MSGBOX_DEFAULT
	releaseall
	end

TestIsland_Text_GotMon:
	.string "You got Sprigatito, a Gen 9 mon!$"

TestIsland_Text_NoRoom:
	.string "No room in your party.$"
