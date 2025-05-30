/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 




#include "bmpman/bmpman.h"
#include "freespace.h"
#include "controlconfig/controlsconfig.h"
#include "gamesequence/gamesequence.h"
#include "gamesnd/eventmusic.h"
#include "gamesnd/gamesnd.h"
#include "globalincs/alphacolors.h"
#include "io/joy.h"
#include "io/key.h"
#include "io/mouse.h"
#include "io/timer.h"
#include "menuui/mainhallmenu.h"
#include "menuui/optionsmenu.h"
#include "menuui/optionsmenumulti.h"
#include "mission/missionbriefcommon.h"
#include "missionui/missiondebrief.h"
#include "missionui/missionscreencommon.h"
#include "nebula/neb.h"
#include "network/multi.h"
#include "options/OptionsManager.h"
#include "options/Option.h"
#include "osapi/osregistry.h"
#include "pilotfile/pilotfile.h"
#include "popup/popup.h"
#include "popup/popupdead.h"
#include "scripting/global_hooks.h"
#include "sound/audiostr.h"
#include "weapon/weapon.h"



// will display a notification warning message
#define OPTIONS_NOTIFY_TIME			3500
#define OPTIONS_NOTIFY_Y            450

#define NUM_BUTTONS	24
#define NUM_ANIS		4
#define NUM_TABS		3
#define NUM_COMMONS	10

#define TABLESS							999

#define OPTIONS_TAB						0
#define MULTIPLAYER_TAB					1
#define DETAIL_LEVELS_TAB 				2
#define ABORT_GAME_BUTTON				3
#define CONTROL_CONFIG_BUTTON			4
#define HUD_CONFIG_BUTTON				5
#define ACCEPT_BUTTON					6

#define BRIEF_VOICE_OFF					7
#define BRIEF_VOICE_ON					8
#define MOUSE_OFF							9
#define MOUSE_ON							10
#define GAMMA_DOWN						11
#define GAMMA_UP							12

// detail level screen buttons
#define PLANETS_ON						13
#define PLANETS_OFF						14
#define HUD_TARGETVIEW_RENDER_ON		15
#define HUD_TARGETVIEW_RENDER_OFF	16
#define WEAPON_EXTRAS_ON				17
#define WEAPON_EXTRAS_OFF				18

#define LOW_DETAIL_N						19
#define MEDIUM_DETAIL_N					20
#define HIGH_DETAIL_N					21
#define VERY_HIGH_DETAIL_N				22
#define CUSTOM_DETAIL_N					23

#define REPEAT						(1<<0)
#define NO_MOUSE_OVER_SOUND	(1<<1)

// indicies for options coordinates
#define OPTIONS_X_COORD 0
#define OPTIONS_Y_COORD 1
#define OPTIONS_W_COORD 2
#define OPTIONS_H_COORD 3

struct options_buttons {
	const char *filename;
	int x, y;
	int hotspot;
	int tab;
	int flags;
	UI_BUTTON button;  // because we have a class inside this struct, we need the constructor below..

	options_buttons(const char *name, int x1, int y1, int h, int t, int f = 0) : filename(name), x(x1), y(y1), hotspot(h), tab(t), flags(f) {}
};

static options_buttons Buttons[GR_NUM_RESOLUTIONS][NUM_BUTTONS] = {
	{	// GR_640
		options_buttons("OPT_00",	17,	2,		0,		-1),							// options tab
		options_buttons("OPT_01",	102,	2,		1,		-1),							// multiplayer tab
		options_buttons("OPT_02",	170,	2,		2,		-1),							// detail levels tab
		options_buttons("OPT_03",	10,	444,	3,		-1),							// abort game button
		options_buttons("OPT_04",	411,	444,	4,		-1),							// control config button
		options_buttons("OPT_05",	506,	444,	5,		-1),							// hud config
		options_buttons("OPT_06",	576,	434,	6,		-1),							// accept button				

		options_buttons("OMB_07",	51,	74,	7,		OPTIONS_TAB, 2),			// Briefing / debriefing voice toggle off
		options_buttons("OMB_08",	106,	74,	8,		OPTIONS_TAB, 2),			// Briefing / debriefing voice toggle on
		options_buttons("OMB_18",	51,	266,	18,	OPTIONS_TAB, 2),			// Mouse off
		options_buttons("OMB_19",	106,	266,	19,	OPTIONS_TAB, 2),			// Mouse on
		options_buttons("OMB_26",	578,	149,	26,	OPTIONS_TAB, 1),			// Gamma Down
		options_buttons("OMB_27",	607,	149,	27,	OPTIONS_TAB, 1),			// Gamma Up
		
		options_buttons("ODB_21",	597,	261,	21,	DETAIL_LEVELS_TAB, 2),	// Planets On
		options_buttons("ODB_20",	539,	261,	20,	DETAIL_LEVELS_TAB, 2),	// Planets Off
		options_buttons("ODB_23",	597,	307,	23,	DETAIL_LEVELS_TAB, 2),	// Target View Rendering On
		options_buttons("ODB_22",	539,	307,	22,	DETAIL_LEVELS_TAB, 2),	// Target View Rendering Off
		options_buttons("ODB_25",	597,	354,	25,	DETAIL_LEVELS_TAB, 2),	// Weapon Extras On
		options_buttons("ODB_24",	539,	354,	24,	DETAIL_LEVELS_TAB, 2),	// Weapon Extras Off

		options_buttons("ODB_14",	614,	76,	14,	DETAIL_LEVELS_TAB, 2),	// Low Preset Detail
		options_buttons("ODB_15",	614,	96,	15,	DETAIL_LEVELS_TAB, 2),	// Medium Preset Detail
		options_buttons("ODB_16",	614,	114,	16,	DETAIL_LEVELS_TAB, 2),	// High Preset Detail
		options_buttons("ODB_17",	614,	133,	17,	DETAIL_LEVELS_TAB, 2),	// Highest Preset Detail
		options_buttons("ODB_18",	614,	152,	18,	DETAIL_LEVELS_TAB, 2),	// Custom Detail
	},
	{	// GR_1024
		options_buttons("2_OPT_00",	27,	4,		0,		-1),						// options tab
		options_buttons("2_OPT_01",	164,	4,		1,		-1),						// multiplayer tab
		options_buttons("2_OPT_02",	272,	4,		2,		-1),						// detail levels tab
		options_buttons("2_OPT_03",	16,	711,	3,		-1),						// abort game
		options_buttons("2_OPT_04",	657,	711,	4,		-1),						// control config button
		options_buttons("2_OPT_05",	809,	711,	5,		-1),						// hud config button
		options_buttons("2_OPT_06",	922,	694,	6,		-1),						// accept button		

		options_buttons("2_OMB_07",	81,	118,	7,		OPTIONS_TAB, 2),			// Briefing / debriefing voice toggle off
		options_buttons("2_OMB_08",	170,	118,	8,		OPTIONS_TAB, 2),			// Briefing / debriefing voice toggle on
		options_buttons("2_OMB_18",	81,	425,	18,	OPTIONS_TAB, 2),			// Mouse off
		options_buttons("2_OMB_19",	170,	425,	19,	OPTIONS_TAB, 2),			// Mouse on
		options_buttons("2_OMB_26",	925,	238,	26,	OPTIONS_TAB, 1),			// Gamma Down
		options_buttons("2_OMB_27",	971,	238,	27,	OPTIONS_TAB, 1),			// Gamma Up
		
		options_buttons("2_ODB_21",	956,	417,	21,	DETAIL_LEVELS_TAB, 2),	// Planets On
		options_buttons("2_ODB_20",	863,	417,	20,	DETAIL_LEVELS_TAB, 2),	// Planets Off
		options_buttons("2_ODB_23",	956,	492,	23,	DETAIL_LEVELS_TAB, 2),	// Target View Rendering On
		options_buttons("2_ODB_22",	863,	492,	22,	DETAIL_LEVELS_TAB, 2),	// Target View Rendering Off
		options_buttons("2_ODB_25",	956,	567,	25,	DETAIL_LEVELS_TAB, 2),	// Weapon Extras On
		options_buttons("2_ODB_24",	863,	567,	24,	DETAIL_LEVELS_TAB, 2),	// Weapon Extras Off

		options_buttons("2_ODB_14",	983,	122,	14,	DETAIL_LEVELS_TAB, 2),	// Low Preset Detail
		options_buttons("2_ODB_15",	983,	153,	15,	DETAIL_LEVELS_TAB, 2),	// Medium Preset Detail
		options_buttons("2_ODB_16",	983,	183,	16,	DETAIL_LEVELS_TAB, 2),	// High Preset Detail
		options_buttons("2_ODB_17",	983,	213,	17,	DETAIL_LEVELS_TAB, 2),	// Highest Preset Detail
		options_buttons("2_ODB_18",	983,	243,	18,	DETAIL_LEVELS_TAB, 2),	// Custom Detail
	}	
};

