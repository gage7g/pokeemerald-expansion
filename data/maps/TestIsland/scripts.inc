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


@ Test: gym gimmicks. Talk to the referee and pick a gym. You fight that gym leader, and the
@ battle starts with the gym's gimmick (weather, terrain, Swamp, Stealth Rock, Magic Room, Dark boost).
@ Your team is healed before and after. The leader's beaten flag is cleared so you can retest.
TestIsland_EventScript_GymReferee::
	lock
	faceplayer
	message TestIsland_Text_GymPick
	waitmessage
	dynmultichoice 0, 0, FALSE, 5, 0, DYN_MULTICHOICE_CB_NONE, TestIsland_Text_Gym1, TestIsland_Text_Gym2, TestIsland_Text_Gym3, TestIsland_Text_Gym4, TestIsland_Text_Gym5, TestIsland_Text_Gym6, TestIsland_Text_Gym7, TestIsland_Text_Gym8, TestIsland_Text_GymCancel
	closemessage
	switch VAR_RESULT
	case 0, TestIsland_EventScript_Gym1
	case 1, TestIsland_EventScript_Gym2
	case 2, TestIsland_EventScript_Gym3
	case 3, TestIsland_EventScript_Gym4
	case 4, TestIsland_EventScript_Gym5
	case 5, TestIsland_EventScript_Gym6
	case 6, TestIsland_EventScript_Gym7
	case 7, TestIsland_EventScript_Gym8
	release
	end

TestIsland_EventScript_Gym1::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_BROOKS, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_BROOKS
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_Gym2::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_MARLEY, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_MARLEY
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_Gym3::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_GIDEON, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_GIDEON
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_Gym4::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_APOLLO, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_APOLLO
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_Gym5::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_ROYCE, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_ROYCE
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_Gym6::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_TOBIAS, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_TOBIAS
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_Gym7::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_VENESSA, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_VENESSA
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_Gym8::
	special HealPlayerParty
	trainerbattle_no_intro TRAINER_LEADER_TYLER, TestIsland_Text_GymWin
	cleartrainerflag TRAINER_LEADER_TYLER
	goto TestIsland_EventScript_GymDone

TestIsland_EventScript_GymDone::
	special HealPlayerParty
	msgbox TestIsland_Text_GymAgain, MSGBOX_DEFAULT
	release
	end

TestIsland_Text_GymPick:
	.string "Which gym gimmick do you\n"
	.string "want to test?$"

TestIsland_Text_Gym1:
	.string "Gym 1 Rain$"

TestIsland_Text_Gym2:
	.string "Gym 2 Swamp$"

TestIsland_Text_Gym3:
	.string "Gym 3 Snow$"

TestIsland_Text_Gym4:
	.string "Gym 4 Sun$"

TestIsland_Text_Gym5:
	.string "Gym 5 None$"

TestIsland_Text_Gym6:
	.string "Gym 6 Psychic$"

TestIsland_Text_Gym7:
	.string "Gym 7 Rocks$"

TestIsland_Text_Gym8:
	.string "Gym 8 Fight Club$"

TestIsland_Text_GymCancel:
	.string "Cancel$"

TestIsland_Text_GymWin:
	.string "Gimmick test done!$"

TestIsland_Text_GymAgain:
	.string "Your team is healed.\n"
	.string "Talk to me to test another gym!$"
