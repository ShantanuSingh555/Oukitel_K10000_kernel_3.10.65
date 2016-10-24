/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/


#ifdef BUILD_LK
#else
#include <linux/string.h>
#if defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#endif
#endif
#include "lcm_drv.h"

#if defined(BUILD_LK)
#else

#include <linux/proc_fs.h>   //proc file use 
#endif


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(540)
#define FRAME_HEIGHT 										(960)
#define LCM_ID                       (0x8191)

#define REGFLAG_DELAY             	(0XFFE)
#define REGFLAG_END_OF_TABLE      	(0x100)	// END OF REGISTERS MARKER


#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/
//cmi 4.5
//Gamma setting
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x03}},
{0x90,9,{0x03,0x14,0x01,0x00,0x00,0x00,0x38,0x00,0x00}},
{0x91,9,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0x92,11,{0x40,0x03,0x04,0x05,0x06,0x00,0x38,0x00,0x00,0x03,0x04}},
{0x94,8,{0x00,0x08,0x03,0x03,0xCA,0x03,0xCB,0x0C}},
{0x95,16,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0x99,2,{0x00,0x00}},
{0x9A,11,{0x00,0x0F,0x03,0xD4,0x03,0xD6,0x00,0x00,0x00,0x00,0x50}},
{0x9B,6,{0x01,0x38,0x00,0x00,0x00,0x00}},
{0x9C,2,{0x00,0x00}},
{0x9D,8,{0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00}},
{0x9E,2,{0x00,0x00}},
{0xA0,10,{0x9F,0x1F,0x08,0x1F,0x0A,0x1F,0x00,0x1F,0x14,0x1F}},
{0xA1,10,{0x15,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xA2,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xA3,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xA4,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xA5,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x15}},
{0xA6,10,{0x1F,0x14,0x1F,0x01,0x1F,0x0B,0x1F,0x09,0x1F,0x1F}},
{0xA7,10,{0x1F,0x1F,0x0B,0x1F,0x09,0x1F,0x01,0x1F,0x15,0x1F}},
{0xA8,10,{0x14,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xA9,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xAA,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xAB,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}},
{0xAC,10,{0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x14}},
{0xAD,10,{0x1F,0x15,0x1F,0x00,0x1F,0x08,0x1F,0x0A,0x1F,0x1F}},
{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
{0xB1,1,{0xFC}},
{0xBC,3,{0x00,0x00,0x00}},
{0x4C,1,{0x11}},
{0xB8,4,{0x01,0x8F,0xBF,0x8F}},
{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
{0xB0,3,{0x05,0x05,0x05}},
{0xB1,3,{0x05,0x05,0x05}},
{0xB6,3,{0x44,0x44,0x44}},
{0xB7,3,{0x34,0x34,0x34}},
{0xB3,3,{0x10,0x10,0x10}},
{0xB9,3,{0x24,0x24,0x24}},
{0xB4,3,{0x0A,0x0A,0x0A}},
{0xBA,3,{0x24,0x24,0x24}},
{0xBC,3,{0x00,0x70,0x00}},
{0xBD,3,{0x00,0x70,0x00}},
{0xBE,1,{0x50}},
{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
{0xD1,16,{0x00,0x00,0x00,0x12,0x00,0x2F,0x00,0x47,0x00,0x5D,0x00,0x7F,0x00,0x9D,0x00,0xCC}},
{0xD2,16,{0x00,0xF2,0x01,0x30,0x01,0x62,0x01,0xAF,0x01,0xEE,0x01,0xEF,0x02,0x28,0x02,0x63}},
{0xD3,16,{0x02,0x89,0x02,0xBB,0x02,0xDE,0x03,0x0D,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x87}},
{0xD4,4,{0x03,0xA5,0x03,0xFF}},
{0xD5,16,{0x00,0x00,0x00,0x12,0x00,0x2F,0x00,0x47,0x00,0x5D,0x00,0x7F,0x00,0x9D,0x00,0xCC}},
{0xD6,16,{0x00,0xF2,0x01,0x30,0x01,0x62,0x01,0xAF,0x01,0xEE,0x01,0xEF,0x02,0x28,0x02,0x63}},
{0xD7,16,{0x02,0x89,0x02,0xBB,0x02,0xDE,0x03,0x0D,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x87}},
{0xD8,4,{0x03,0xA5,0x03,0xFF}},
{0xD9,16,{0x00,0x00,0x00,0x12,0x00,0x2F,0x00,0x47,0x00,0x5D,0x00,0x7F,0x00,0x9D,0x00,0xCC}},
{0xDD,16,{0x00,0xF2,0x01,0x30,0x01,0x62,0x01,0xAF,0x01,0xEE,0x01,0xEF,0x02,0x28,0x02,0x63}},
{0xDE,16,{0x02,0x89,0x02,0xBB,0x02,0xDE,0x03,0x0D,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x87}},
{0xDF,4,{0x03,0xA5,0x03,0xFF}},
{0xE0,16,{0x00,0x00,0x00,0x12,0x00,0x2F,0x00,0x47,0x00,0x5D,0x00,0x7F,0x00,0x9D,0x00,0xCC}},
{0xE1,16,{0x00,0xF2,0x01,0x30,0x01,0x62,0x01,0xAF,0x01,0xEE,0x01,0xEF,0x02,0x28,0x02,0x63}},
{0xE2,16,{0x02,0x89,0x02,0xBB,0x02,0xDE,0x03,0x0D,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x87}},
{0xE3,4,{0x03,0xA5,0x03,0xFF}},
{0xE4,16,{0x00,0x00,0x00,0x12,0x00,0x2F,0x00,0x47,0x00,0x5D,0x00,0x7F,0x00,0x9D,0x00,0xCC}},
{0xE5,16,{0x00,0xF2,0x01,0x30,0x01,0x62,0x01,0xAF,0x01,0xEE,0x01,0xEF,0x02,0x28,0x02,0x63}},
{0xE6,16,{0x02,0x89,0x02,0xBB,0x02,0xDE,0x03,0x0D,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x87}},
{0xE7,4,{0x03,0xA5,0x03,0xFF}},
{0xE8,16,{0x00,0x00,0x00,0x12,0x00,0x2F,0x00,0x47,0x00,0x5D,0x00,0x7F,0x00,0x9D,0x00,0xCC}},
{0xE9,16,{0x00,0xF2,0x01,0x30,0x01,0x62,0x01,0xAF,0x01,0xEE,0x01,0xEF,0x02,0x28,0x02,0x63}},
{0xEA,16,{0x02,0x89,0x02,0xBB,0x02,0xDE,0x03,0x0D,0x03,0x2B,0x03,0x51,0x03,0x6A,0x03,0x87}},
{0xEB,4,{0x03,0xA5,0x03,0xFF}},
{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},
{0xC5,5,{0x02,0x7C,0x00,0xFF,0x0A}},
{0xC6,8,{0x85,0xD6,0x8D,0x66,0x22,0x32,0xFF,0x0F}},
{0xC7,17,{0x10,0x52,0x97,0xFB,0x00,0x00,0xA0,0xBA,0x0B,0x00,0x60,0x00,0x88,0x88,0xF8,0xDB,0xFF}},
{0xFE,7,{0x00,0x10,0x00,0x94,0x05,0x00,0x4A}},
{0xF3,4,{0x10,0x5B,0x99,0x4F}},
{0xF6,3,{0xC0,0x90,0x60}},
{0xF1,3,{0x22,0x22,0x32}},
{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
{0x35,1,{0x00}},
{0x11,0,{0x00}},
{REGFLAG_DELAY, 120, {}},
{0x29,0,{0x00}},
{REGFLAG_DELAY, 100, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 150, {}},
	// Display ON
	//{0x2C, 1, {0x00}},
	//{0x13, 1, {0x00}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 200, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
    {REGFLAG_DELAY, 100, {}},

	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	{0x4f, 1, {0x01}},
	{REGFLAG_DELAY, 120, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_compare_id_setting[] = {
	// Display off sequence
	{0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x01}},
	{REGFLAG_DELAY, 10, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));
	params->type   = LCM_TYPE_DSI;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	params->dbi.te_mode 			= LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;



	params->dsi.mode   = SYNC_EVENT_VDO_MODE;


	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;
	//The following defined the fomat for data coming from LCD engine. 
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST; 
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;
	// Video mode setting		
	params->dsi.intermediat_buffer_num = 2;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    params->dsi.vertical_sync_active				= 4;//8;
    params->dsi.vertical_backporch					= 4;//18;
    params->dsi.vertical_frontporch					= 4;//18;	
 //   params->dsi.vertical_blanking_pixel				= 60;
    params->dsi.vertical_active_line				= FRAME_HEIGHT;
    
    params->dsi.horizontal_sync_active				= 10;//10;
    params->dsi.horizontal_backporch				= 32;//37;
    params->dsi.horizontal_frontporch				= 32;//37;
//	params->dsi.horizontal_blanking_pixel		       = 60;
	params->dsi.horizontal_active_pixel		       = FRAME_WIDTH;
	// Bit rate calculation
#if 0
	params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4
	params->dsi.pll_div2=1;		// div2=0,1,2,3;div2_real=1,2,4,4
//	params->dsi.fbk_sel=1;		 // fbk_sel=0,1,2,3;fbk_sel_real=1,2,4,4
	params->dsi.fbk_div =18;		//33 30 fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
#else
	params->dsi.PLL_CLOCK=234;
#endif
} 

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(50);
    SET_RESET_PIN(1);
    MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);


#ifdef BUILD_LK
	printf("[erick-lk]%s\n", __func__);
#else
	printk("[erick-k]%s\n", __func__);
#endif
}


static void lcm_suspend(void)
{
#ifndef BUILD_LK
//	SET_RESET_PIN(1);
//	MDELAY(10);
//	SET_RESET_PIN(0);
//	MDELAY(50);
//	SET_RESET_PIN(1);

//	MDELAY(120);

	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);	//wqtao. enable
     
 //    SET_RESET_PIN(0);
 //    MDELAY(120);

	#ifdef BUILD_LK
		printf("[erick-lk]%s\n", __func__);
	#else
		printk("[erick-k]%s\n", __func__);
	#endif
#endif
}