#define NUM_OPTIONS_SLIDERS			7
#define OPT_SOUND_VOLUME_SLIDER		0
#define OPT_MUSIC_VOLUME_SLIDER		1
#define OPT_VOICE_VOLUME_SLIDER		2
#define OPT_MOUSE_SENS_SLIDER			3
#define OPT_JOY_SENS_SLIDER			4
#define OPT_JOY_DEADZONE_SLIDER		5
#define OPT_SKILL_SLIDER				6

op_sliders Options_sliders[GR_NUM_RESOLUTIONS][NUM_OPTIONS_SLIDERS] = {
	{ // GR_640		
		op_sliders("OMB_10",		31,	139,	-1,	-1,	10,	20,	10,
					  "OMB_11",		11,	226,	137,
					  "OMB_09",		9,		4,		137 ),								// sound fx volume slider
		op_sliders("OMB_13",		31,	174,	-1,	-1,	13,	20,	10,
					  "OMB_14",		14,	226,	172,
					  "OMB_12",		12,	4,		172 ),								// music volume slider
		op_sliders("OMB_16",		31,	209,	-1,	-1,	16,	20,	10,
					  "OMB_17",		17,	226,	206,
					  "OMB_15",		15,	4,		206 ),								// voice volume slider
		op_sliders("OMB_20",		6,		316,	-1,	-1,	20,	20,	10, NULL, -1, -1, -1, NULL, -1, -1, -1),	// mouse sensitivity	
		op_sliders("OMB_28",		440,	259,	-1,	-1,	28,	20,	10, NULL, -1, -1, -1, NULL, -1, -1, -1),	// joystick sensitivity
		op_sliders("OMB_29",		440,	290,	-1,	-1,	29,	20,	10, NULL, -1, -1, -1, NULL, -1, -1, -1),	// joystick deadzone
		op_sliders("OMB_21",		440,	75,	-1,	-1,	21,	36,	5,	NULL, -1, -1, -1, NULL, -1, -1, -1)
	},	
	{ // GR_1024		
		op_sliders("2_OMB_10",		50,	223,	-1,	-1,	10,	32,	10,
					  "2_OMB_11",		11,	361,	219,
					  "2_OMB_09",		9,		7,		219 ),								// sound fx volume slider
		op_sliders("2_OMB_13",		50,	279,	-1,	-1,	13,	32,	10,
					  "2_OMB_14",		14,	361,	275,
					  "2_OMB_12",		12,	7,		275 ),								// music volume slider
		op_sliders("2_OMB_16",		50,	335,	-1,	-1,	16,	32,	10,
					  "2_OMB_17",		17,	361,	330,
					  "2_OMB_15",		15,	7,		330 ),								// voice volume slider
		op_sliders("2_OMB_20",		9,		505,	-1,	-1,	20,	32,	10, NULL, -1, -1, -1, NULL, -1, -1, -1),	// mouse sensitivity	
		op_sliders("2_OMB_28",		704,	414,	-1,	-1,	28,	32,	10, NULL, -1, -1, -1, NULL, -1, -1, -1),	// joystick sensitivity
		op_sliders("2_OMB_29",		704,	464,	-1,	-1,	29,	32,	10, NULL, -1, -1, -1, NULL, -1, -1, -1),	// joystick deadzone
		op_sliders("2_OMB_21",		704,	120,	-1,	-1,	21,	60,	5,	NULL, -1, -1, -1, NULL, -1, -1, -1)
	}
};

static struct {
	const char *filename;
	const char *mask_filename;
	int bitmap;
	int mask;
	
} Backgrounds[GR_NUM_RESOLUTIONS][NUM_TABS] = {
//XSTR:OFF
	{	// GR_640
		{ "OptionsMain", "OptionsMain-M", -1, -1},
		{ "OptionsMulti", "OptionsMulti-M", -1, -1},
		{ "OptionsDetail", "OptionsDetail-M", -1, -1},
	},
	{	// GR_1024
		{ "2_OptionsMain", "2_OptionsMain-M", -1, -1},
		{ "2_OptionsMulti", "2_OptionsMulti-M", -1, -1},
		{ "2_OptionsDetail", "2_OptionsDetail-M", -1, -1},
	}
//XSTR:ON
};

static int Tab = 0;
static int Options_menu_inited = 0;
static int Options_multi_inited = 0;
static int Options_detail_inited = 0;
static int Button_bms[NUM_COMMONS][MAX_BMAPS_PER_GADGET];

static UI_WINDOW Ui_window;
UI_GADGET Options_bogus;

static int Backup_skill_level;
static float Backup_sound_volume;
static float Backup_music_volume;
static float Backup_voice_volume;

static int Backup_mouse_sensitivity;
static int Backup_joy_sensitivity;
static int Backup_joy_deadzone;
static float Backup_gamma;

static bool Backup_briefing_voice_enabled;
static bool Backup_use_mouse_to_fly;

static int Sound_volume_int;
static int Music_volume_int;
static int Voice_volume_int;

static sound_handle Voice_vol_handle = sound_handle::invalid();
UI_TIMESTAMP Options_notify_stamp;
char Options_notify_string[200];

// Called whenever the options menu state is accepted, either via 
// clicking the accept button or when navigating to the Controls or HUD screen.
// The reasoning is that clicking a button means 'accept and move on' if it leads to a new state.
// This behavior has always been the case for the main options and detail tabs, 
// and now it is the same for the multi tab, too. --wookieejedi 
void options_accept();
void options_force_button_frame(int n, int frame_num);

void options_add_notify(const char *str);
void options_notify_do_frame();

int Options_gamma_coords[GR_NUM_RESOLUTIONS][4] = {
	{
		435, 179, 195,	28	// GR_640
	},
	{
		692, 287, 308, 44		// GR_1024
	}
};

#define MAX_GAMMA_BITMAP_SIZE 17500

int Options_gamma_num_coords[GR_NUM_RESOLUTIONS][4] = {
	{
		489, 159, 65, 17		// GR_640
	},
	{
		779, 254, 65, 17		// GR_1024
	}
};

int Options_skills_text_coords[GR_NUM_RESOLUTIONS][4] = {
	{
		468, 104, 155, 10		// GR_640
	},
	{
		750, 169, 246, 21		// GR_1024
	}
};

int Options_scp_string_coords[GR_NUM_RESOLUTIONS][2] = {
	{
		265, 18		// GR_640
	},
	{
		465, 25		// GR_1024
	}
};

std::pair<SCP_string, int> Options_scp_string_text = {"Press F3 to access additional options", 1831};


// ---------------------------------------------------------------------------------------------------------
// DETAIL LEVEL OPTIONS definitions  BEGIN
//

#define NUM_DETAIL_SLIDERS			8

