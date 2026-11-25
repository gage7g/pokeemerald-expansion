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

@ Test: forced battle with Shadow Lugia (level 10) so you can see its look. Stepping on the tile starts it.
TestIsland_EventScript_ShadowLugiaTest::
	lockall
	setvar VAR_TEMP_D, 1
	givemon SPECIES_LUGIA_SHADOW, 10
	msgbox TestIsland_Text_ShadowLugiaGiven, MSGBOX_DEFAULT
	msgbox TestIsland_Text_ShadowLugia, MSGBOX_DEFAULT
	setwildbattle SPECIES_LUGIA_SHADOW, 10
	dowildbattle
	releaseall
	end

TestIsland_Text_ShadowLugia:
	.string "A dark shape rises from the sea!$"

TestIsland_Text_ShadowLugiaGiven:
	.string "You got a Shadow Lugia too!$"

@ Test: Mega Evolution. Gives the Mega Bracelet and a Camerupt holding Cameruptite, then a trainer battle
@ against a Garchomp holding Garchompite Z, so both sides can Mega Evolve.
TestIsland_EventScript_MegaTest::
	lockall
	setvar VAR_TEMP_C, 1
	checkitem ITEM_MEGA_RING
	goto_if_eq VAR_RESULT, TRUE, TestIsland_EventScript_MegaTestGiveMon
	giveitem ITEM_MEGA_RING
TestIsland_EventScript_MegaTestGiveMon::
	givemon SPECIES_CAMERUPT, 50, item=ITEM_CAMERUPTITE, move1=MOVE_FLAMETHROWER, move2=MOVE_EARTH_POWER, move3=MOVE_ROCK_SLIDE, move4=MOVE_PROTECT
	goto_if_eq VAR_RESULT, 2, TestIsland_EventScript_NoRoom
	msgbox TestIsland_Text_MegaTestIntro, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_TEST_MEGA, TestIsland_Text_MegaTestDefeat
	releaseall
	end

TestIsland_Text_MegaTestIntro:
	.string "You got a Camerupt holding its Mega\n"
	.string "Stone. Time to test Mega Evolution!$"

TestIsland_Text_MegaTestDefeat:
	.string "Mega test done!$"

