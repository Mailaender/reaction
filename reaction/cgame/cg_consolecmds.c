//-----------------------------------------------------------------------------
//
// $Id$
//
//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.36  2002/03/24 22:51:30  niceass
// scoreboard toggle changes
//
// Revision 1.35  2002/03/19 06:00:14  niceass
// tab scoreboard quicky change to q2 way
//
// Revision 1.34  2002/03/18 19:19:08  slicer
// Fixed bandage bugs ( i hope )
//
// Revision 1.33  2002/03/17 13:41:28  jbravo
// Added a debug cmd to print out stuff when bugs occor
//
// Revision 1.32  2002/03/14 02:24:39  jbravo
// Adding radio :)
//
// Revision 1.31  2002/03/13 18:41:18  slicer
// Adjusted some of elder's unzoom code for the new sniper system ( server side )
//
// Revision 1.30  2002/03/07 00:03:00  assimon
// Added some trap_AddComand for ref  cvars
//
// Revision 1.29  2002/02/28 05:41:54  blaze
// weapons stats on client side
//
// Revision 1.28  2002/02/10 08:17:08  niceass
// many changes to scoreboard (deaths/second mode)
//
// Revision 1.27  2002/02/09 00:10:12  jbravo
// Fixed spectator follow and free and updated zcam to 1.04 and added the
// missing zcam files.
//
// Revision 1.26  2002/02/08 05:59:09  niceass
// scoreboard timer thing added
//
// Revision 1.25  2002/02/03 21:24:12  slicer
// More Matchmode code
//
// Revision 1.23  2002/01/11 20:20:57  jbravo
// Adding TP to main branch
//
// Revision 1.22  2002/01/11 19:48:29  jbravo
// Formatted the source in non DOS format.
//
// Revision 1.21  2001/12/31 16:28:41  jbravo
// I made a Booboo with the Log tag.
//
//
//-----------------------------------------------------------------------------
// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
extern menuDef_t *menuScoreboard;
#endif



void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendConsoleCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}

/*
=================
CG_DropWeapon_f

Elder: reset local zoom, then proceed with server action
=================
*/
static void CG_DropWeapon_f (void) {
	if ( !cg.snap ) {
		//CG_Printf("No snapshot: normally exiting\n");
		return;
	}

	// if we are going into the intermission, don't do anything
	if ( cg.intermissionStarted ) {
		return;
	}

	///Elder: spectator?
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

//	if ((cg.snap->ps.stats[STAT_RQ3] & RQ3_BANDAGE_WORK) == RQ3_BANDAGE_WORK)
	if(cg.snap->ps.weaponstate == WEAPON_BANDAGING)
	{
		CG_Printf("You are too busy bandaging!\n");
		return;
	}
//Slicer: Done Server Side
//	CG_RQ3_Zoom1x();
	trap_SendClientCommand("dropweapon");
}

/*
=================
CG_DropItem_f

Elder: Do any client pre-processing here for drop item
=================
*/
static void CG_DropItem_f (void) {
	if ( !cg.snap ) {
		//CG_Printf("No snapshot: normally exiting\n");
		return;
	}

	// if we are going into the intermission, don't do anything
	if ( cg.intermissionStarted ) {
		return;
	}

	///Elder: spectator?
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	trap_SendClientCommand("dropitem");
}

/*
=================
CG_Bandage_f

Elder:
check cases that are possible before sending
the client command to reduce bandwidth use slightly
=================
*/
static void CG_Bandage_f (void) {
	if ( !cg.snap ) {
		return;
	}

	// if we are going into the intermission, don't do anything
	if ( cg.intermissionStarted ) {
		return;
	}

	//Elder: spectator?
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	//Elder: prevent "bandaging" when dead hehe
	if ( cg.snap->ps.stats[STAT_HEALTH] < 0 ) {
		CG_Printf("It's a bit too late to bandage yourself...\n");
		return;
	}

//	if ( (cg.snap->ps.stats[STAT_RQ3] & RQ3_BANDAGE_WORK) == RQ3_BANDAGE_WORK) {
	if(cg.snap->ps.weaponstate == WEAPON_BANDAGING) {
		CG_Printf("You are already bandaging!\n");
		return;
	}

	//Elder: don't allow bandaging when in the middle of bursts
	if (cg.snap->ps.stats[STAT_BURST] > 0)
		return;

	//Elder: added to prevent bandaging while reloading or firing
	//Moved below "already-bandaging" case and removed message
	//if ( cg.snap->ps.weaponTime > 0 || cg.snap->ps.weaponstate == WEAPON_COCKED) {
		//CG_Printf("You are too busy with your weapon!\n");
		//return;
	//}
	if (cg.snap->ps.weaponTime > 0 || cg.snap->ps.stats[STAT_RELOADTIME] > 0)
		return;
//Slicer: Done Server Side
/*	if ( (cg.snap->ps.stats[STAT_RQ3] & RQ3_BANDAGE_NEED) == RQ3_BANDAGE_NEED) {
		CG_RQ3_Zoom1x();
	}
	*/
	trap_SendClientCommand("bandage");
}