/*
#define DETAIL_DISTANCE_SLIDER	0
#define NEBULA_DETAIL_SLIDER		1
#define HARDWARE_TEXTURES_SLIDER	2
#define NUM_PARTICLES_SLIDER		6
#define SHARD_CULLING_SLIDER		3
#define SHIELD_DETAIL_SLIDER		4
#define NUM_STARS_SLIDER			5
#define LIGHTING_SLIDER				7
*/
#define DETAIL_DISTANCE_SLIDER	0
#define NEBULA_DETAIL_SLIDER		1
#define HARDWARE_TEXTURES_SLIDER	2
#define NUM_PARTICLES_SLIDER		3
#define SHARD_CULLING_SLIDER		4
#define SHIELD_DETAIL_SLIDER		5
#define NUM_STARS_SLIDER			6
#define LIGHTING_SLIDER				7
op_sliders Detail_sliders[GR_NUM_RESOLUTIONS][NUM_DETAIL_SLIDERS] = {
	{ // GR_640
		op_sliders("ODB_07",	21,	71,	-1,	-1,	7,		20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// model detail
		op_sliders("ODB_08",	21,	119,	-1,	-1,	8,		20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// nebula detail
		op_sliders("ODB_09",	21,	166,	-1,	-1,	9,		20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// textures
		op_sliders("ODB_10",	21,	212,	-1,	-1,	10,	20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// particles
		op_sliders("ODB_11",	21,	260,	-1,	-1,	11,	20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// debris
		op_sliders("ODB_12",	21,	307,	-1,	-1,	12,	20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// shield hit
		op_sliders("ODB_13",	21,	354,	-1,	-1,	13,	20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// stars
		op_sliders("ODB_19",	518,	212,	-1,	-1,	19,	20,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),			// lighting		
	},	
	{ // GR_1024
		op_sliders("2_ODB_07",	34,	114,	-1,	-1,	7,		32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// model detail
		op_sliders("2_ODB_08",	34,	190,	-1,	-1,	8,		32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// nebula detail
		op_sliders("2_ODB_09",	34,	265,	-1,	-1,	9,		32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// textures
		op_sliders("2_ODB_10",	34,	340,	-1,	-1,	10,	32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// particles
		op_sliders("2_ODB_11",	34,	416,	-1,	-1,	11,	32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// debris
		op_sliders("2_ODB_12",	34,	492,	-1,	-1,	12,	32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// shield hit
		op_sliders("2_ODB_13",	34,	567,	-1,	-1,	13,	32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// stars
		op_sliders("2_ODB_19",	829,	340,	-1,	-1,	19,	32,	5, NULL, -1, -1, -1, NULL, -1, -1, -1),		// lighting
	}
};
int Detail_slider_pos[NUM_DETAIL_SLIDERS];
detail_levels Detail_original;	// backup of Detail settings when screen is first entered
UI_GADGET Detail_bogus;
void options_detail_init();
void options_detail_hide_stuff();
void options_detail_unhide_stuff();
void options_detail_do_frame();
void options_detail_set_level(DefaultDetailPreset preset);

// text
#define OPTIONS_NUM_TEXT				49
UI_XSTR Options_text[GR_NUM_RESOLUTIONS][OPTIONS_NUM_TEXT] = {
	{ // GR_640
		// common text
		{ "Options",	1036,		10,	35,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][OPTIONS_TAB].button },
		{ "Multi",		1042,		97,	35,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][MULTIPLAYER_TAB].button },
		{ "Detail",		1351,		166,	35,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][DETAIL_LEVELS_TAB].button },
		{ "Exit",		1059,		8,		417,	UI_XSTR_COLOR_PINK,	-1, &Buttons[0][ABORT_GAME_BUTTON].button },
		{ "Game",		1412,		8,		430,	UI_XSTR_COLOR_PINK,	-1, &Buttons[0][ABORT_GAME_BUTTON].button },
		{ "Control",	1352,		409,	418,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][CONTROL_CONFIG_BUTTON].button },
		{ "Config",		1353,		409,	430,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][CONTROL_CONFIG_BUTTON].button },
		{ "HUD",			1354,		504,	418,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][HUD_CONFIG_BUTTON].button },
		{ "Config",		1415,		504,	430,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][HUD_CONFIG_BUTTON].button },
		{ "Accept",		1035,		573,	412,	UI_XSTR_COLOR_PINK,	-1, &Buttons[0][ACCEPT_BUTTON].button },

		// text for the detail level screen 
		{ "Preset Detail Levels",	1355,	455,	56,	UI_XSTR_COLOR_GREEN,	-1, &Detail_bogus },
		{ "Low",			1160,		570,	82,	UI_XSTR_COLOR_GREEN, -1, &Buttons[0][LOW_DETAIL_N].button },
		{ "Medium",		1161,		550,	100,	UI_XSTR_COLOR_GREEN, -1, &Buttons[0][MEDIUM_DETAIL_N].button },
		{ "High",		1162,		568,	120,	UI_XSTR_COLOR_GREEN, -1, &Buttons[0][HIGH_DETAIL_N].button },
		{ "Very High",	1163,		530,	139,	UI_XSTR_COLOR_GREEN, -1, &Buttons[0][VERY_HIGH_DETAIL_N].button },
		{ "Custom",		1356,		546,	158,	UI_XSTR_COLOR_GREEN, -1, &Buttons[0][CUSTOM_DETAIL_N].button },		
		{ "Off",			1286,		509,	267,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][PLANETS_OFF].button },
		{ "On",			1285,		573,	267,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][PLANETS_ON].button },
		{ "Off",			1286,		509,	314,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][HUD_TARGETVIEW_RENDER_OFF].button },
		{ "On",			1285,		573,	314,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][HUD_TARGETVIEW_RENDER_ON].button },
		{ "Off",			1286,		509,	361,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][WEAPON_EXTRAS_OFF].button },
		{ "On",			1285,		573,	361,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][WEAPON_EXTRAS_ON].button },
		{ "Planets/Backgrounds",		1357,	455,	244, UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Target View Rendering",		1358,	446,	291, UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Weapon Extras",				1359,	497,	338, UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Model Detail",					1360,	27,	56,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Nebula Detail",				1361,	27,	103,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "3D Hardware Textures",		1362,	27,	150,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Particles",						1363,	27,	197,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Impact Effects",				1364,	27,	244,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Shield Hit Effects",			1365,	27,	291,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Stars",							1366,	27,	338,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Lighting",						1367, 549,	197,	UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },

		// main options screen text
		{ "Briefing Voice",	1368,	14,	58,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Off",					1286,	20,	81,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][BRIEF_VOICE_OFF].button },
		{ "On",					1285,	83,	81,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][BRIEF_VOICE_ON].button },
		{ "Volume",				1369,	14,	111,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Effects",			1370,	20,	130,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Music",				1371,	20,	165,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Voice",				1372,	20,	199,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Mouse Input Mode",	1665,	10,	249,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Mouse",				1373,	10,	273,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][MOUSE_OFF].button },
		{ "Joy-0",				1666,	70,	273,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[0][MOUSE_ON].button },
		{ "Sensitivity",		1529,	20,	297,	UI_XSTR_COLOR_GREEN,	-1, &Options_sliders[0][OPT_MOUSE_SENS_SLIDER].slider },
		{ "Skill Level",		1509,	533,	58,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Brightness",		1375,	532,	133,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Joystick",			1376,	556,	231,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Sensitivity",		1374,	538,	250,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Deadzone",			1377,	538,	281,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
	},
	{ // GR_1024
			// common text
		{ "Options",	1036,		16,	57,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][OPTIONS_TAB].button },
		{ "Multi",		1042,		172,	57,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][MULTIPLAYER_TAB].button },
		{ "Detail",		1351,		283,	57,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][DETAIL_LEVELS_TAB].button },
		{ "Exit",		1059,		13,	685,	UI_XSTR_COLOR_PINK,	-1, &Buttons[1][ABORT_GAME_BUTTON].button },
		{ "Game",		1412,		13,	696,	UI_XSTR_COLOR_PINK,	-1, &Buttons[1][ABORT_GAME_BUTTON].button },
		{ "Control",	1352,		655,	685,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][CONTROL_CONFIG_BUTTON].button },
		{ "Config",		1353,		655,	696,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][CONTROL_CONFIG_BUTTON].button },
		{ "HUD",			1354,		806,	685,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][HUD_CONFIG_BUTTON].button },
		{ "Config",		1415,		806,	696,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][HUD_CONFIG_BUTTON].button },
		{ "Accept",		1035,		927,	672,	UI_XSTR_COLOR_PINK,	-1, &Buttons[1][ACCEPT_BUTTON].button },

		// text for the detail level screen 
		{ "Preset Detail Levels",	1355,	809,	90,	UI_XSTR_COLOR_GREEN,	-1, &Detail_bogus },
		{ "Low",			1160,		944,	131,	UI_XSTR_COLOR_GREEN, -1, &Buttons[1][LOW_DETAIL_N].button },
		{ "Medium",		1161,		924,	161,	UI_XSTR_COLOR_GREEN, -1, &Buttons[1][MEDIUM_DETAIL_N].button },
		{ "High",		1162,		942,	192,	UI_XSTR_COLOR_GREEN, -1, &Buttons[1][HIGH_DETAIL_N].button },
		{ "Very High",	1163,		903,	222,	UI_XSTR_COLOR_GREEN, -1, &Buttons[1][VERY_HIGH_DETAIL_N].button },
		{ "Custom",		1356,		922,	252,	UI_XSTR_COLOR_GREEN, -1, &Buttons[1][CUSTOM_DETAIL_N].button },		
		{ "Off",			1286,		835,	427,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][PLANETS_OFF].button },
		{ "On",			1285,		936,	427,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][PLANETS_ON].button },
		{ "Off",			1286,		835,	503,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][HUD_TARGETVIEW_RENDER_OFF].button },
		{ "On",			1285,		936,	503,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][HUD_TARGETVIEW_RENDER_ON].button },
		{ "Off",			1286,		835,	578,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][WEAPON_EXTRAS_OFF].button },
		{ "On",			1285,		936,	578,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][WEAPON_EXTRAS_ON].button },
		{ "Planets/Backgrounds",	1357,	808,	391, UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Target View Rendering",	1358,	799,	466, UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },
		{ "Weapon Extras",			1359,	850,	542, UI_XSTR_COLOR_GREEN, -1, &Detail_bogus },				
		{ "Model Detail",					1360,	44,	99,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][DETAIL_DISTANCE_SLIDER].slider },
		{ "Nebula Detail",				1361,	44,	175,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][NEBULA_DETAIL_SLIDER].slider },
		{ "3D Hardware Textures",		1362,	44,	250,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][HARDWARE_TEXTURES_SLIDER].slider },
		{ "Particles",						1363,	44,	325,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][NUM_PARTICLES_SLIDER].slider },
		{ "Impact Effects",				1364,	44,	401,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][SHARD_CULLING_SLIDER].slider },
		{ "Shield Hit Effects",			1365,	44,	476,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][SHIELD_DETAIL_SLIDER].slider },
		{ "Stars",							1366, 44,	552,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][NUM_STARS_SLIDER].slider },
		{ "Lighting",						1367, 903,	326,	UI_XSTR_COLOR_GREEN, -1, &Detail_sliders[1][LIGHTING_SLIDER].slider },

		// main options screen text
		{ "Briefing Voice",	1368,	23,	93,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Off",					1286,	32,	130,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][BRIEF_VOICE_OFF].button },
		{ "On",					1285,	134,	130,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][BRIEF_VOICE_ON].button },
		{ "Volume",				1369,	23,	178,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Effects",			1370,	33,	209,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Music",				1371,	33,	264,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Voice",				1372,	33,	319,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Mouse Input Mode",	1665,	22,		399,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Mouse",				1373,	22,		437,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][MOUSE_OFF].button },
		{ "Joy-0",				1666,	124,	437,	UI_XSTR_COLOR_GREEN,	-1, &Buttons[1][MOUSE_ON].button },
		{ "Sensitivity",		1529,	34,	477,	UI_XSTR_COLOR_GREEN,	-1, &Options_sliders[1][OPT_MOUSE_SENS_SLIDER].slider },
		{ "Skill Level",		1509,	854,	93,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Brightness",		1375,	852,	214,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Joystick",			1376,	891,	370,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Sensitivity",		1374,	861,	400,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
		{ "Deadzone",			1377,	861,	451,	UI_XSTR_COLOR_GREEN,	-1, &Options_bogus },
	}
};


