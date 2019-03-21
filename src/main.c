#include <string.h>

#include "gfx/di.h"
#include "gfx/gfx.h"

#include "mem/heap.h"

#include "soc/hw_init.h"

#include "core/launcher.h"

#include "utils/util.h"
#include "utils/fs_utils.h"
#include "utils/touch.h"
#include "utils/btn.h"

#include "minerva/minerva.h"

extern void pivot_stack(u32 stack_top);

static inline void setup_gfx()
{
    u32 *fb = display_init_framebuffer();
    gfx_init_ctxt(&g_gfx_ctxt, fb, 1280, 720, 720);
    gfx_clear_buffer(&g_gfx_ctxt);
    gfx_con_init(&g_gfx_con, &g_gfx_ctxt);
    gfx_con_setcol(&g_gfx_con, 0xFFCCCCCC, 1, BLACK);
}

static u32 get_text_width(char *text, int scale)
{
    u32 lenght = strlen(text);
    return lenght * scale * (u32)CHAR_WIDTH;
}

static void render_Main_Info_Centered(int y, int scale, char *text, int level)
{
    g_gfx_con.scale = scale;
    s32 x_offset = (1280 - get_text_width(text, scale)) / 2;
	if(x_offset < 0){
		x_offset = 0;
	}
    gfx_con_setpos(&g_gfx_con, x_offset, y);
	if (level == 1){
		gfx_printf(&g_gfx_con, "%s\n", text);
	}else if(level == 2){
		gfx_printf(&g_gfx_con, "%k%s\n%k", 0xFFFFDD00, text, 0xFFCCCCCC);
	}else if(level == 3){
		gfx_printf(&g_gfx_con, "%k%s\n%k", 0xFF800000, text, 0xFF555555);
	}
}

static int print_no_card(int time)
{
	gfx_clear_buffer(&g_gfx_ctxt);
	time = time + 1;
	g_gfx_con.scale = 2;
	gfx_con_setpos(&g_gfx_con, 0, 5);
	gfx_printf(&g_gfx_con, "Life -%d", time);
	gfx_con_setpos(&g_gfx_con, 640, 5);
	gfx_printf(&g_gfx_con, "Press power button to reboot into RCM...\n");
	if(time > 5){
		render_Main_Info_Centered(320, 5, "PLEASE CHECK YOUR CARD!!!!!!!", 3);
	}else{
		render_Main_Info_Centered(300, 12, "No TF Card!", 2);
		render_Main_Info_Centered(500, 5, "Please insert a card\n", 1);
		render_Main_Info_Centered(570, 5, "Press Vol - to try again\n", 1);
		render_Main_Info_Centered(640, 5, "Press Vol + to power Off :)\n", 1);
	}
	gfx_swap_buffer(&g_gfx_ctxt);
	g_gfx_con.scale = 2;
	return time;
}

void print_now_loading()
{
	gfx_clear_buffer(&g_gfx_ctxt);
	g_gfx_con.scale = 12;
	gfx_con_setpos(&g_gfx_con, 135, 300);
	gfx_printf(&g_gfx_con, "Now Loading...n");
	g_gfx_con.scale = 2;
}

static int print_no_payload(int time)
{
	gfx_clear_buffer(&g_gfx_ctxt);
	time = time + 1;
	g_gfx_con.scale = 2;
	gfx_con_setpos(&g_gfx_con, 0, 5);
	gfx_printf(&g_gfx_con, "Life -%d", time);
	gfx_con_setpos(&g_gfx_con, 640, 5);
	gfx_printf(&g_gfx_con, "Press power button to reboot into RCM...\n");
	if(time > 5){
		render_Main_Info_Centered(320, 5, "PLEASE CHECK YOUR PAYLOAD!!!!!!!", 3);
	}else{
		render_Main_Info_Centered(300, 12, "No PAYLOAD!", 2);
		render_Main_Info_Centered(500, 5, "Check the payload.bin\n", 1);
		render_Main_Info_Centered(570, 5, "Press Vol - to try again\n", 1);
		render_Main_Info_Centered(640, 5, "Press Vol + to power Off :)\n", 1);
	}
	gfx_swap_buffer(&g_gfx_ctxt);
	g_gfx_con.scale = 2;
	return time;
}

void ipl_main()
{
    config_hw();

    pivot_stack(0x90010000);
    heap_init(0x90020000);

    display_init();
    setup_gfx();
    display_backlight_pwm_init();
    display_backlight_brightness(100, 1000);

    g_gfx_con.mute = 1;
    minerva();
    g_gfx_con.mute = 0;

    touch_power_on();
    
	gfx_con_setcol(&g_gfx_con, 0xFFF9F9F9, 0, 0xFF191414);

	int utime = 0;
	if(!sd_mount()){
		utime = print_no_card(utime);
		while (true)
		{
			u32 btn = btn_wait();
			if (btn & BTN_VOL_DOWN){
				if(sd_mount()){
					break;
				}else{
					utime = print_no_card(utime);
				}
			}else if (btn & BTN_VOL_UP){
				power_off();
			}else if (btn & BTN_POWER){
				reboot_rcm();
			}
		};
	};
	
	if(!sd_file_exists("payload.bin")){
		utime = print_no_payload(utime);
		while (true)
		{
			u32 btn = btn_wait();
			if (btn & BTN_VOL_DOWN){
				sd_unmount();
				if(sd_mount()){
					if(sd_file_exists("payload.bin")){
						break;
					}else{
						utime = print_no_payload(utime);
					}
				}else{
					utime = print_no_card(utime);
				}
			}else if (btn & BTN_VOL_UP){
				power_off();
			}else if (btn & BTN_POWER){
				reboot_rcm();
			}
		};
	};
	
	launch_payload("payload.bin");
	
	g_gfx_con.scale = 3;
	gfx_con_setpos(&g_gfx_con, 160, 320);
	gfx_printf(&g_gfx_con, "Press power button to reboot into RCM...\n");
    gfx_swap_buffer(&g_gfx_ctxt);
    while (true) {
        u32 btn = btn_read();
        if (btn & BTN_POWER) {
            reboot_rcm();
        }
    }
	
}