/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}

/*
=============
CG_PlayerOrigin_f

Debugging command to print player's origin
=============
*/
static void CG_PlayerOrigin_f (void) {
	CG_Printf("(%i %i %i)\n", (int)cg.snap->ps.origin[0],
		(int)cg.snap->ps.origin[1], (int)cg.snap->ps.origin[2]);
}

/*
static void CG_ScoresDown_f( void ) {

#ifdef MISSIONPACK
		CG_BuildSpectatorString();
#endif
	if ( cg.time - cg.scoreFadeTime < 500 && !cg.showScores)
		cg.scoreTPMode = (cg.scoreTPMode == 0) ? 1 : 0;			// Toggle
	
	if ( cg.time - cg.scoreFadeTime >= 500 && !cg.showScores)
		cg.scoreTPMode = 0;

	if ( !cg.showScores ) cg.scoreStartTime = cg.time;

	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}
*/

static void CG_Scores_f ( void ) {

	if ( cgs.gametype >= GT_TEAM ) {
		if ( cg.showScores && cg.scoreTPMode == 1 ) {
			cg.showScores = qfalse;
			cg.scoreFadeTime = 0; //cg.time;
			cg.scoreTPMode = 0;
			return;
		}

		if (cg.scoreTPMode == 0 && cg.showScores) {
			cg.scoreTPMode = 1;
			return;
		}
	}
	else {
		if ( cg.showScores ) {
			cg.showScores = qfalse;
			cg.scoreFadeTime = 0; //cg.time;
			return;
		}
	}

	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.scoreStartTime = cg.time;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
		cg.scoreStartTime = cg.time;
	}
}

/*
=================================
CG_WeaponStatsUp_f
Turns on the players weapon stats
=================================
*/
static void CG_WeaponStatsUp_f(void)
{	
  if ( cg.wstatsRequestTime + 2000 < cg.time ) {
		// the stats are more than two seconds out of data,
		// so request new ones
		cg.wstatsRequestTime = cg.time;
		trap_SendClientCommand( "wstats" );
	}

	if (!cg.showWStats) 
  {
    cg.wstatsStartTime = cg.time;
    cg.showWStats = qtrue;
  }
}


/*
=================================
CG_WeaponStatsDown_f
Turns off the players weapon stats
=================================
*/
static void CG_WeaponStatsDown_f(void)
{
	if (cg.showWStats)
  {
    cg.showWStats = qfalse;
  }
}


#ifdef MISSIONPACK
extern menuDef_t *menuScoreboard;
void Menu_Reset();			// FIXME: add to right include file

static void CG_LoadHud_f( void) {
  char buff[1024];
	const char *hudSet;
  memset(buff, 0, sizeof(buff));

	String_Init();
	Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
  menuScoreboard = NULL;
}


static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}


static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	if (cg_RQ3_anouncer.integer == 1) trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", SCREEN_HEIGHT * .30, 0);
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	if (cg_RQ3_anouncer.integer == 1) trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", SCREEN_HEIGHT * .30, 0);
}

#endif

static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

#ifdef MISSIONPACK
static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF) {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_guantlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}

/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/

#endif

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/

/*
==================
CG_Say_f

Elder: Pre-validate say command to avoid text flooding
==================
*/

static void CG_Say_f ( void ) {

	if (cg.time - cg.sayTime > 2000)
		cg.sayCount = 0;

	if (cg.sayCount == 0)
		cg.sayTime = cg.time;
	
	cg.sayCount++;

	// CG_Printf("sayCount: %i sayTime: %i\n", cg.sayCount, cg.sayTime);

	if (cg.sayCount > 4 && cg.time - cg.sayTime < 2000)
	{
		CG_Printf("Cannot send messages.\n");
		return;
	}

	trap_SendClientCommand("say");
}