//
// DETAIL LEVEL tab definitions END
// ---------------------------------------------------------------------------------------------------------

void options_play_voice_clip()
{
	if ( snd_is_playing(Voice_vol_handle) ) {
		snd_stop(Voice_vol_handle);
		Voice_vol_handle = sound_handle::invalid();
	}
	auto gs = gamesnd_get_interface_sound(InterfaceSounds::VOICE_SLIDER_CLIP);
	auto entry = gamesnd_choose_entry(gs);

	auto snd_id = snd_load(entry, &gs->flags, 0);

	Voice_vol_handle = snd_play_raw( snd_id, 0.0f, 1.0f, SND_PRIORITY_SINGLE_INSTANCE );
}

void options_add_notify(const char *str)
{
	strcpy_s(Options_notify_string, str);
	Options_notify_stamp = ui_timestamp(OPTIONS_NOTIFY_TIME);
}

void options_notify_do_frame()
{
	int w,h;

	if (Options_notify_stamp.isValid()) {
		if (ui_timestamp_elapsed(Options_notify_stamp)) {
			Options_notify_stamp = UI_TIMESTAMP::invalid();

		} else {
			gr_get_string_size(&w, &h, Options_notify_string);
			gr_printf_menu((gr_screen.max_w_unscaled - w) / 2, OPTIONS_NOTIFY_Y, "%s", Options_notify_string);
		}
	}
}

/*
void options_set_bmaps(int btn, int bm_index)
{
	int j;

	for (j=0; j<MAX_BMAPS_PER_GADGET; j++){
		Buttons[gr_screen.res][btn].button.bmap_ids[j] = Button_bms[bm_index][j];
	}
}
*/

void options_tab_setup(int  /*set_palette*/)
{
	// char *pal;
	int i;
	int flags[256];

	if (Tab != MULTIPLAYER_TAB) {
		Assert(Backgrounds[gr_screen.res][Tab].mask >= 0);
		Ui_window.set_mask_bmap(Backgrounds[gr_screen.res][Tab].mask, Backgrounds[gr_screen.res][Tab].mask_filename);
	}

	for (i=0; i<256; i++){
		flags[i] = 0;
	}

	// activate, deactivate any necessary controls
	for (i=0; i<NUM_BUTTONS; i++) {
		if ( ((Buttons[gr_screen.res][i].tab == Tab) || (Buttons[gr_screen.res][i].tab == -1)) && !flags[Buttons[gr_screen.res][i].hotspot] ) {
			flags[Buttons[gr_screen.res][i].hotspot] = 1;
			Buttons[gr_screen.res][i].button.enable();
			if (Buttons[gr_screen.res][i].filename)
				Buttons[gr_screen.res][i].button.unhide();

		} else {
			Buttons[gr_screen.res][i].button.disable();
			Buttons[gr_screen.res][i].button.hide();
		}
	}

	// maybe enable/disable controls based upon current tab
	if (Tab == OPTIONS_TAB) {
		for(i=0; i<NUM_OPTIONS_SLIDERS; i++) {
			Options_sliders[gr_screen.res][i].slider.enable();
			Options_sliders[gr_screen.res][i].slider.unhide();
		}		
		if (Cmdline_deadzone >= 0) {
			//Deadzone is being set by the command line 
			Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.disable();
			Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.hide();
		}
	} else {
		for(i=0; i<NUM_OPTIONS_SLIDERS; i++) {
			Options_sliders[gr_screen.res][i].slider.hide();
			Options_sliders[gr_screen.res][i].slider.disable();
		}		
	}	

	if( ((Game_mode & GM_IN_MISSION) && (!popupdead_is_active())) || (Game_mode & GM_MULTIPLAYER) ){
		Options_sliders[gr_screen.res][OPT_SKILL_SLIDER].slider.disable();
		Ui_window.use_hack_to_get_around_stupid_problem_flag = 0;
	}


	// do other special processing
	switch (Tab) {
		case MULTIPLAYER_TAB:
			options_multi_select();
			break;

		case DETAIL_LEVELS_TAB:
			options_detail_unhide_stuff();	
			break;
	}
}

// call this function to close down, do other processing of data in the tab that's being left
void options_tab_close()
{
	switch (Tab) {
		case MULTIPLAYER_TAB:
			options_multi_unselect();		
			break;

		case DETAIL_LEVELS_TAB:
			options_detail_hide_stuff();
			break;
	}
}

void options_change_tab(int n)
{
	int idx;

	switch (n) {
		case MULTIPLAYER_TAB:
			if ( !Options_multi_inited ) {
				// init multiplayer
				options_multi_init(&Ui_window);
				options_multi_unselect();
				Options_multi_inited = 1;
			}

			break;

		case DETAIL_LEVELS_TAB:
			if (!Options_detail_inited) {
				// init detail levels
				options_detail_init();
				options_detail_hide_stuff();
				Options_detail_inited = 1;
			}

			break;
	}

	// if we're going into the main screen
	if(n == OPTIONS_TAB){
		Options_bogus.enable();
		Options_bogus.unhide();
		for(idx=0; idx<NUM_OPTIONS_SLIDERS; idx++){
			Options_sliders[gr_screen.res][idx].slider.enable();
			Options_sliders[gr_screen.res][idx].slider.unhide();
		}
		if (Cmdline_deadzone >= 0){
			//Deadzone is being set by the command line 
			Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.disable();
			Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.hide();
		}
	} else {
		Options_bogus.hide();
		Options_bogus.disable();
		for(idx=0; idx<NUM_OPTIONS_SLIDERS; idx++){
			Options_sliders[gr_screen.res][idx].slider.hide();
			Options_sliders[gr_screen.res][idx].slider.disable();
		}
	}

	if (n != MULTIPLAYER_TAB) {
		if (Backgrounds[gr_screen.res][n].mask < 0) {
			gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);
			return;
		}
	}

	options_tab_close();

	Tab = n;
	options_tab_setup(1);
	gamesnd_play_iface(InterfaceSounds::SCREEN_MODE_PRESSED);

	// adds scripting hook for 'On Options Tab Changed' --wookieejedi
	if (scripting::hooks::OnOptionsTabChanged->isActive()) {
		scripting::hooks::OnOptionsTabChanged->run(scripting::hook_param_list(scripting::hook_param("TabNumber", 'i', Tab)));
	}
}

void options_cancel_exit()
{
	snd_set_effects_volume(Backup_sound_volume);
	event_music_set_volume(Backup_music_volume);
	snd_set_voice_volume(Backup_voice_volume);

	Mouse_sensitivity = Backup_mouse_sensitivity ;
	Joy_sensitivity = Backup_joy_sensitivity ;
	Joy_dead_zone_size = Backup_joy_deadzone * 5;
	gr_set_gamma(Backup_gamma);

	if(!(Game_mode & GM_MULTIPLAYER)){
		Game_skill_level = Backup_skill_level;
	}

	Briefing_voice_enabled = Backup_briefing_voice_enabled;
	Use_mouse_to_fly = Backup_use_mouse_to_fly;

	if ( Options_detail_inited ) {
		Detail = Detail_original;
	}

	// We have to discard in game options here
	if (Using_in_game_options) {
		options::OptionsManager::instance()->discardChanges();
	}

	// adds scripting hook for 'On Options Menu Closed' --wookieejedi
	if (scripting::hooks::OnOptionsMenuClosed->isActive()) {
		scripting::hooks::OnOptionsMenuClosed->run(scripting::hook_param_list(scripting::hook_param("OptionsAccepted", 'b', false)));
	}

	gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
}

