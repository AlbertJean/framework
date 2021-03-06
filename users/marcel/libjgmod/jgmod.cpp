/*   
 *
 *                _______  _______  __________  _______  _____ 
 *               /____  / / _____/ /         / / ___  / / ___ \
 *               __  / / / / ____ / //   // / / /  / / / /  / /
 *             /  /_/ / / /__/ / / / /_/ / / / /__/ / / /__/ /
 *            /______/ /______/ /_/     /_/ /______/ /______/
 *
 *
 *
 *
 *  The player. Just to demonstrate how JGMOD sounds.
 *  Also used by me for testing MODs. */


#include "framework.h"
#include "framework-allegro2.h"
#include "jgmod.h"

//

#define font_color      7
#define bitmap_height   12
#define starting_speed  100
#define starting_pitch  100

//

static void drawCircle(int chn);

//

struct OLD_CHN_INFO
{
    int old_sample;
    Color color;
};

//

static JGMOD * the_mod = nullptr;

static OLD_CHN_INFO old_chn_info[MAX_ALLEG_VOICE];

static int start_chn;
static int end_chn;
static bool cp_circle = true;
static int note_length = 130;
static int note_relative_pos = 0;

//

static int do_load(const char * filename)
{
	if (the_mod)
	{
		stop_mod();
		destroy_mod(the_mod);
	}
	
	//
	
    the_mod = load_mod((char*)filename);
	
    if (the_mod == nullptr)
	{
        logError("%s", jgmod_error);
        return 1;
	}

    start_chn = 0;
    end_chn = the_mod->no_chn;
	
    if (end_chn > 33)
        end_chn = 33;
	
    if (mi.flag & XM_MODE)
        note_length = 180;
    else
        note_length = 140;
	
	set_mod_speed(starting_speed);
    set_mod_pitch(starting_pitch);
	
	return TRUE;
}

static void handleAction(const std::string & action, const Dictionary & d)
{
	if (action == "filedrop")
	{
		auto file = d.getString("file", "");
		
		if (!file.empty())
		{
			do_load(file.c_str());
			
			play_mod(the_mod, TRUE);
		}
	}
}

//