/*
==================
CG_SayTeam_f

Elder: Pre-validate say_team command to avoid text flooding
==================
*/

static void CG_SayTeam_f ( void ) {

	if (cg.time - cg.sayTime > 2000)
		cg.sayCount = 0;

	if (cg.sayCount == 0)
		cg.sayTime = cg.time;
	
	cg.sayCount++;

	// CG_Printf("sayCount: %i sayTime: %i\n", cg.sayCount, cg.sayTime);

	if (cg.sayCount > 4 && cg.time - cg.sayTime < 2000)
	{
		CG_Printf("Cannot send messages.\n");
		return;
	}
	
	trap_SendClientCommand("say_team");
}

/*
==================
CG_IRVision_f

Elder: Attempt to turn on IR Vision
==================
*/

static void CG_IRVision_f ( void ) {

	if (bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_BANDOLIER)
	{
		if (cg.rq3_irvision)
		{
			CG_Printf("IR vision disabled.\n");
			cg.rq3_irvision = qfalse;
		}
		else
		{
			CG_Printf("IR vision enabled.\n");
			cg.rq3_irvision = qtrue;
		}
		
		trap_S_StartLocalSound(cgs.media.weapToggleSound, CHAN_ITEM);
	}
	else
	{
		CG_Printf("You do not have IR goggles.\n");
	}

}


typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "playerorigin", CG_PlayerOrigin_f },
	//{ "+scores", CG_ScoresDown_f },
	{ "scores", CG_Scores_f },
/*	{ "+zoom", CG_ZoomDown_f },				// hawkins not needed in Reaction
	{ "-zoom", CG_ZoomUp_f },*/			
	//Blaze: Weapon stats
	{ "+wstats", CG_WeaponStatsUp_f },
	{ "-wstats", CG_WeaponStatsDown_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },				// Elder: it's for RQ3 and Q3A
// JBravo: adding the drop command and unregistering the other two
//	{ "dropitem", CG_DropItem_f },
//	{ "dropweapon", CG_DropWeapon_f },		// Elder: added to reset zoom then goto server
	{ "bandage", CG_Bandage_f },			// Elder: added to reset zoom then goto server
	{ "specialweapon", CG_SpecialWeapon_f },	// Elder: select special weapon
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },

#ifdef MISSIONPACK
	{ "loadhud", CG_LoadHud_f },
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
#endif
	{ "startOrbit", CG_StartOrbit_f },
	//{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "irvision", CG_IRVision_f }
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
	trap_AddCommand ("vsay");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vtaunt");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("team");
	trap_AddCommand ("follow");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("callvote");
	trap_AddCommand ("vote");
	trap_AddCommand ("callteamvote");
	trap_AddCommand ("teamvote");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo
 	trap_AddCommand ("opendoor");
 	trap_AddCommand ("bandage");
	//trap_AddCommand ("drop");	// XRAY FMJ weap drop cmd - Elder: not used
	//Elder: added to give drop weapon auto-complete
// JBravo: no need for tab completion for those two
//	trap_AddCommand ("dropweapon");
//	trap_AddCommand ("dropitem");
	//Blaze: to get weapon stats
	trap_AddCommand ("playerstats");
	//Elder: try this
	trap_AddCommand ("weapon");
	trap_AddCommand ("specialweapon");
	//trap_AddCommand ("messagemode");
	//trap_AddCommand ("messagemode2");
	trap_AddCommand ("playerorigin");
	trap_AddCommand ("irvision");
// JBravo: adding choose and drop commands.
	trap_AddCommand ("choose");
	trap_AddCommand ("drop");
// JBravo: for zcam
#ifdef __ZCAM__
	trap_AddCommand ("camera");
#endif
// JBravo: for radio
	trap_AddCommand ("radio");
	trap_AddCommand ("radiogender");
	trap_AddCommand ("radio_power");
// Slicer: Matchmode
	trap_AddCommand ("captain");
	trap_AddCommand ("ready");
	trap_AddCommand ("sub");
// aasimon: refeere mm
	trap_AddCommand ("reflogin");
	trap_AddCommand ("ref");
	trap_AddCommand ("refresign");
// JBravo: debugging cmd
	trap_AddCommand ("debugshit");
}