void options_change_gamma(float delta)
{
	char tmp_gamma_string[32];

	auto gamma = Gr_gamma + delta;
	if (gamma < 0.1f) {
		gamma = 0.1f;
		gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);

	} else if (gamma > 5.0f) {
		gamma = 5.0f;
		gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);

	} else {
		gamesnd_play_iface(InterfaceSounds::USER_SELECT);
	}

	gr_set_gamma(gamma);
	sprintf(tmp_gamma_string, NOX("%.2f"), gamma);

	os_config_write_string( NULL, NOX("GammaD3D"), tmp_gamma_string );

	// The Gamma option sets its display value differently to the serialized value itself
	// so we'll leave this here instead of trying to make a global method that works just
	// for this one specific case
	if (Using_in_game_options) {
		const options::OptionBase* thisOpt = options::OptionsManager::instance()->getOptionByKey("Graphics.Gamma");
		if (thisOpt != nullptr) {
			auto val = thisOpt->getCurrentValueDescription();
			SCP_string newVal = std::to_string(gamma);  // OptionsManager stores values as serialized strings
			thisOpt->setValueDescription({val.display, newVal.c_str()});
		}
	}
}

void options_button_pressed(int n)
{
	int choice;	

	switch (n) {		
		case OPTIONS_TAB:
		case MULTIPLAYER_TAB:
		case DETAIL_LEVELS_TAB:	
			if (Tab != n)
				options_change_tab(n);

			break;

		case ABORT_GAME_BUTTON:
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			choice = popup( PF_NO_NETWORKING | PF_BODY_BIG, 2, POPUP_NO, POPUP_YES, XSTR( "Exit Game?", 374));
			if ( choice == 1 ) {
				if (gameseq_get_state(1) == GS_STATE_DEBRIEF && (Game_mode & GM_CAMPAIGN_MODE)) {
					// auto-accept mission outcome before quitting
					debrief_maybe_auto_accept();
				}
				options_accept();
				gameseq_post_event(GS_EVENT_QUIT_GAME);
				return;
			}
			break;

		case CONTROL_CONFIG_BUTTON:
			options_accept();
			gamesnd_play_iface(InterfaceSounds::SWITCH_SCREENS);
			gameseq_post_event(GS_EVENT_CONTROL_CONFIG);
			break;				

		case HUD_CONFIG_BUTTON:
			// can't go to the hud config screen when a multiplayer observer
			if((Game_mode & GM_MULTIPLAYER) && (Net_player->flags & NETINFO_FLAG_OBSERVER)){
				gamesnd_play_iface(InterfaceSounds::GENERAL_FAIL);
				options_add_notify(XSTR( "Cannot use HUD config when an observer!", 375));
				break;
			}
			options_accept();
			gamesnd_play_iface(InterfaceSounds::SWITCH_SCREENS);
			gameseq_post_event(GS_EVENT_HUD_CONFIG);
			break;

		case ACCEPT_BUTTON:
			options_accept();
			gamesnd_play_iface(InterfaceSounds::COMMIT_PRESSED);
			gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
			break;		

			// BEGIN - detail level tab buttons

			// Target View Rendering is currently not handled by in-game options, assumes "On"
		case HUD_TARGETVIEW_RENDER_ON:
			Detail.targetview_model = true;
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case HUD_TARGETVIEW_RENDER_OFF:
			Detail.targetview_model = false;
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

			// Planets is currently not handled by in-game options, assumes "On"
		case PLANETS_ON:
			Detail.planets_suns = true;
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case PLANETS_OFF:
			Detail.planets_suns = false;
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

			// Weapon extras is currently not handled by in-game options, assumes "On"
		case WEAPON_EXTRAS_ON:
			Detail.weapon_extras = true;
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case WEAPON_EXTRAS_OFF:
			Detail.weapon_extras = false;
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;		

		case LOW_DETAIL_N:
			options_detail_set_level(DefaultDetailPreset::Low);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case MEDIUM_DETAIL_N:
			options_detail_set_level(DefaultDetailPreset::Medium);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case HIGH_DETAIL_N:
			options_detail_set_level(DefaultDetailPreset::High);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case VERY_HIGH_DETAIL_N:
			options_detail_set_level(DefaultDetailPreset::VeryHigh);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case CUSTOM_DETAIL_N:
			options_detail_set_level(DefaultDetailPreset::Custom);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;
			// END - detail level tab buttons

		case GAMMA_DOWN:
			options_change_gamma(-0.05f);
			// Gamma in-game change is handled in the above method
			break;

		case GAMMA_UP:
			options_change_gamma(0.05f);
			// Gamma in-game change is handled in the above method
			break;

		case BRIEF_VOICE_ON:
			Briefing_voice_enabled = true;
			options::OptionsManager::instance()->set_ingame_binary_option("Audio.BriefingVoice", true);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case BRIEF_VOICE_OFF:
			Briefing_voice_enabled = false;
			options::OptionsManager::instance()->set_ingame_binary_option("Audio.BriefingVoice", false);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case MOUSE_ON:
			Use_mouse_to_fly = 1;
			options::OptionsManager::instance()->set_ingame_binary_option("Input.UseMouse", true);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;

		case MOUSE_OFF:
			Use_mouse_to_fly = 0;
			options::OptionsManager::instance()->set_ingame_binary_option("Input.UseMouse", false);
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
			break;
	}
}

void options_sliders_update()
{
	// sound slider
	if (Options_sliders[gr_screen.res][OPT_SOUND_VOLUME_SLIDER].slider.pos != Sound_volume_int) {
		Sound_volume_int = Options_sliders[gr_screen.res][OPT_SOUND_VOLUME_SLIDER].slider.pos;
		snd_set_effects_volume((float) (Sound_volume_int) / 9.0f);
		options::OptionsManager::instance()->set_ingame_range_option("Audio.Effects", Master_sound_volume); // Volume options save the global float, not the range slider position
		gamesnd_play_iface(InterfaceSounds::USER_SELECT);
	}

	// music slider
	if (Options_sliders[gr_screen.res][OPT_MUSIC_VOLUME_SLIDER].slider.pos != Music_volume_int) {
		Music_volume_int = Options_sliders[gr_screen.res][OPT_MUSIC_VOLUME_SLIDER].slider.pos;
		event_music_set_volume((float) (Music_volume_int) / 9.0f);
		options::OptionsManager::instance()->set_ingame_range_option("Audio.Music", Master_event_music_volume); // Volume options save the global float, not the range slider position
		gamesnd_play_iface(InterfaceSounds::USER_SELECT);
	}

	// voice slider
	if (Options_sliders[gr_screen.res][OPT_VOICE_VOLUME_SLIDER].slider.pos != Voice_volume_int) {
		Voice_volume_int = Options_sliders[gr_screen.res][OPT_VOICE_VOLUME_SLIDER].slider.pos;
		snd_set_voice_volume((float) (Voice_volume_int) / 9.0f);
		options::OptionsManager::instance()->set_ingame_range_option("Audio.Voice", Master_voice_volume); // Volume options save the global float, not the range slider position
		options_play_voice_clip();
	}

	if (Mouse_sensitivity != Options_sliders[gr_screen.res][OPT_MOUSE_SENS_SLIDER].slider.pos) {
		Mouse_sensitivity = Options_sliders[gr_screen.res][OPT_MOUSE_SENS_SLIDER].slider.pos;
		options::OptionsManager::instance()->set_ingame_range_option("Input.MouseSensitivity", Mouse_sensitivity);
		gamesnd_play_iface(InterfaceSounds::USER_SELECT);
	}

	if (Joy_sensitivity != Options_sliders[gr_screen.res][OPT_JOY_SENS_SLIDER].slider.pos) {
		Joy_sensitivity = Options_sliders[gr_screen.res][OPT_JOY_SENS_SLIDER].slider.pos;
		options::OptionsManager::instance()->set_ingame_range_option("Input.JoystickSensitivity", Joy_sensitivity);
		gamesnd_play_iface(InterfaceSounds::USER_SELECT);
	}

	if (Joy_dead_zone_size != Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.pos * 5) {
		Joy_dead_zone_size = Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.pos * 5;
		options::OptionsManager::instance()->set_ingame_range_option("Input.JoystickDeadZone", Joy_dead_zone_size);
		gamesnd_play_iface(InterfaceSounds::USER_SELECT);
	}

	if (Game_skill_level != Options_sliders[gr_screen.res][OPT_SKILL_SLIDER].slider.pos) {
		Game_skill_level = Options_sliders[gr_screen.res][OPT_SKILL_SLIDER].slider.pos;
		options::OptionsManager::instance()->set_ingame_range_option("Game.SkillLevel", Game_skill_level);
		gamesnd_play_iface(InterfaceSounds::USER_SELECT);
	}
}

void options_detail_sliders_in_game_update()
{
	// Save in-game options settings
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.Detail", Detail.detail_distance);
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.NebulaDetail", Detail.nebula_detail);
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.Texture", Detail.hardware_textures);
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.Particles", Detail.num_particles);
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.SmallDebris", Detail.num_small_debris);
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.ShieldEffects", Detail.shield_effects);
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.Stars", Detail.num_stars);
	options::OptionsManager::instance()->set_ingame_multi_option("Graphics.Lighting", Detail.lighting);
}

