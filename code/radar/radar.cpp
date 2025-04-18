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
#include "gamesnd/gamesnd.h"
#include "globalincs/linklist.h"
#include "graphics/font.h"
#include "graphics/matrix.h"
#include "iff_defs/iff_defs.h"
#include "io/timer.h"
#include "jumpnode/jumpnode.h"
#include "localization/localize.h"
#include "network/multi.h"
#include "object/object.h"
#include "playerman/player.h"
#include "radar/radar.h"
#include "ship/awacs.h"
#include "ship/ship.h"
#include "ship/subsysdamage.h"
#include "weapon/emp.h"
#include "weapon/weapon.h"

extern int radar_target_id_flags;

HudGaugeRadarStd::HudGaugeRadarStd():
HudGaugeRadar(HUD_OBJECT_RADAR_STD, 255, 255, 255)
{
	gr_init_alphacolor( &radar_crosshairs, 255, 255, 255, 196);
}

void HudGaugeRadarStd::initCenterOffsets(float x, float y)
{
	Radar_center_offsets[0] = x;
	Radar_center_offsets[1] = y;
}

void HudGaugeRadarStd::initBitmaps(char *fname)
{
	Radar_gauge.first_frame = bm_load_animation(fname, &Radar_gauge.num_frames);
	if ( Radar_gauge.first_frame < 0 ) {
		Warning(LOCATION,"Cannot load hud ani: %s\n", fname);
	}
}