int main(int argc, char **argv)
{
    fast_loading = FALSE;
    enable_m15 = TRUE;
    enable_lasttrk_loop = TRUE;

    srand(time(0));

    if (argc != 2)
	{
        logInfo(
			"JGMOD %s player by %s\n"
			"Date : %s\n\n"
			"Syntax : jgmod filename\n"
			"\nThis program is used to play MOD files\n",
			JGMOD_VERSION_STR,
			JGMOD_AUTHOR,
			JGMOD_DATE_STR);
		
        return (1);
	}

    if (exists(argv[1]) == 0)
	{
		logError("%s not found", argv[1]);
		return 1;
	}

	if (do_load(argv[1]) == FALSE)
		return 1;
	
	framework.actionHandler = handleAction;
	
	if (framework.init(0, nullptr, 640, 480) == false)
	{
		logError("failed to initialize framework");
		return 1;
	}
	
	setFont("unispace.ttf");
	pushFontMode(FONT_SDF);
	
    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, null) < 0)
	{
        logError("unable to initialize sound card");
        return 1;
	}

    for (int index = 0; index < MAX_ALLEG_VOICE; ++index)
	{
        old_chn_info[index].old_sample = -1;
        old_chn_info[index].color = colorBlack;
	}

    if (install_mod(MAX_ALLEG_VOICE) < 0)
	{
        logError("unable to allocate %d voices", MAX_ALLEG_VOICE);
        return 1;
	}
	
    play_mod(the_mod, TRUE);

    while (is_mod_playing())
	{
        framework.process();
		
		if (keyboard.wentDown(SDLK_LEFT, true))
			prev_mod_track();
		if (keyboard.wentDown(SDLK_RIGHT, true))
			next_mod_track();
		
		if (keyboard.wentDown(SDLK_PLUS, true))
			set_mod_volume(get_mod_volume() + 5);
		if (keyboard.wentDown(SDLK_MINUS, true))
			set_mod_volume(get_mod_volume() - 5);
		
		if (keyboard.wentDown(SDLK_F1, true))
			set_mod_speed (mi.speed_ratio - 5);
		if (keyboard.wentDown(SDLK_F2, true))
			set_mod_speed (mi.speed_ratio + 5);
		if (keyboard.wentDown(SDLK_F3, true))
			set_mod_pitch (mi.pitch_ratio - 5);
		if (keyboard.wentDown(SDLK_F4, true))
			set_mod_pitch (mi.pitch_ratio + 5);
		if (keyboard.wentDown(SDLK_F5, true))
			note_length++;
		
		if (keyboard.wentDown(SDLK_F6, true))
		{
			note_length--;
			if (note_length <= 0)
				note_length = 1;
		}
		if (keyboard.wentDown(SDLK_F7, true))
		{
			note_relative_pos -= 2;
			if (note_relative_pos < -300)
				note_relative_pos = -300;
		}
		if (keyboard.wentDown(SDLK_F8, true))
		{
			note_relative_pos += 2;
			if (note_relative_pos > 300)
				note_relative_pos = 300;
		}
		
		if (keyboard.wentDown(SDLK_r, true))
			play_mod (the_mod, TRUE);
		if (keyboard.wentDown(SDLK_p, true))
			toggle_pause_mode ();
		
		if (keyboard.wentDown(SDLK_DOWN, true))
		{
			if (the_mod->no_chn > 33)
			{
				end_chn = start_chn + 33 + 1;
				if (end_chn > the_mod->no_chn)
					end_chn = the_mod->no_chn;

				start_chn = end_chn - 33;
			}
		}
		if (keyboard.wentDown(SDLK_UP, true))
		{
			if (the_mod->no_chn > 33)
			{
				start_chn--;
				if (start_chn < 0)
					start_chn = 0;

				end_chn = start_chn + 33;
			}
		}

		if (keyboard.wentDown(SDLK_n))
		{
			cp_circle = !cp_circle;
		}

		if (keyboard.wentDown(SDLK_ESCAPE) || keyboard.wentDown(SDLK_SPACE))
		{
			stop_mod();
			destroy_mod (the_mod);
			return 0;
		}
		
        framework.beginDraw(0, 0, 0, 0);
        {
        	// draw header
			
			drawText(0,  0, 12, +1, +1, "Song name   : %s", the_mod->name);
			drawText(0, 12, 12, +1, +1, "No Channels : %2d  Period Type : %s  No Inst : %2d ",
				the_mod->no_chn, (the_mod->flag & LINEAR_MODE) ? "Linear" : "Amiga", the_mod->no_instrument);
			drawText(0, 24, 12, +1, +1, "No Tracks   : %2d  No Patterns : %2d  No Sample : %2d ", the_mod->no_trk, the_mod->no_pat, the_mod->no_sample);
			
			// draw playback info
			
			drawText(0, 36, 12, +1, +1, "Tempo : %3d  Bpm : %3d  Speed : %3d%%  Pitch : %3d%% ", mi.tempo, mi.bpm, mi.speed_ratio, mi.pitch_ratio);
			drawText(0, 48, 12, +1, +1, "Global volume : %2d  User volume : %2d ", mi.global_volume, get_mod_volume());
			drawText(0, 70, 12, +1, +1, "%03d-%02d-%02d    ", mi.trk, mi.pos, mi.tick < 0 ? 0 : mi.tick);

			for (int index = 0; index < the_mod->no_chn; ++index)
			{
				if (old_chn_info[index].old_sample != ci[index].sample)
					old_chn_info[index].color = Color::fromHSL((rand() % 68 + 32) / 100.f, .5f, .5f);

				old_chn_info[index].old_sample = ci[index].sample;
			}

			for (int index = start_chn; index < end_chn; ++index)
			{
				if (cp_circle)
				{
					drawCircle(index);
				}

				if (voice_get_position(voice_table[index]) >= 0 &&
					ci[index].volume >= 1 &&
					ci[index].volenv.v >= 1 &&
					voice_get_frequency(voice_table[index]) > 0 &&
					mi.global_volume > 0)
				{
					drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3d %2d %6dHz %3d ", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan);
					//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2d %6dHz %3d %d %d", index+1, ci[index].sample+1, ci[index].volume,  voice_get_frequency(voice_table[index]), ci[index].pan, ci[index].volenv.v, ci[index].volenv.p);
				}
				else
				{
					drawText(0, 82+(index-start_chn)*bitmap_height, 12, +1, +1, "%2d: %3s %2s %6sHz %3s  ", index+1, " --", "--",  " -----", "---");
					//textprintf (screen, font, 0,82+(index-start_chn)*bitmap_height, font_color, "%2d: %3d %2s %6sHz %3s ", index+1, ci[index].sample+1, "--",  " -----", "---");
				}
			}
		}
		framework.endDraw();
	}
	
    return 0;
}

static void drawCircle(int chn)
{
    if (voice_get_position(voice_table[chn]) >= 0 &&
    	ci[chn].volume >= 1 &&
    	ci[chn].volenv.v >= 1 &&
    	voice_get_frequency(voice_table[chn]) > 0 &&
    	(mi.global_volume > 0))
	{
		gxPushMatrix();
		{
			const int dx = 200;
			const int dy = 79+(chn-start_chn)*bitmap_height;
			gxTranslatef(dx, dy, 0);
			
			const float radius = (ci[chn].volume / 13.f) + 2.f;
			
			float xpos;
			
			if (mi.flag & XM_MODE)
				xpos = voice_get_frequency(voice_table[chn]);
			else
				xpos = voice_get_frequency(voice_table[chn]) * 8363.f / ci[chn].c2spd;

			xpos /= note_length;
			xpos += note_relative_pos;

			if (xpos > 439)
				xpos = 439;
			else if (xpos < 0)
				xpos = 0;
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				setColor(old_chn_info[chn].color);
				hqFillCircle(xpos, 5, radius);
				setColor(colorWhite);
			}
			hqEnd();
		}
		gxPopMatrix();
	}
}