void options_accept()
{
	// apply the selected multiplayer options
	if ( Options_multi_inited ) {
		// commenting out old method, as we are using the new streamlined method
		// which is justified via comment in the function itself
		// --wookieejedi and confirmed with taylor

		//if (!options_multi_accept()) {
			//gamesnd_play_iface(InterfaceSounds::COMMIT_PRESSED);
			//popup(PF_USE_AFFIRMATIVE_ICON, 1, POPUP_OK, "PXO is selected but password or username is missing");
			//return false;
		//}

		options_multi_accept();
	}

	// We have to save in game options here
	if (Using_in_game_options) {
		// detail sliders are updated every frame but it's silly to run OptionsManager every frame, too
		// so just set them on Accept and then persist.
		options_detail_sliders_in_game_update();

		options::OptionsManager::instance()->persistChanges();
	}

	// If music is zero volume, disable
	if ( Master_event_music_volume <= 0.0f ) {
//		event_music_disable();
		event_music_level_close();
	}

	// apply other options (display options, etc)
	// note: return in here (and play failed sound) if they can't accept yet for some reason

	// run posting events outside this function, 
	// since they will vary depending on what button was clicked --wookieejedi

	// adds scripting hook for 'On Options Menu Closed' --wookieejedi
	if (scripting::hooks::OnOptionsMenuClosed->isActive()) {
		scripting::hooks::OnOptionsMenuClosed->run(scripting::hook_param_list(scripting::hook_param("OptionsAccepted", 'b', true)));
	}
}

void options_load_background_and_mask(int tab)
{
	Assert(tab == OPTIONS_TAB || tab == DETAIL_LEVELS_TAB );
	Backgrounds[gr_screen.res][tab].bitmap = bm_load(Backgrounds[gr_screen.res][tab].filename);
	Backgrounds[gr_screen.res][tab].mask = bm_load(Backgrounds[gr_screen.res][tab].mask_filename);
}

int Gamma_last_set = -1;
int Gamma_colors_inited = 0;

void options_menu_init()
{
	int i, j;
	options_buttons *b;

	Assert(!Options_menu_inited);

	Tab = 0;
	Gamma_last_set = -1;

	common_set_interface_palette("InterfacePalette");  // set the interface palette
	Ui_window.create(0, 0, gr_screen.max_w_unscaled, gr_screen.max_h_unscaled, 0);

	for (i=0; i<PLANETS_ON; i++) {
		b = &Buttons[gr_screen.res][i];

		b->button.create(&Ui_window, "", b->x, b->y, 60, 30, b->flags & REPEAT, 1);
		// set up callback for when a mouse first goes over a button
		if (b->filename) {
			b->button.set_bmaps(b->filename);
			if ( !(b->flags & NO_MOUSE_OVER_SOUND) ) {
				b->button.set_highlight_action(common_play_highlight_sound);
			}

		} else {
			b->button.hide();
		}

		b->button.link_hotspot(b->hotspot);
		if (i < NUM_COMMONS) {
			for (j=0; j<MAX_BMAPS_PER_GADGET; j++){
				Button_bms[i][j] = b->button.bmap_ids[j];
			}
		}
	}

	// add all xstr text
	for(i=0; i<OPTIONS_NUM_TEXT; i++){
		Ui_window.add_XSTR(&Options_text[gr_screen.res][i]);
	}

	// bogus controls
	Detail_bogus.base_create(&Ui_window, UI_KIND_ICON, 0, 0, 0, 0);
	Options_bogus.base_create(&Ui_window, UI_KIND_ICON, 0, 0, 0, 0);

	Buttons[gr_screen.res][GAMMA_DOWN].button.set_hotkey(KEY_COMMA);
	Buttons[gr_screen.res][GAMMA_UP].button.set_hotkey(KEY_PERIOD);

	/*
	Skill_control.first_frame = bm_load_animation("OPa_11", &Skill_control.total_frames);
	if (Skill_control.first_frame < 0) {
		Error(LOCATION, "Could not load OPa_11.ani\n");
		return;
	}
	*/

	for (i=0; i<NUM_TABS; i++) {
		Backgrounds[gr_screen.res][i].bitmap = -1;
		Backgrounds[gr_screen.res][i].mask = -1;
	}

	options_load_background_and_mask(OPTIONS_TAB);
	options_tab_setup(0);

	Backup_skill_level = Game_skill_level;
	Backup_sound_volume = Master_sound_volume;
	Backup_music_volume = Master_event_music_volume;
	Backup_voice_volume = Master_voice_volume;
	Backup_briefing_voice_enabled = Briefing_voice_enabled;
	Backup_use_mouse_to_fly = Use_mouse_to_fly;

	Backup_mouse_sensitivity = Mouse_sensitivity;
	Backup_joy_sensitivity = Joy_sensitivity;
	Backup_joy_deadzone = Joy_dead_zone_size / 5;
	Backup_gamma = Gr_gamma;
	
	// create slider	
	for ( i = 0; i < NUM_OPTIONS_SLIDERS; i++ ) {
		 Options_sliders[gr_screen.res][i].slider.create(&Ui_window, Options_sliders[gr_screen.res][i].x, Options_sliders[gr_screen.res][i].y,
														 Options_sliders[gr_screen.res][i].dots, Options_sliders[gr_screen.res][i].filename,
														 Options_sliders[gr_screen.res][i].hotspot,
														 Options_sliders[gr_screen.res][i].right_filename, Options_sliders[gr_screen.res][i].right_mask, Options_sliders[gr_screen.res][i].right_x, Options_sliders[gr_screen.res][i].right_y,
														 Options_sliders[gr_screen.res][i].left_filename, Options_sliders[gr_screen.res][i].left_mask, Options_sliders[gr_screen.res][i].left_x, Options_sliders[gr_screen.res][i].left_y,
														 Options_sliders[gr_screen.res][i].dot_w);
	}	

	// maybe disable the skill slider
	if( ((Game_mode & GM_IN_MISSION) && (!popupdead_is_active())) || (Game_mode & GM_MULTIPLAYER) ) {
		Options_sliders[gr_screen.res][OPT_SKILL_SLIDER].slider.disable();
		Ui_window.use_hack_to_get_around_stupid_problem_flag = 0;
	}
	
	// setup slider values 
	// note slider scale is 0-9, while Master_ values calc with 1-10 scale (hence the -1)
	Sound_volume_int = Options_sliders[gr_screen.res][OPT_SOUND_VOLUME_SLIDER].slider.pos = (int)std::lround(Master_sound_volume * 9.0f);
	Music_volume_int = Options_sliders[gr_screen.res][OPT_MUSIC_VOLUME_SLIDER].slider.pos = (int)std::lround(Master_event_music_volume * 9.0f);
	Voice_volume_int = Options_sliders[gr_screen.res][OPT_VOICE_VOLUME_SLIDER].slider.pos = (int)std::lround(Master_voice_volume * 9.0f);

	Options_sliders[gr_screen.res][OPT_JOY_SENS_SLIDER].slider.pos = Joy_sensitivity;	
	Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.pos = Joy_dead_zone_size / 5;
	Options_sliders[gr_screen.res][OPT_MOUSE_SENS_SLIDER].slider.pos = Mouse_sensitivity;
	Options_sliders[gr_screen.res][OPT_SKILL_SLIDER].slider.pos = Game_skill_level;

	Gamma_colors_inited = 0;

	if (Cmdline_deadzone >= 0){
		//Deadzone is being set by the command line 
		Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.disable();
		Options_sliders[gr_screen.res][OPT_JOY_DEADZONE_SLIDER].slider.hide();
	}

	Options_menu_inited = 1;

	// hide options crap
	options_detail_hide_stuff();
}

void options_menu_close()
{
	int i;	

	Assert(Options_menu_inited);	

	for (i=0; i<NUM_TABS; i++) {
		if (Backgrounds[gr_screen.res][i].bitmap >= 0){
			bm_release(Backgrounds[gr_screen.res][i].bitmap);
		}
		if ((Backgrounds[gr_screen.res][i].mask >= 0) && (i != Tab)){  // Ui_window.destroy() expects to release current tab's mask.
			bm_release(Backgrounds[gr_screen.res][i].mask);
		}
	}

	if (Voice_vol_handle.isValid()) {
		snd_stop(Voice_vol_handle);
		Voice_vol_handle = sound_handle::invalid();
	}

	options_multi_close();

	Ui_window.destroy();
	common_free_interface_palette();		// restore game palette
	Pilot.save_player();
	Pilot.save_savefile();
	game_flush();
	
	Options_menu_inited = 0;
	Options_multi_inited = 0;
	Options_detail_inited = 0;
}