static void lcm_resume(void)
{
#ifndef BUILD_LK
	lcm_init();

	//push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);

	#ifdef BUILD_LK
		printf("[erick-lk]%s\n", __func__);
	#else
		printk("[erick-k]%s\n", __func__);
	#endif
#endif
}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];


	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00290508; //HW bug, so need send one HS packet
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
#endif

#if 0	//wqtao.		
static void lcm_setbacklight(unsigned int level)
{
	unsigned int default_level = 145;
	unsigned int mapped_level = 0;

	//for LGE backlight IC mapping table
	if(level > 255) 
			level = 255;

	if(level >0) 
			mapped_level = default_level+(level)*(255-default_level)/(255);
	else
			mapped_level=0;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#endif

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_UBOOT
	if(lcm_esd_test)
	{
	    lcm_esd_test = FALSE;
	    return TRUE;
	}

	/// please notice: the max return packet size is 1
	/// if you want to change it, you can refer to the following marked code
	/// but read_reg currently only support read no more than 4 bytes....
	/// if you need to read more, please let BinHan knows.
	/*
	        unsigned int data_array[16];
	        unsigned int max_return_size = 1;
	        
	        data_array[0]= 0x00003700 | (max_return_size << 16);    
	        
	        dsi_set_cmdq(&data_array, 1, 1);
	*/

	if(read_reg(0x0a) == 0x9c)
	{
	    return FALSE;
	}
	else
	{            
	    return TRUE;
	}
#endif
}

