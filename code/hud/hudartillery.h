/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 



#ifndef _FS2_HUD_ARTILLERY_HEADER_FILE
#define _FS2_HUD_ARTILLERY_HEADER_FILE

#include "globalincs/globals.h"
#include "globalincs/pstypes.h"
#include "fireball/fireballs.h"

// -----------------------------------------------------------------------------------------------------------------------
// ARTILLERY DEFINES/VARS
//

// SSM shapes
#define SSM_SHAPE_POINT			0
#define SSM_SHAPE_CIRCLE		1
#define SSM_SHAPE_SPHERE		2

// global ssm types
typedef struct ssm_info {
	char        name[NAME_LENGTH];          // strike name
	int         count = 1;                  // # of missiles in this type of strike
	int         max_count = -1;             // Maximum # of missiles in this type of strike (-1 for no variance)
	int         weapon_info_index = -1;     // missile type
	int         fireball_type = FIREBALL_WARP;  // Type of fireball to use for warp effect (defaults to FIREBALL_WARP)
	float       warp_radius = 50.0f;        // radius of associated warp effect
	float       warp_time = 4.0f;           // how long the warp effect lasts
	float       radius = 500.0f;            // radius around the target ship
	float       max_radius = -1.0f;         // Maximum radius around the target ship (-1.0f for no variance)
	float       offset = 0.0f;              // offset in front of the target ship
	float       max_offset = -1.0f;         // Maximum offset in front of the target ship (-1.0f for no variance)
	bool        send_message = true;
	bool        use_custom_message = false;
	char        custom_message[NAME_LENGTH];
	gamesnd_id  sound_index;
	int         shape = SSM_SHAPE_CIRCLE;

	ssm_info()
	{
		name[0] = '\0';
		custom_message[0] = '\0';
	}
} ssm_info;

// creation info for the strike (useful for multiplayer)
typedef struct ssm_firing_info {
	SCP_vector<TIMESTAMP>	delay_stamp;	// timestamps
	SCP_vector<vec3d>		start_pos;		// start positions

	int					count;			// The ssm_info count may be variable; this stores the actual number of projectiles for this strike.
	size_t				ssm_index;		// index into ssm_info array
	class object*		target;			// target for the strike
	int					ssm_team;		// team that fired the ssm.
	float				duration;		// how far into the warp effect to fire
} ssm_firing_info;

// the strike itself
typedef struct ssm_strike {
	SCP_vector<int>		fireballs;		// warpin effect fireballs
	SCP_vector<bool>	done_flags;		// when we've fired off the individual missiles

	// this is the info that controls how the strike behaves (just like for beam weapons)
	ssm_firing_info		sinfo;
} ssm_strike;


extern SCP_vector<ssm_info> Ssm_info;


// -----------------------------------------------------------------------------------------------------------------------
// ARTILLERY FUNCTIONS
//

// level init
void hud_init_artillery();

// update all hud artillery related stuff
void hud_artillery_update();

// render all hud artillery related stuff
void hud_artillery_render();

// start a subspace missile effect
// (it might be possible to make `target` const, but that would set off another const-cascade)
void ssm_create(object *target, const vec3d *start, size_t ssm_index, const ssm_firing_info *override, int team);

// Goober5000
extern int ssm_info_lookup(const char *name);

#endif