void draw_gamma_box()
{
	int x, y, v;

// NEILK: i had to change this declaration because the size is determined dynamically. I just picked an arbitrary large number to data size (although we should always be using less)
// TODO: change MAX size to maximum size for a 1024x768 bitmap
//	ushort Gamma_data[Options_gamma_coords[gr_screen.res][OPTIONS_W_COORD]*Options_gamma_coords[gr_screen.res][OPTIONS_H_COORD]*2];
	ushort Gamma_data[MAX_GAMMA_BITMAP_SIZE];

	v = fl2i( pow(0.5f, 1.0f / Gr_gamma) * 255.0f );
	if (v > 255){
		v = 255;
	} else if (v < 0){
		v = 0;
	}

	Gamma_last_set = v;

	{
		ushort clr_full_white = 0;
		ushort clr_half_white = 0;
		ubyte r, g, b, a;

		BM_SELECT_TEX_FORMAT();

		// set full white
		r = g = b = a = 255;		
		bm_set_components((ubyte*)&clr_full_white, &r, &g, &b, &a);

		// set half white
		r = g = b = (ubyte)v;
		bm_set_components((ubyte*)&clr_half_white, &r, &g, &b, &a);

		memset( Gamma_data, 0, sizeof(ushort) * MAX_GAMMA_BITMAP_SIZE );

		ushort *dptr = Gamma_data;
		for (y=0; y<Options_gamma_coords[gr_screen.res][OPTIONS_H_COORD]; y++) {
			for (x=0; x<Options_gamma_coords[gr_screen.res][OPTIONS_W_COORD]; x++) {
				if ((x / 20) & 1) {
					*dptr = clr_half_white;
				} else {
					if ((x & 1) == (y & 1)) {
						*dptr = clr_full_white;
					} else {
						*dptr = 0;
					}
				}
				dptr++;
			}
		}

		// always go back to screen format
		BM_SELECT_SCREEN_FORMAT();

		// if we're in bitmap poly mode		
		int Gamma_bitmap = bm_create( 16, Options_gamma_coords[gr_screen.res][OPTIONS_W_COORD], Options_gamma_coords[gr_screen.res][OPTIONS_H_COORD], Gamma_data, 0 );
		gr_set_bitmap(Gamma_bitmap);
		gr_bitmap( Options_gamma_coords[gr_screen.res][OPTIONS_X_COORD], Options_gamma_coords[gr_screen.res][OPTIONS_Y_COORD], GR_RESIZE_MENU );

		bm_release( Gamma_bitmap );

	}
}


void options_menu_do_frame(float  /*frametime*/)
{
	int i, k, x, y;	

	Assert(Options_menu_inited);
	k = Ui_window.process() & ~KEY_DEBUGGED;
	switch (k) {
		case KEY_SHIFTED | KEY_TAB:
		case KEY_LEFT:  // activate previous tab
			i = Tab - 1;
			if (i < 0)
				i = NUM_TABS - 1;

			options_change_tab(i);
			break;

		case KEY_TAB:
		case KEY_RIGHT:  // activate next tab
			// check to see if the multiplayer options screen wants to eat the tab kay
			if ((k == KEY_TAB) && (Tab == MULTIPLAYER_TAB)) {
				if (options_multi_eat_tab()) {
					break;
				}
			}

			i = Tab + 1;
			if (i >= NUM_TABS)
				i = 0;

			options_change_tab(i);
			break;

		case KEY_C:
			if (Tab == OPTIONS_TAB) {
				options_accept();
				gamesnd_play_iface(InterfaceSounds::SWITCH_SCREENS);
				gameseq_post_event(GS_EVENT_CONTROL_CONFIG);
				return;
			}

			break;

		case KEY_H:
			if (Tab == OPTIONS_TAB) {
				options_accept();
				gamesnd_play_iface(InterfaceSounds::SWITCH_SCREENS);
				gameseq_post_event(GS_EVENT_HUD_CONFIG);
				return;
			}

			break;

		case KEY_ESC:
			if (escape_key_behavior_in_options == EscapeKeyBehaviorInOptions::SAVE) {
				options_accept();
				gamesnd_play_iface(InterfaceSounds::COMMIT_PRESSED);
				gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
				return;
			} else {
				options_cancel_exit();
				return;
			}
			break;

		case KEY_CTRLED | KEY_ENTER:
			options_accept();
			gamesnd_play_iface(InterfaceSounds::COMMIT_PRESSED);
			gameseq_post_event(GS_EVENT_PREVIOUS_STATE);
			return;
			break;

		case KEY_DELETE:
			break;

		case KEY_ENTER:			
			break;

		case KEY_F3: // SCP ingame options
			if (Using_in_game_options) {
				// Going into F3 Options needs to either discard or save the changes made here. 
				// There's an argument to be made for both but I feel like saving is the better choice - Mjn
				// This is also consistent with the protocols for saving options 
				// when navigating to the Controls or HUD screens --wookieejedi
				options_accept();
				gamesnd_play_iface(InterfaceSounds::IFACE_MOUSE_CLICK);
				gameseq_post_event(GS_EVENT_INGAME_OPTIONS);
				return;
			}
			break;
	}	

	for (i=0; i<NUM_BUTTONS; i++) {
		if (Buttons[gr_screen.res][i].button.pressed())
			options_button_pressed(i);
	}

	options_sliders_update();

	// if we're in the multiplayer options tab, get the background bitmap from the options multi module
	if(Tab == MULTIPLAYER_TAB){
		i = options_multi_background_bitmap();
	} else {
		i = Backgrounds[gr_screen.res][Tab].bitmap;
	}

	GR_MAYBE_CLEAR_RES(i);
	if (i >= 0) {
		gr_set_bitmap(i);
		gr_bitmap(0, 0, GR_RESIZE_MENU);
	} 

	Ui_window.draw();

	// NOTE : this must be done here so that any special drawing crap we do is not overwritten by the UI_WINDOW::draw() call
	// do specific processing for the multiplayer tab
	switch (Tab) {
		case MULTIPLAYER_TAB:
			options_multi_do(k);
			break;

		case DETAIL_LEVELS_TAB:
			options_detail_do_frame();
			break;

		default:
			Game_skill_level = Options_sliders[gr_screen.res][OPT_SKILL_SLIDER].slider.pos;
			break;
	}

	// handle the displaying of any notification messages
	options_notify_do_frame();

	for (i=0; i<NUM_TABS; i++){
		if (Buttons[gr_screen.res][i].button.button_down()){
			break;
		}
	}

	if (i == NUM_TABS){
		Buttons[gr_screen.res][Tab].button.draw_forced(2);
	}

	if (Tab == OPTIONS_TAB) {
		// draw correct frame for briefing voice radio buttons
		if (Briefing_voice_enabled) {
			options_force_button_frame(BRIEF_VOICE_ON, 2);
			options_force_button_frame(BRIEF_VOICE_OFF, 0);

		} else {
			options_force_button_frame(BRIEF_VOICE_OFF, 2);
			options_force_button_frame(BRIEF_VOICE_ON, 0);
		}

		if (Use_mouse_to_fly) {
			options_force_button_frame(MOUSE_ON, 2);
			options_force_button_frame(MOUSE_OFF, 0);

		} else {
			options_force_button_frame(MOUSE_OFF, 2);
			options_force_button_frame(MOUSE_ON, 0);
		}		

		int w;
		gr_get_string_size(&w, NULL, Skill_level_names(Game_skill_level));
		x = Options_skills_text_coords[gr_screen.res][OPTIONS_X_COORD];
		y = Options_skills_text_coords[gr_screen.res][OPTIONS_Y_COORD];
		gr_set_color_fast(&Color_bright_white);
		gr_string(x + (Options_skills_text_coords[gr_screen.res][OPTIONS_W_COORD] / 2) - (w/2), y, Skill_level_names(Game_skill_level), GR_RESIZE_MENU);

		//==============================================================================
		// Draw the gamma adjustment grid.

		draw_gamma_box();
		
		gr_set_color_fast(&Color_white);
		x = Options_gamma_num_coords[gr_screen.res][OPTIONS_X_COORD];
		y = Options_gamma_num_coords[gr_screen.res][OPTIONS_Y_COORD];

		gr_printf_menu(x, y, NOX("%.2f"), Gr_gamma);
	}
	//==============================================================================

	// maybe blit a waveform
	if(Tab == MULTIPLAYER_TAB){
		options_multi_vox_process_waveform();
	}

	// maybe blit the SCP Options string
	if (Using_in_game_options) {
		gr_set_color_fast(&Color_bright_blue);
		gr_string(Options_scp_string_coords[gr_screen.res][OPTIONS_X_COORD],
			Options_scp_string_coords[gr_screen.res][OPTIONS_Y_COORD],
			XSTR(Options_scp_string_text.first.c_str(), Options_scp_string_text.second),
			GR_RESIZE_MENU);
	}
	
	gr_flip();
}


// ---------------------------------------------------------------------------------------------------------
// DETAIL LEVEL OPTIONS definitions  BEGIN
//