static unsigned int lcm_esd_recover(void)
{
    unsigned char para = 0;

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(120);
	  push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(10);
	  push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(10);
    dsi_set_cmdq_V2(0x35, 1, &para, 1);     ///enable TE
    MDELAY(10);

    return TRUE;
}

extern void DSI_clk_HS_mode(unsigned char enter);
static unsigned int lcm_compare_id(void)
{
	unsigned int id0 = 0, id1 = 0;
	unsigned char buffer[2];

	unsigned int data_array[16];

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);
/*
	DSI_clk_HS_mode(1);
	MDELAY(50);
	DSI_clk_HS_mode(0);
	MDELAY(5);

	data_array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10); 
	read_reg_v2(0xDB, &id0, 1);
	
#ifdef BUILD_LK
	printf("@@@@ db = 0x%08x\n", id0);
#endif
*/
/*	
	data_array[0] = 0x00110500;		// Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
*/
		
//*************Enable CMD2 Page1  *******************//
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(10); 

	data_array[0] = 0x00023700;// read id return two byte,version and id
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10); 
	
	read_reg_v2(0xC5, buffer, 2);
	id0 = buffer[0]; //we only need ID
	id1 = buffer[1]; //we test buffer 1
    //id0 = (id0<<8);
	//id0 += id1;
#ifdef BUILD_LK
	printf("rm68191 id = 0x%08x\n", id0);
#else
	printk("rm68191 id = 0x%08x\n", id0);
#endif	

	return (0X81 == id0)?1:0;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_rm68191_dsi_vdo_qhd_boe = 
{
	.name			= "hct_rm68191_dsi_vdo_qhd_boe",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,	
	.compare_id    = lcm_compare_id,	
#if (LCM_DSI_CMD_MODE)
	//.set_backlight	= lcm_setbacklight,
    //.esd_check   = lcm_esd_check,	
    //.esd_recover   = lcm_esd_recover,	
    .update         = lcm_update,
#endif	//wqtao
};