void HudGaugeRadarStd::blipDrawDistorted(blip *b, int x, int y)
{
	int xdiff, ydiff;
	float scale;
	xdiff =  Random::next(-10, 9);
	ydiff =  Random::next(-10, 9);

	// maybe scale the effect if EMP is active
	if(emp_active_local()){
		scale = emp_current_intensity();

		xdiff = (int)((float)xdiff * scale);
		ydiff = (int)((float)ydiff * scale);
	}

	drawContactCircle(x + xdiff, y + ydiff, b->rad);
}
void HudGaugeRadarStd::blipDrawFlicker(blip *b, int x, int y)
{
	int xdiff=0, ydiff=0, flicker_index;

	if ( (b-Blips) & 1 ) {
		flicker_index=0;
	} else {
		flicker_index=1;
	}

	if ( timestamp_elapsed(Radar_flicker_timer[flicker_index]) ) {
		Radar_flicker_timer[flicker_index] = _timestamp_rand(50,1000);
		Radar_flicker_on[flicker_index] = !Radar_flicker_on[flicker_index];
	}

	if ( !Radar_flicker_on[flicker_index] ) {
		return;
	}

	if (Random::flip_coin()) {
		xdiff = Random::next(-2, 1);
		ydiff = Random::next(-2, 1);
	}

	drawContactCircle(x + xdiff, y + ydiff, b->rad);
}
void HudGaugeRadarStd::blitGauge(bool config)
{
	int x = position[0];
	int y = position[1];
	float scale = 1.0;

	if (config) {
		std::tie(x, y, scale) = hud_config_convert_coord_sys(position[0], position[1], base_w, base_h);
		int bmw, bmh;
		bm_get_info(Radar_gauge.first_frame + 1, &bmw, &bmh);
		hud_config_set_mouse_coords(gauge_config_id, x, x + fl2i(bmw * scale), y, y + fl2i(bmh * scale));
	}
	
	if (Radar_gauge.first_frame + 1 >= 0)
		renderBitmap(Radar_gauge.first_frame+1, x, y, scale, config);
}
void HudGaugeRadarStd::drawBlips(int blip_type, int bright, int distort)
{
	blip *b = NULL;
	blip *blip_head = NULL;
	int x, y;

	Assert((blip_type >= 0) && (blip_type < MAX_BLIP_TYPES));


	// Need to set font.
	font::set_font(font::FONT1);


	// get the appropriate blip list
	if (bright)
		blip_head = &Blip_bright_list[blip_type];
	else
		blip_head = &Blip_dim_list[blip_type];


	// draw all blips of this type
	for (b = GET_FIRST(blip_head); b != END_OF_LIST(blip_head); b = GET_NEXT(b))
	{
		gr_set_color_fast(b->blip_color);
		plotBlip(b, &x, &y);

		// maybe draw cool blip to indicate current target
		if (b->flags & BLIP_CURRENT_TARGET)
		{
			b->rad = Radar_blip_radius_target;
			current_target_x = x;
			current_target_y = y;
		}
		else
		{
			b->rad = Radar_blip_radius_normal;
		}

		// maybe distort blip
		if (distort)
		{
			blipDrawDistorted(b, x, y);
		}
		else if (b->flags & BLIP_DRAW_DISTORTED)
		{
			blipDrawFlicker(b, x, y);
		}
		else
		{
			if (b->radar_image_2d == -1 && b->radar_color_image_2d == -1)
				drawContactCircle(x, y, b->rad);
			else
				drawContactImage(x, y, b->rad, b->radar_image_2d, b->radar_color_image_2d, b->radar_image_size);
		}
	}
}
void HudGaugeRadarStd::drawBlipsSorted(int distort)
{
	current_target_x = 0;
	current_target_y = 0;
	// draw dim blips first, then bright blips
	for (int is_bright = 0; is_bright < 2; is_bright++)
	{
		drawBlips(BLIP_TYPE_JUMP_NODE, is_bright, distort);
		drawBlips(BLIP_TYPE_WARPING_SHIP, is_bright, distort);
		drawBlips(BLIP_TYPE_NAVBUOY_CARGO, is_bright, distort);
		drawBlips(BLIP_TYPE_NORMAL_SHIP, is_bright, distort);
		drawBlips(BLIP_TYPE_BOMB, is_bright, distort);
		drawBlips(BLIP_TYPE_TAGGED_SHIP, is_bright, distort);
	}
	// draw crosshairs last - if at all.
	if(radar_target_id_flags & RTIF_CROSSHAIRS) {
		drawCrosshairs(current_target_x, current_target_y);
	}
}
void HudGaugeRadarStd::drawContactCircle( int x, int y, int rad )
{
	if ( rad == Radar_blip_radius_target )	{
		if (radar_target_id_flags & RTIF_BLINK) {
			if (Missiontime & 8192)
				return;
		}
		renderCircle(x, y, 6);
	} else {
		// rad = RADAR_BLIP_RADIUS_NORMAL;
		renderCircle(x, y, 4);
	}
}
void HudGaugeRadarStd::drawContactImage( int x, int y, int rad, int idx, int clr_idx, int size )
{
	// this we will move as ships.tbl option (or use for radar scaling etc etc)
	//int size = 24; 
	
	int w, h, old_bottom, old_bottom_unscaled, old_right, old_right_unscaled;
	float scalef, wf, hf, xf, yf;
	vec3d blip_scaler;

	if(bm_get_info(idx, &w, &h) < 0)
	{
		// Just if something goes terribly wrong
		drawContactCircle(x, y, rad);
		return;
	}

	// just to make sure the missing casts wont screw the math
	wf = (float) w;
	hf = (float) h;
	xf = (float) x;
	yf = (float) y;

	// make sure we use the larger dimension for the scaling
	// lets go case by case to make sure there are no probs
	if (size == -1)
		scalef = 1.0f;
	else if ((h == w) && (size == h))
		scalef = 1.0f;
	else if ( h > w)
		scalef = ((float) size) / hf;
	else
		scalef = ((float) size) / wf;

	Assert(scalef != 0);

	// animate the targeted icon - option 1 of highlighting the targets
	if ( rad == Radar_blip_radius_target ) {
		if (radar_target_id_flags & RTIF_PULSATE) {
			scalef *= 1.3f + (sinf(10 * f2fl(Missiontime)) * 0.3f);
		}
		if (radar_target_id_flags & RTIF_BLINK) {
			if (Missiontime & 8192)
				return;
		}
		if (radar_target_id_flags & RTIF_ENLARGE) {
			scalef *= 1.3f;
		}
	}

	// setup the scaler
	blip_scaler.xyz.x = scalef;
	blip_scaler.xyz.y = scalef;
	blip_scaler.xyz.z = 1.0f;
	
	old_bottom = gr_screen.clip_bottom;
	old_bottom_unscaled = gr_screen.clip_bottom_unscaled;
	gr_screen.clip_bottom = (int) (old_bottom/scalef);
	gr_screen.clip_bottom_unscaled = (int) (old_bottom_unscaled/scalef);

	old_right = gr_screen.clip_right;
	old_right_unscaled = gr_screen.clip_right_unscaled;
	gr_screen.clip_right = (int) (old_right/scalef);
	gr_screen.clip_right_unscaled = (int) (old_right_unscaled/scalef);

	// scale the drawing coordinates
	x = (int) ((xf / scalef) - wf/2.0f);
	y = (int) ((yf / scalef) - hf/2.0f);

	gr_push_scale_matrix(&blip_scaler);

	if ( idx >= 0 ) {
		gr_set_bitmap(idx,GR_ALPHABLEND_NONE,GR_BITBLT_MODE_NORMAL,1.0f);
		renderBitmap( x, y );
	}

	if ( clr_idx >= 0 ) {
		gr_set_screen_scale(base_w, base_h);
		gr_bitmap(x, y);
		gr_reset_screen_scale();
	}

	gr_pop_scale_matrix();

	gr_screen.clip_bottom = old_bottom;
	gr_screen.clip_bottom_unscaled = old_bottom_unscaled;

	gr_screen.clip_right = old_right;
	gr_screen.clip_right_unscaled = old_right_unscaled;
}