void options_detail_synch_sliders()
{
	Detail_slider_pos[DETAIL_DISTANCE_SLIDER] = Detail_sliders[gr_screen.res][DETAIL_DISTANCE_SLIDER].slider.pos = Detail.detail_distance;
	Detail_slider_pos[NEBULA_DETAIL_SLIDER] = Detail_sliders[gr_screen.res][NEBULA_DETAIL_SLIDER].slider.pos = Detail.nebula_detail;
	Detail_slider_pos[HARDWARE_TEXTURES_SLIDER] = Detail_sliders[gr_screen.res][HARDWARE_TEXTURES_SLIDER].slider.pos = Detail.hardware_textures;	
	Detail_slider_pos[SHARD_CULLING_SLIDER] = Detail_sliders[gr_screen.res][SHARD_CULLING_SLIDER].slider.pos = Detail.num_small_debris;
	Detail_slider_pos[SHIELD_DETAIL_SLIDER] = Detail_sliders[gr_screen.res][SHIELD_DETAIL_SLIDER].slider.pos = Detail.shield_effects;
	Detail_slider_pos[NUM_STARS_SLIDER] = Detail_sliders[gr_screen.res][NUM_STARS_SLIDER].slider.pos = Detail.num_stars;
	Detail_slider_pos[NUM_PARTICLES_SLIDER] = Detail_sliders[gr_screen.res][NUM_PARTICLES_SLIDER].slider.pos = Detail.num_particles;
	Detail_slider_pos[LIGHTING_SLIDER] = Detail_sliders[gr_screen.res][LIGHTING_SLIDER].slider.pos = Detail.lighting;
}

void options_detail_init()
{
	int i;
	options_buttons *b;

	Detail_original = Detail;

	options_load_background_and_mask(DETAIL_LEVELS_TAB);

	for (i=PLANETS_ON; i<NUM_BUTTONS; i++) {
		b = &Buttons[gr_screen.res][i];

		b->button.create(&Ui_window, "", b->x, b->y, 60, 30, b->flags & REPEAT, 1);
		// set up callback for when a mouse first goes over a button
		if (b->filename) {
			b->button.set_bmaps(b->filename);
			if ( !(b->flags & NO_MOUSE_OVER_SOUND) ) {
				b->button.set_highlight_action(common_play_highlight_sound);
			}

		} else {
			b->button.hide();
		}

		b->button.link_hotspot(b->hotspot);
	}

	// create detail level sliders	
	for ( i = 0; i < NUM_DETAIL_SLIDERS; i++ ) {
		Detail_sliders[gr_screen.res][i].slider.create(&Ui_window, Detail_sliders[gr_screen.res][i].x, Detail_sliders[gr_screen.res][i].y,
														Detail_sliders[gr_screen.res][i].dots, Detail_sliders[gr_screen.res][i].filename,
														Detail_sliders[gr_screen.res][i].hotspot, Detail_sliders[gr_screen.res][i].left_filename, Detail_sliders[gr_screen.res][i].left_mask, Detail_sliders[gr_screen.res][i].left_x, Detail_sliders[gr_screen.res][i].left_y,
														Detail_sliders[gr_screen.res][i].right_filename, Detail_sliders[gr_screen.res][i].right_mask, Detail_sliders[gr_screen.res][i].right_x, Detail_sliders[gr_screen.res][i].right_y,
														Detail_sliders[gr_screen.res][i].dot_w);
	}

	// init the actual slider positions and our internal positions
	options_detail_synch_sliders();
}

bool shader_compile_status = false;
bool shader_compile_started = false;
SCP_string recompile_state = "";
void shader_recompile_callback(size_t current, size_t total) 
{
	recompile_state = "";
	recompile_state += "Recompiling shader ";
	recompile_state += std::to_string(current + 1);
	recompile_state += "/";
	recompile_state += std::to_string(total);

	shader_compile_status = (current + 1) == total;
}

int recompile_shaders() 
{
	if (!shader_compile_started) {
		gr_recompile_all_shaders(shader_recompile_callback);
		shader_compile_started = true;
	}
	popup_change_text(recompile_state.c_str());
	return shader_compile_status;
}

void options_detail_sliders_update()
{
	int i;

	for (i = 0; i < NUM_DETAIL_SLIDERS; i++) {
		if (Detail_sliders[gr_screen.res][i].slider.pos != Detail_slider_pos[i]) {
			Detail_slider_pos[i] = Detail_sliders[gr_screen.res][i].slider.pos;
			gamesnd_play_iface(InterfaceSounds::USER_SELECT);
		}
	}

	// set Detail based on slider positions
	Detail.detail_distance = Detail_sliders[gr_screen.res][DETAIL_DISTANCE_SLIDER].slider.pos;

	// modify nebula stuff
	Detail.nebula_detail = Detail_sliders[gr_screen.res][NEBULA_DETAIL_SLIDER].slider.pos;

	Detail.hardware_textures = Detail_sliders[gr_screen.res][HARDWARE_TEXTURES_SLIDER].slider.pos;
	Detail.num_small_debris = Detail_sliders[gr_screen.res][SHARD_CULLING_SLIDER].slider.pos;
	Detail.shield_effects = Detail_sliders[gr_screen.res][SHIELD_DETAIL_SLIDER].slider.pos;
	Detail.num_stars = Detail_sliders[gr_screen.res][NUM_STARS_SLIDER].slider.pos;
	Detail.num_particles = Detail_sliders[gr_screen.res][NUM_PARTICLES_SLIDER].slider.pos;

	// If the new lighting setting is above 3 and the old one was below or the reverse,
	// we need to recompile all shaders we have to account for the changed lighting model.
	
	if (Detail.lighting != Detail_sliders[gr_screen.res][LIGHTING_SLIDER].slider.pos) {
		Detail.lighting = Detail_sliders[gr_screen.res][LIGHTING_SLIDER].slider.pos;
		shader_compile_status = false;
		shader_compile_started = false;
		popup_till_condition(recompile_shaders, POPUP_CANCEL, "Recompiling shaders");
	}
}

void options_detail_hide_stuff()
{
	int i;

	for ( i = 0; i < NUM_DETAIL_SLIDERS; i++ ) {
		Detail_sliders[gr_screen.res][i].slider.disable();
		Detail_sliders[gr_screen.res][i].slider.hide();
	}

	// this will hide text unassociated with any real control
	Detail_bogus.hide();
}

void options_detail_unhide_stuff()
{
	int i;

	for ( i = 0; i < NUM_DETAIL_SLIDERS; i++ ) {
		Detail_sliders[gr_screen.res][i].slider.enable();
		Detail_sliders[gr_screen.res][i].slider.unhide();
	}

	// this will hide text unassociated with any real control
	Detail_bogus.unhide();
}

void options_force_button_frame(int n, int frame_num)
{
	if ( !Buttons[gr_screen.res][n].button.button_down() ) {
		Buttons[gr_screen.res][n].button.draw_forced(frame_num);
	}
}

// called once per frame to set lit buttons
void options_detail_do_frame()
{
	options_detail_sliders_update();	

	// force on/off buttons to draw their correct setting

	if ( Detail.targetview_model ) {
		options_force_button_frame(HUD_TARGETVIEW_RENDER_ON, 2);
		options_force_button_frame(HUD_TARGETVIEW_RENDER_OFF, 0);
	} else {
		options_force_button_frame(HUD_TARGETVIEW_RENDER_OFF, 2);
		options_force_button_frame(HUD_TARGETVIEW_RENDER_ON, 0);
	}

	if ( Detail.planets_suns) {
		options_force_button_frame(PLANETS_ON, 2);
		options_force_button_frame(PLANETS_OFF, 0);
	} else {
		options_force_button_frame(PLANETS_OFF, 2);
		options_force_button_frame(PLANETS_ON, 0);
	}

	if ( Detail.weapon_extras) {
		options_force_button_frame(WEAPON_EXTRAS_ON, 2);
		options_force_button_frame(WEAPON_EXTRAS_OFF, 0);
	} else {
		options_force_button_frame(WEAPON_EXTRAS_OFF, 2);
		options_force_button_frame(WEAPON_EXTRAS_ON, 0);
	}	

	DefaultDetailPreset current_preset = (Detail.setting != DefaultDetailPreset::Custom) ? current_detail_preset() : DefaultDetailPreset::Custom;

	Detail.setting = current_preset;

	options_force_button_frame(LOW_DETAIL_N, 0);
	options_force_button_frame(MEDIUM_DETAIL_N, 0);
	options_force_button_frame(HIGH_DETAIL_N, 0);
	options_force_button_frame(VERY_HIGH_DETAIL_N, 0);
	options_force_button_frame(CUSTOM_DETAIL_N, 0);

	switch (current_preset) {
	case DefaultDetailPreset::Custom:
		options_force_button_frame(CUSTOM_DETAIL_N, 2);
		break;
	case DefaultDetailPreset::Low:
		options_force_button_frame(LOW_DETAIL_N, 2);
		break;
	case DefaultDetailPreset::Medium:
		options_force_button_frame(MEDIUM_DETAIL_N, 2);
		break;
	case DefaultDetailPreset::High:
		options_force_button_frame(HIGH_DETAIL_N, 2);
		break;
	case DefaultDetailPreset::VeryHigh:
		options_force_button_frame(VERY_HIGH_DETAIL_N, 2);
		break;
	default:
		Assertion(false, "Invalid preset called for in Options menu");
		break;
	}
}

// Set all the detail settings to a predefined level
void options_detail_set_level(DefaultDetailPreset preset)
{
	detail_level_set(preset);
	options_detail_synch_sliders();
}

//
// DETAIL LEVEL tab definitions END
// ---------------------------------------------------------------------------------------------------------