void HudGaugeRadarStd::render(float  /*frametime*/, bool config)
{
	//WMC - This strikes me as a bit hackish
	bool g3_yourself = !g3_in_frame();
	if(g3_yourself)
		g3_start_frame(1);

	int ok_to_blit_radar = 1;

	if (!config) {
		float sensors_str = ship_get_subsystem_strength(Player_ship, SUBSYSTEM_SENSORS);

		if (ship_subsys_disrupted(Player_ship, SUBSYSTEM_SENSORS)) {
			sensors_str = MIN_SENSOR_STR_TO_RADAR - 1;
		}

		// note that on lowest skill level, there is no radar effects due to sensors damage
		if (((Game_skill_level == 0) || (sensors_str > SENSOR_STR_RADAR_NO_EFFECTS)) && !Sensor_static_forced) {
			Radar_static_playing = false;
			Radar_static_next = TIMESTAMP::never();
			Radar_death_timer = TIMESTAMP::never();
			Radar_avail_prev_frame = true;
		} else if (sensors_str < MIN_SENSOR_STR_TO_RADAR) {
			if (Radar_avail_prev_frame) {
				Radar_death_timer = _timestamp(2000);
				Radar_static_next = TIMESTAMP::immediate();
			}
			Radar_avail_prev_frame = false;
		} else {
			Radar_death_timer = TIMESTAMP::never();
			if (Radar_static_next.isNever())
				Radar_static_next = TIMESTAMP::immediate();
		}

		if (timestamp_elapsed(Radar_death_timer)) {
			ok_to_blit_radar = 0;
		}
	}

	setGaugeColor(HUD_C_NONE, config);
	blitGauge(config);
	drawRange(config);

	if ( timestamp_elapsed(Radar_static_next) ) {
		Radar_static_playing = !Radar_static_playing;
		Radar_static_next = _timestamp_rand(50, 750);
	}

	if (!config) {
		// if the emp effect is active, always draw the radar wackily
		if (emp_active_local()) {
			Radar_static_playing = true;
		}

		if (ok_to_blit_radar) {
			if (Radar_static_playing) {
				drawBlipsSorted(1); // passing 1 means to draw distorted
				if (!Radar_static_looping.isValid()) {
					Radar_static_looping = snd_play_looping(gamesnd_get_game_sound(GameSounds::STATIC));
				}
			} else {
				drawBlipsSorted(0);
				if (Radar_static_looping.isValid()) {
					snd_stop(Radar_static_looping);
					Radar_static_looping = sound_handle::invalid();
				}
			}
		} else {
			if (Radar_static_looping.isValid()) {
				snd_stop(Radar_static_looping);
				Radar_static_looping = sound_handle::invalid();
			}
		}
	}

	if(g3_yourself)
		g3_end_frame();
}
void HudGaugeRadarStd::pageIn()
{
	bm_page_in_aabitmap( Radar_gauge.first_frame, Radar_gauge.num_frames );
}

void HudGaugeRadarStd::clampBlip(vec3d* b)
{
	float max_radius;
	float hypotenuse;

	max_radius = i2fl(Radar_radius[0] - 5);

	// Scale blip to radar size
	vm_vec_scale(b, i2fl(Radar_radius[0])/2.0f);

	hypotenuse = hypotf(b->xyz.x, b->xyz.y);

	// Clamp to inside of plot area, if needed
	if (hypotenuse > max_radius) {
		vm_vec_scale2(b, max_radius, hypotenuse);
	}

	// Scale Y to respect aspect ratio
	b->xyz.y *= (i2fl(Radar_radius[1]) / i2fl(Radar_radius[0]));
}

void HudGaugeRadarStd::plotBlip(blip *b, int *x, int *y)
{
	float zdist, rscale;
	vec3d *pos = &b->position;

	if (b->dist < pos->xyz.z) {
		rscale = 0.0f;
	} else {
		rscale = (float) acosf_safe(pos->xyz.z / b->dist) / PI;
	}

	zdist = hypotf(pos->xyz.x, pos->xyz.y);

	vec3d new_pos = {};

	if (zdist >= 0.01f)
	{
		new_pos = *pos;
		new_pos.xyz.z = 0.0f;
		vm_vec_scale(&new_pos, rscale / zdist);		// Values are within +/- 1.0f
		clampBlip(&new_pos);
	}

	*x = fl2i(position[0] + Radar_center_offsets[0] + new_pos.xyz.x);
	*y = fl2i(position[1] + Radar_center_offsets[1] - new_pos.xyz.y);
}

void HudGaugeRadarStd::drawCrosshairs(int x, int y)
{
	int i,j,m;

	gr_set_color_fast(&radar_crosshairs);

	for(i = 0; i < 2; i++) {
		m = (i * 2) - 1;
		renderGradientLine(x + m*4,y,x + m*8,y);
	}
	for(j = 0; j < 2; j++) {
		m = (j * 2) - 1;
		renderGradientLine(x,y + m*4,x,y + m*8);
	}
}
