/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*****************************************************************************
 *
 * Filename:
 * ---------
 *     OV48B2Qmipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 * Setting version:
 * ------------
 *   update full pd setting for OV48B2QEB_03B
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#define PFX "OV48B2Q_camera_sensor"

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "imgsensor_common.h"

#include "imgsensor_eeprom.h"
#include "ov48bmipiraw_Sensor.h"
#include "ov48b_Sensor_setting.h"
#include "ov48b_Sensor_setting_20151.h"
#ifdef OPLUS_FEATURE_CAMERA_COMMON
#include <soc/oplus/system/oplus_project.h>
#endif

#define LOG_INF(format, args...)    \
	pr_debug(PFX "[%s] " format, __func__, ##args)

#define MULTI_WRITE 1

#define FPT_PDAF_SUPPORT 1

#ifdef OPLUS_FEATURE_CAMERA_COMMON
/*Wenjun.Wu@Camera.Driver add for 19131 ,20200211 */
//static kal_uint32 streaming_control(kal_bool enable);
static bool IsProject_20151 = false ;

//chenyanrong@CAMERA.DRV.2022/12/19,add for ITS--sensor_fusion,Modify for OFFSET
#define DOMESTIC_OFFSET -375375
#define EXPORT_OFFSET -1055400
//chenyanrong@CAMERA.DRV.2022/12/19,add for ITS--sensor_fusion,Modify for OFFSET

//wangyang@CAMERA.DRV.2020/11/02,Modify for InSensorZoom
#define _I2C_BUF_SIZE 4096
kal_uint16 _i2c_data [_I2C_BUF_SIZE];
static unsigned int _size_to_write;
static bool _is_seamless = false;
#endif
static DEFINE_SPINLOCK(imgsensor_drv_lock);

static struct imgsensor_info_struct imgsensor_info = {
	.sensor_id = OV48B_SENSOR_ID,

	.checksum_value = 0x81c2c910,//test_Pattern_mode

	.pre = {
		.pclk = 115200000,
		.linelength = 1200,
		.framelength = 3200,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 300,
		.mipi_pixel_rate = 476000000,
	},
	.cap = {
		.pclk = 115200000,
		.linelength = 1200,
		.framelength = 3200,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 300,
		.mipi_pixel_rate = 476000000,
	},
	/*size@10fps, same as capture*/
#if 0
	.cap1 = {
		.pclk = 115200000,
		.linelength = 1872,
		.framelength = 6152,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 8000,
		.grabwindow_height = 6000,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 100,
		.mipi_pixel_rate = 584000000,
	},
#endif
	.normal_video = {
		.pclk = 115200000,
		.linelength = 1200,
		.framelength = 3200,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 2256,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 300,
		.mipi_pixel_rate = 476000000,
	},
	.hs_video = {
		.pclk = 115200000,
		.linelength = 576,
		.framelength = 1666,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2000,
		.grabwindow_height = 1128,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 1200,
        .mipi_pixel_rate = 528000000,
	},
	.slim_video = {
		.pclk = 115200000,
		.linelength = 1056,
		.framelength = 3636,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 2600,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 300,
		.mipi_pixel_rate = 584000000,
	},
  .custom1 = {
		.pclk = 115200000,
		.linelength = 1200,
		.framelength = 4000,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 240,
		.mipi_pixel_rate = 476000000,
	},
  .custom2 = {
		.pclk = 115200000,
		.linelength = 1200,
		.framelength = 3200,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 3840,
		.grabwindow_height = 2160,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 300,
		.mipi_pixel_rate = 476000000,
	},
	.custom3 = {
		.pclk = 115200000,
		.linelength = 1872,
		.framelength = 6152,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 8000,
		.grabwindow_height = 6000,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 100,
		.mipi_pixel_rate = 584000000,
	},
	.custom4 = {
		.pclk = 115200000,
		.linelength = 384,
		.framelength = 1254,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 2000,
		.grabwindow_height = 1128,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 2400,
		.mipi_pixel_rate = 716000000,
	  },
	.custom5 = {
		.pclk = 115200000,
		.linelength = 1152,
		.framelength = 3333,
		.startx = 0,
		.starty = 0,
		.grabwindow_width = 4000,
		.grabwindow_height = 3000,
		.mipi_data_lp2hs_settle_dc = 120,
		.max_framerate = 300,
		.mipi_pixel_rate = 476000000,
	},

	.margin = 22,					/* sensor framelength & shutter margin */
	.min_shutter = 4,				/* min shutter */
	.min_gain = 64, /*1x gain*/
	.max_gain = 4096, /*64x gain*/
	.min_gain_iso = 100,
	.gain_step = 1,
	.gain_type = 1,/*to be modify,no gain table for sony*/
	.max_frame_length = 0xffffe9,     /* max framelength by sensor register's limitation */
	.ae_shut_delay_frame = 0,		//check
	.ae_sensor_gain_delay_frame = 0,//check
	.ae_ispGain_delay_frame = 2,
	.ihdr_support = 0,
	.ihdr_le_firstline = 0,
	.sensor_mode_num = 10,			//support sensor mode num

	.cap_delay_frame = 3,			//enter capture delay frame num
	.pre_delay_frame = 2,			//enter preview delay frame num
	.video_delay_frame = 2,			//enter video delay frame num
	.hs_video_delay_frame = 2,		//enter high speed video  delay frame num
	.slim_video_delay_frame = 2,	//enter slim video delay frame num
	.custom1_delay_frame = 2,		//enter custom1 delay frame num
	.custom2_delay_frame = 2,		//enter custom2 delay frame num
	.custom3_delay_frame = 2,		//enter custom3 delay frame num
	.custom4_delay_frame = 2,		//enter custom4 delay frame num
	.custom5_delay_frame = 2,		//enter custom5 delay frame num
	.frame_time_delay_frame = 3,

	.isp_driving_current = ISP_DRIVING_8MA,
	.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
	.mipi_sensor_type = MIPI_OPHY_NCSI2,
	.mipi_settle_delay_mode = 1,
	.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_4CELL_HW_BAYER_B,
	.mclk = 24,//mclk value, suggest 24 or 26 for 24Mhz or 26Mhz
	.mipi_lane_num = SENSOR_MIPI_4_LANE,//mipi lane num
	.i2c_addr_table = {0x6d, 0xff},
	.i2c_speed = 1000,
};

static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_NORMAL,
	.sensor_mode = IMGSENSOR_MODE_INIT,
	.shutter = 0x3D0,
	.gain = 0x100,
	.dummy_pixel = 0,
	.dummy_line = 0,
	.current_fps = 30,
	.autoflicker_en = KAL_FALSE,
	.test_pattern = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,
	.ihdr_en = 0,
	.i2c_write_id = 0x20,
};

/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[10] = {
	{8000, 6000,    0,    0, 8000, 6000, 4000, 3000,  0,  0, 4000, 3000, 0, 0, 4000, 3000},     /* Preview check*/
	{8000, 6000,    0,    0, 8000, 6000, 4000, 3000,  0,  0, 4000, 3000, 0, 0, 4000, 3000},      /* capture */
	{8000, 6000,    0,  744, 8000, 4512, 4000, 2256,  0,  0, 4000, 2256, 0, 0, 4000, 2256},     /* video */
	{8000, 6000,    0,    0, 8000, 6000, 2000, 1500,  0,186, 2000, 1128, 0, 0, 2000, 1128},      /* hs vedio */
	{8000, 6000,    0,  400, 8000, 5200, 4000, 2600,  0,  0, 4000, 2600, 0, 0, 4000, 2600},     /* slim vedio */
	{8000, 6000,    0,    0, 8000, 6000, 4000, 3000,  0,  0, 4000, 3000, 0, 0, 4000, 3000},     /* Custom1 */
	{8000, 6000,  170,  840, 7680, 4320, 3840, 2160,  0,  0, 3840, 2160, 0, 0, 3840, 2160},    /* Custom2 */
	{8000, 6000,    0,    0, 8000, 6000, 8000, 6000,  0,  0, 8000, 6000, 0, 0, 8000, 6000},     /* Custom3 */
	{8000, 6000,    0,    0, 8000, 6000, 2000, 1500,  0,186, 2000, 1128, 0, 0, 2000, 1128},     /* Custom4 */
	{8000, 6000, 2000, 1500, 4000, 3000, 4000, 3000,  0,  0, 4000, 3000, 0, 0, 4000, 3000},     /* Custom5 */
};

static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[5] = {
    /* Preview,capture,custom1 mode setting 496(pxiel)*1496*/
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x0FA0, 0x0BB8, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x01F0, 0x05D8, 0x03, 0x00, 0x0000, 0x0000
    },
    /* Video mode setting 496(pxiel)*1128 */
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x0FA0, 0x08D0, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x01F0, 0x0468, 0x03, 0x00, 0x0000, 0x0000
    },
    /* Slim video ,custom5mode setting 496(pxiel)*1296*/
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x0FA0, 0x08D0, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x01F0, 0x0510, 0x03, 0x00, 0x0000, 0x0000
    },
     /* custom2 mode setting 480(pxiel)*1080*/
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x0FA0, 0x08D0, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x01E0, 0x0438, 0x03, 0x00, 0x0000, 0x0000
    },
    /* custom5 mode setting 496(pxiel)*748*/
    {
        0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
        0x00, 0x2b, 0x0FA0, 0x0BB8, 0x00, 0x00, 0x0280, 0x0001,
        0x01, 0x2b, 0x01F0, 0x02EC, 0x03, 0x00, 0x0000, 0x0000
    },
};


#if FPT_PDAF_SUPPORT
/*PD information update*/
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info = {
	 .i4OffsetX = 16,
	 .i4OffsetY = 4,
	 .i4PitchX = 16,
	 .i4PitchY = 16,
	 .i4PairNum = 8,
	 .i4SubBlkW = 8,
	 .i4SubBlkH = 4,
	 .i4PosL = {{23, 6}, {31, 6}, {19, 10}, {27, 10},
		{23, 14}, {31, 14}, {19, 18}, {27, 18} },
	 .i4PosR = {{22, 6}, {30, 6}, {18, 10}, {26, 10},
		{22, 14}, {30, 14}, {18, 18}, {26, 18} },
	 .iMirrorFlip = 0,
	 .i4BlockNumX = 248,
	 .i4BlockNumY = 187,
	 .i4Crop = { {0, 0}, {0, 0}, {0, 372}, {1000, 936}, {0, 200},
		{0, 0}, {80, 420}, {0, 0}, {1000, 936}, {0, 200} },
};
#endif

#if MULTI_WRITE
#define I2C_BUFFER_LEN 765	/*trans# max is 255, each 3 bytes*/
#else
#define I2C_BUFFER_LEN 3
#endif

static kal_uint16 ov48b2q_table_write_cmos_sensor(
					kal_uint16 *para, kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	tosend = 0;
	IDX = 0;
	while (len > IDX) {
		addr = para[IDX];
		{
			puSendCmd[tosend++] = (char)(addr >> 8);
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;
		}
#if MULTI_WRITE
		if ((I2C_BUFFER_LEN - tosend) < 3 ||
			len == IDX ||
			addr != addr_last) {
			iBurstWriteReg_multi(puSendCmd, tosend,
				imgsensor.i2c_write_id,
				3, imgsensor_info.i2c_speed);
			tosend = 0;
		}
#else
		iWriteRegI2C(puSendCmd, 3, imgsensor.i2c_write_id);
		tosend = 0;

#endif
	}
	return 0;
}

static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pusendcmd, 2, (u8 *)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pusendcmd[4] = {(char)(addr >> 8),
		(char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pusendcmd, 3, imgsensor.i2c_write_id);
}

static void set_dummy(void)
{
	if(!_is_seamless) {
		write_cmos_sensor(0x3208, 0x00);
		write_cmos_sensor(0x380c, imgsensor.line_length >> 8);
		write_cmos_sensor(0x380d, imgsensor.line_length & 0xFF);
		write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x3208, 0x10);
		write_cmos_sensor(0x3208, 0xa0);
	} else {
		_i2c_data[_size_to_write++] = 0x3840;
		_i2c_data[_size_to_write++] = imgsensor.frame_length >> 16;
		_i2c_data[_size_to_write++] = 0x380e;
		_i2c_data[_size_to_write++] = imgsensor.frame_length >> 8;
		_i2c_data[_size_to_write++] = 0x380f;
		_i2c_data[_size_to_write++] = imgsensor.frame_length & 0xFF;
	}
}

static void set_max_framerate(UINT16 framerate, kal_bool min_framelength_en)
{
	kal_uint32 frame_length = imgsensor.frame_length;

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;

	spin_lock(&imgsensor_drv_lock);
	imgsensor.frame_length = (frame_length > imgsensor.min_frame_length) ?
			frame_length : imgsensor.min_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length -
		imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
	imgsensor.frame_length = imgsensor_info.max_frame_length;
	imgsensor.dummy_line = imgsensor.frame_length -
		imgsensor.min_frame_length;
	}
	if (min_framelength_en)
	imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
}

static void set_max_framerate_video(UINT16 framerate, kal_bool min_framelength_en)
{
	set_max_framerate(framerate, min_framelength_en);
	set_dummy();
}

static void write_shutter(kal_uint32 shutter)
{
	kal_uint16 realtime_fps = 0;

	// OV Recommend Solution
	// if shutter bigger than frame_length, should extend frame length first
	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin)
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	else
		imgsensor.frame_length = imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length)
	imgsensor.frame_length = imgsensor_info.max_frame_length;
	spin_unlock(&imgsensor_drv_lock);

	shutter = (shutter < imgsensor_info.min_shutter) ?
			imgsensor_info.min_shutter : shutter;
	shutter = (shutter >
			(imgsensor_info.max_frame_length - imgsensor_info.margin)) ?
			(imgsensor_info.max_frame_length - imgsensor_info.margin) :
			shutter;

	//frame_length and shutter should be an even number.
	//shutter = (shutter >> 1) << 1;
	//imgsensor.frame_length = (imgsensor.frame_length >> 1) << 1;

	if(imgsensor.autoflicker_en == KAL_TRUE){
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 /
			imgsensor.frame_length;
		if(realtime_fps >= 297 && realtime_fps <= 305) {
			realtime_fps = 296;
			set_max_framerate(realtime_fps, 0);
		}else if(realtime_fps >= 147 && realtime_fps <= 150) {
			realtime_fps = 146;
			set_max_framerate(realtime_fps, 0);
		}else{
		}
	}

    /*Warning : shutter must be even. Odd might happen Unexpected Results */
	if(!_is_seamless) {
		write_cmos_sensor(0x3208, 0x01);
		write_cmos_sensor(0x3840, imgsensor.frame_length >> 16);
		write_cmos_sensor(0x380e, imgsensor.frame_length >> 8);
		write_cmos_sensor(0x380f, imgsensor.frame_length & 0xFF);
		write_cmos_sensor(0x3500, (shutter >> 16) & 0xFF);
		write_cmos_sensor(0x3501, (shutter >> 8) & 0xFF);
		write_cmos_sensor(0x3502, (shutter)  & 0xFF);
		write_cmos_sensor(0x3208, 0x11);
		write_cmos_sensor(0x3208, 0xa1);
	} else {
		_i2c_data[_size_to_write++] = 0x3840;
		_i2c_data[_size_to_write++] = imgsensor.frame_length >> 16;
		_i2c_data[_size_to_write++] = 0x380e;
		_i2c_data[_size_to_write++] = imgsensor.frame_length >> 8;
		_i2c_data[_size_to_write++] = 0x380f;
		_i2c_data[_size_to_write++] = imgsensor.frame_length & 0xFF;
		_i2c_data[_size_to_write++] = 0x3500;
		_i2c_data[_size_to_write++] = (shutter >> 16) & 0xFF;
		_i2c_data[_size_to_write++] = 0x3501;
		_i2c_data[_size_to_write++] = (shutter >> 8) & 0xFF;
		_i2c_data[_size_to_write++] = 0x3502;
		_i2c_data[_size_to_write++] = (shutter)  & 0xFF;
	}
	pr_debug("shutter =%d, framelength =%d, realtime_fps =%d _is_seamless %d\n",
	        shutter, imgsensor.frame_length, realtime_fps, _is_seamless);
}

static void set_shutter(kal_uint32 shutter)  //should not be kal_uint16 -- can't reach long exp
{
	unsigned long flags;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);
	write_shutter(shutter);
}

static kal_uint16 gain2reg(const kal_uint16 gain)
{
	kal_uint16 iReg = 0x0000;

	//platform 1xgain = 64, sensor driver 1*gain = 0x100
	iReg = gain*256/BASEGAIN;

	if(iReg < 0x100)	//sensor 1xGain
	{
		iReg = 0X100;
	}
	if(iReg > 0x3fff)	//sensor 64xGain
	{
		iReg = 0x3fff;
	}
	return iReg;		/* sensorGlobalGain */
}

static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain, max_gain = imgsensor_info.max_gain;
	unsigned long flags;


	if (gain < imgsensor_info.min_gain || gain > max_gain) {
		pr_debug("Error gain setting");

		if (gain < imgsensor_info.min_gain)
			gain = imgsensor_info.min_gain;
		else if (gain > max_gain)
			gain = max_gain;
	}

	reg_gain = gain2reg(gain);
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.gain = reg_gain;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	printk("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	if (reg_gain > 0xf00) {
		//15xA gain with digital gain
		write_cmos_sensor(0x03508, 0xf);
		write_cmos_sensor(0x03509, 0x00);
		reg_gain = reg_gain / 15 * 4 ; // calculate 15xAgain+ Dgain
		write_cmos_sensor(0x0350A, (reg_gain >> 10 )& 0xF);
		write_cmos_sensor(0x0350B, (reg_gain >> 2) & 0xFF);
		write_cmos_sensor(0x0350C, (reg_gain & 0x3) << 6);
	} else {
		write_cmos_sensor(0x03508, (reg_gain >> 8));
		write_cmos_sensor(0x03509, (reg_gain&0xff));
		write_cmos_sensor(0x0350A, 0x01);
		write_cmos_sensor(0x0350B, 0x00);
		write_cmos_sensor(0x0350C, 0x00);
	}

	return gain;
}

/* ITD: Modify Dualcam By Jesse 190924 Start */
static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 target_frame_length, kal_bool auto_extend_en)
{

	spin_lock(&imgsensor_drv_lock);
	if(target_frame_length > 1)
		imgsensor.dummy_line = target_frame_length - imgsensor.frame_length;
	imgsensor.frame_length = imgsensor.frame_length + imgsensor.dummy_line;
	imgsensor.min_frame_length = imgsensor.frame_length;
	spin_unlock(&imgsensor_drv_lock);
	set_shutter(shutter);
}
/* ITD: Modify Dualcam By Jesse 190924 End */

static void ihdr_write_shutter_gain(kal_uint16 le,
				kal_uint16 se, kal_uint16 gain)
{
}

static void night_mode(kal_bool enable)
{
}

static void sensor_init(void)
{
	write_cmos_sensor(0x0103, 0x01);//SW Reset, need delay
	mdelay(5);
	printk("sensor_init start\n");
	if(IsProject_20151){
		ov48b2q_table_write_cmos_sensor(
			addr_data_pair_init_ov48b2q_20151,
			sizeof(addr_data_pair_init_ov48b2q_20151) / sizeof(kal_uint16));
	}else{
		ov48b2q_table_write_cmos_sensor(
			addr_data_pair_init_ov48b2q,
			sizeof(addr_data_pair_init_ov48b2q) / sizeof(kal_uint16));
	}
	printk("sensor_init end\n");
}

static void preview_setting(void)
{
    int _length = 0;
    printk("preview_setting RES_4000x3000_30fps\n");
    _length = sizeof(addr_data_pair_preview_ov48b2q_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_preview_ov48b2q_20151,
                sizeof(addr_data_pair_preview_ov48b2q_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_preview_ov48b2q,
                sizeof(addr_data_pair_preview_ov48b2q) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_preview_ov48b2q_20151,
            sizeof(addr_data_pair_preview_ov48b2q_20151));
        _size_to_write += _length;
    }
    pr_debug("preview_setting end\n");
}

static void capture_setting(kal_uint16 currefps)
{
    int _length = 0;
    printk("capture_setting currefps = %d\n",currefps);
    _length = sizeof(addr_data_pair_capture_ov48b2q_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_capture_ov48b2q_20151,
                sizeof(addr_data_pair_capture_ov48b2q_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_capture_ov48b2q,
                sizeof(addr_data_pair_capture_ov48b2q) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_capture_ov48b2q_20151,
            sizeof(addr_data_pair_capture_ov48b2q_20151));
        _size_to_write += _length;
    }
    pr_debug("capture_setting end\n");
}

static void normal_video_setting(kal_uint16 currefps)
{
    int _length = 0;
    printk("normal_video_setting RES_4000x3000_zsl_30fps\n");
    _length = sizeof(addr_data_pair_video_ov48b2q_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_video_ov48b2q_20151,
                sizeof(addr_data_pair_video_ov48b2q_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_video_ov48b2q,
                sizeof(addr_data_pair_video_ov48b2q) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_video_ov48b2q_20151,
            sizeof(addr_data_pair_video_ov48b2q_20151));
        _size_to_write += _length;
    }
    pr_debug("normal_video_setting end\n");
}

static void hs_video_setting(void)
{
    int _length = 0;
    printk("hs_video_setting RES_1280x720_160fps\n");
    _length = sizeof(addr_data_pair_hs_video_ov48b2q_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_hs_video_ov48b2q_20151,
                sizeof(addr_data_pair_hs_video_ov48b2q_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_hs_video_ov48b2q,
                sizeof(addr_data_pair_hs_video_ov48b2q) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_hs_video_ov48b2q_20151,
            sizeof(addr_data_pair_hs_video_ov48b2q_20151));
        _size_to_write += _length;
    }
    pr_debug("hs_video_setting end\n");
}

static void slim_video_setting(void)
{
    int _length = 0;
    printk("slim_video_setting RES_3840x2160_30fps\n");
    _length = sizeof(addr_data_pair_slim_video_ov48b2q_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_slim_video_ov48b2q_20151,
                sizeof(addr_data_pair_slim_video_ov48b2q_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_slim_video_ov48b2q,
                sizeof(addr_data_pair_slim_video_ov48b2q) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_slim_video_ov48b2q_20151,
            sizeof(addr_data_pair_slim_video_ov48b2q_20151));
        _size_to_write += _length;
    }
    pr_debug("slim_video_setting end\n");
}

/* ITD: Modify Dualcam By Jesse 190924 Start */
static void custom1_setting(void)
{
    int _length = 0;
    printk("custom1_setting start\n");
    _length = sizeof(addr_data_pair_custom1_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom1_20151,
                sizeof(addr_data_pair_custom1_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom1,
                sizeof(addr_data_pair_custom1) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_custom1_20151,
            sizeof(addr_data_pair_custom1_20151));
        _size_to_write += _length;
    }
    pr_debug("custom1_setting end\n");
}	/*	custom1_setting  */

static void custom2_setting(void)
{
    int _length = 0;
    printk("custom2_setting start\n");
    _length = sizeof(addr_data_pair_custom2_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom2_20151,
                sizeof(addr_data_pair_custom2_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom2,
                sizeof(addr_data_pair_custom2) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_custom2_20151,
            sizeof(addr_data_pair_custom2_20151));
        _size_to_write += _length;
    }
    pr_debug("custom2_setting end\n");
}	/*	custom2_setting  */

static void custom3_setting(void)
{
    int _length = 0;
    printk("custom3_setting start\n");
    _length = sizeof(addr_data_pair_custom3_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom3_20151,
                sizeof(addr_data_pair_custom3_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom3,
                sizeof(addr_data_pair_custom3) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_custom3_20151,
            sizeof(addr_data_pair_custom3_20151));
        _size_to_write += _length;
    }
    pr_debug("custom3_setting end\n");
}	/*	custom3_setting  */

static void custom4_setting(void)
{
    int _length = 0;
    printk("custom4_setting start\n");
    _length = sizeof(addr_data_pair_custom4_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom4_20151,
                sizeof(addr_data_pair_custom4_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom4,
                sizeof(addr_data_pair_custom4) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_custom4_20151,
            sizeof(addr_data_pair_custom4_20151));
        _size_to_write += _length;
    }
    pr_debug("custom4_setting end\n");
}	/*	custom4_setting  */

static void custom5_setting(void)
{
    int _length = 0;
    printk("custom5_setting start\n");
    _length = sizeof(addr_data_pair_custom5_20151) / sizeof(kal_uint16);

    if(!_is_seamless) {
        if(IsProject_20151){
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom5_20151,
                sizeof(addr_data_pair_custom5_20151) / sizeof(kal_uint16));
        }else{
            ov48b2q_table_write_cmos_sensor(
                addr_data_pair_custom5,
                sizeof(addr_data_pair_custom5) / sizeof(kal_uint16));
        }
    } else {
        pr_debug("%s _is_seamless %d, _size_to_write %d\n",
            __func__, _is_seamless, _size_to_write);

        if (_size_to_write + _length > _I2C_BUF_SIZE) {
            pr_err("_too much i2c data for fast siwtch %d\n",
                _size_to_write + _length);
            return;
        }
        memcpy((void *) (_i2c_data + _size_to_write),
            addr_data_pair_custom5_20151,
            sizeof(addr_data_pair_custom5_20151));
        _size_to_write += _length;
    }
    pr_debug("custom5_setting end\n");
}	/*	custom5_setting  */

static kal_uint16 ov48b2q_burst_write_cmos_sensor(
					kal_uint16 *para, kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	tosend = 0;
	IDX = 0;
	while (len > IDX) {
		addr = para[IDX];
		if(tosend==0)
		{
			puSendCmd[tosend++] = (char)(addr >> 8);
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;
		}
		else if(addr == addr_last+1)
		{
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data & 0xFF);
			addr_last = addr;
			IDX += 2;
		}

		if (( tosend>=I2C_BUFFER_LEN)||
			len == IDX ||
			addr != addr_last) {
			iBurstWriteReg_multi(puSendCmd, tosend,
				imgsensor.i2c_write_id,
				tosend, imgsensor_info.i2c_speed);
			tosend = 0;
		}
	}
	return 0;
}

/*OVPD-1:720Bytes & Crosstalk 288Bytes*/
static kal_uint16 ov48b_QSC_OVPD_setting[8*2];
static kal_uint16 ov48b_QSC_OVPD_setting_burst[720*2];
static kal_uint16 ov48b_QSC_CT_setting[288*2];
static kal_uint8 ct_qsc_flag = 0;
static kal_uint8 crossTalk_flag = 0;

static void read_EepromQSC(void)
{
    kal_uint16 addr_ovpd = 0x24A4, sensor_PDC_addr1 = 0x5C0E, sensor_PDC_addr2 = 0x5900;
    kal_uint16 addr_crotalk = 0x0D00, senaddr_crotalk = 0x53c0;
    kal_uint16 i = 0;
    kal_uint32 Dac_master = 0, Dac_mac = 0, Dac_inf = 0;
    /*Read OVPD*/
    for (i = 0; i < 8; i ++) {
        ov48b_QSC_OVPD_setting[2*i] = sensor_PDC_addr1+i;
        ov48b_QSC_OVPD_setting[2*i+1] = Eeprom_1ByteDataRead((addr_ovpd+i), 0xA0);
    }
    for (i = 0; i < 720; i ++) {
        ov48b_QSC_OVPD_setting_burst[2*i] = sensor_PDC_addr2+i;
        ov48b_QSC_OVPD_setting_burst[2*i+1] = Eeprom_1ByteDataRead((addr_ovpd+8+i), 0xA0);
    }
    /*Read crosstalk*/
    for (i = 0; i < 288; i ++) {
        ov48b_QSC_CT_setting[2*i] = senaddr_crotalk+i;
        ov48b_QSC_CT_setting[2*i+1] = Eeprom_1ByteDataRead((addr_crotalk+i), 0xA0);
    }
    /*Read normal eeprom data*/
    gImgEepromInfo.camNormdata[0][0] = Eeprom_1ByteDataRead(0x00, 0xA0);
    gImgEepromInfo.camNormdata[0][1] = Eeprom_1ByteDataRead(0x01, 0xA0);
    imgsensor_info.module_id = Eeprom_1ByteDataRead(0x00, 0xA0);
    Oplusimgsensor_Registdeviceinfo(gImgEepromInfo.pCamModuleInfo[0].name,
                                    gImgEepromInfo.pCamModuleInfo[0].version,
                                    imgsensor_info.module_id);
    for(i = 2; i < 8; i++) {
       gImgEepromInfo.camNormdata[0][i] = Eeprom_1ByteDataRead(0x04+i, 0xA0);
    }
    for (i = 0; i < OPLUS_CAMERASN_LENS; i ++) {
       gImgEepromInfo.camNormdata[0][8+i] = Eeprom_1ByteDataRead(0xB0+i, 0xA0);
    }
    Dac_mac = (Eeprom_1ByteDataRead(0x93, 0xA0) << 8) | Eeprom_1ByteDataRead(0x92, 0xA0);
    Dac_inf = (Eeprom_1ByteDataRead(0x95, 0xA0) << 8) | Eeprom_1ByteDataRead(0x94, 0xA0);
    Dac_master = (5*Dac_mac+36*Dac_inf)/41;
    pr_info("Dac_inf:%d Dac_Mac:%d Dac_master:%d\n", Dac_inf, Dac_mac, Dac_master);
    memcpy(&gImgEepromInfo.camNormdata[2][28], &Dac_master, 4);
    /*Read stereo eeprom data*/
    for (i = 0; i < CALI_DATA_MASTER_LENGTH; i ++) {
       gImgEepromInfo.stereoMWdata[i] = Eeprom_1ByteDataRead(OV48B_STEREO_START_ADDR+i, 0xA0);
    }
    gImgEepromInfo.i4CurSensorIdx = 0;
    gImgEepromInfo.i4CurSensorId = imgsensor_info.sensor_id;
}

static void write_sensor_OVPD_QSC(void)
{
    pr_info("%s start\n", __func__);

    ov48b2q_table_write_cmos_sensor(ov48b_QSC_OVPD_setting,
        sizeof(ov48b_QSC_OVPD_setting) / sizeof(kal_uint16));
    ov48b2q_burst_write_cmos_sensor(ov48b_QSC_OVPD_setting_burst,
        sizeof(ov48b_QSC_OVPD_setting_burst) / sizeof(kal_uint16));

    pr_info("%s end\n", __func__);
}

static void write_sensor_CT_QSC(void)
{
    pr_info("%s start\n", __func__);

    if( !ct_qsc_flag ) {
        ov48b2q_table_write_cmos_sensor(ov48b_QSC_CT_setting,
            sizeof(ov48b_QSC_CT_setting) / sizeof(kal_uint16));
        ct_qsc_flag = 1;
    }

    pr_info("%s end\n", __func__);
}

static void write_sensor_crossTalk_data(void)
{
    pr_info("%s start\n", __func__);

    if( !crossTalk_flag ) {
        ov48b2q_burst_write_cmos_sensor(addr_data_pair_crossTalk_ov48b2q_20151,
            sizeof(addr_data_pair_crossTalk_ov48b2q_20151) / sizeof(kal_uint16));
        crossTalk_flag = 1;
    }

    pr_info("%s end\n", __func__);
}

static kal_uint32 return_sensor_id(void)
{
	return ((read_cmos_sensor(0x300a) << 16) |
		(read_cmos_sensor(0x300b) << 8) | read_cmos_sensor(0x300c));
}

static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
    kal_uint8 i = 0;
    kal_uint8 retry = 2;

    while (imgsensor_info.i2c_addr_table[i] != 0xff) {
    spin_lock(&imgsensor_drv_lock);
    imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
    spin_unlock(&imgsensor_drv_lock);
    do {
        *sensor_id = return_sensor_id();
        if (*sensor_id == imgsensor_info.sensor_id) {
            printk("i2c write id: 0x%x, sensor id: 0x%x\n",
                imgsensor.i2c_write_id, *sensor_id);
            #ifdef OPLUS_FEATURE_CAMERA_COMMON
            read_EepromQSC();
            printk("RENM0_module_id=%d\n",imgsensor_info.module_id);
            #endif
            return ERROR_NONE;
        }
        retry--;
    } while (retry > 0);
    i++;
    retry = 1;
    }
    if (*sensor_id != imgsensor_info.sensor_id) {
        printk("get_imgsensor_id: 0x%x fail\n", *sensor_id);
        *sensor_id = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    return ERROR_NONE;
}

static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 1;
	kal_uint32 sensor_id = 0;

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
	spin_lock(&imgsensor_drv_lock);
	imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
	spin_unlock(&imgsensor_drv_lock);
	do {
		sensor_id = return_sensor_id();
	if (sensor_id == imgsensor_info.sensor_id) {
		printk("i2c write id: 0x%x, sensor id: 0x%x\n",
			imgsensor.i2c_write_id, sensor_id);
		break;
	}
		retry--;
	} while (retry > 0);
	i++;
	if (sensor_id == imgsensor_info.sensor_id)
	break;
	retry = 2;
	}
	if (imgsensor_info.sensor_id != sensor_id) {
		printk("Open sensor id: 0x%x fail\n", sensor_id);
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	#ifdef OPLUS_FEATURE_CAMERA_COMMON
	if(is_project(20151)||is_project(20301)||is_project(20302)||is_project(20392)){
		IsProject_20151 = true;
	}
	#endif
	sensor_init();
	#ifdef OPLUS_FEATURE_CAMERA_COMMON
	/*Wenjun.Wu@Camera.Driver add for writing PDC data ,20200226*/
	write_sensor_OVPD_QSC();
	#endif
	spin_lock(&imgsensor_drv_lock);
	imgsensor.autoflicker_en = KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_en = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static kal_uint32 close(void)
{
	ct_qsc_flag = 0;
	crossTalk_flag = 0;

	return ERROR_NONE;
}   /*  close  */

static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.current_fps = imgsensor.current_fps;
	//imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	return ERROR_NONE;
}

static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
		  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	imgsensor.pclk = imgsensor_info.cap.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	//imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	capture_setting(imgsensor.current_fps);
	return ERROR_NONE;
} /* capture() */

static kal_uint32 normal_video(
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	//imgsensor.current_fps = 300;
	//imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	normal_video_setting(imgsensor.current_fps);
	return ERROR_NONE;
}

static kal_uint32 hs_video(
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	hs_video_setting();
	return ERROR_NONE;
}

static kal_uint32 slim_video(
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	//imgsensor.video_mode = KAL_TRUE;
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	//imgsensor.current_fps = 300;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	return ERROR_NONE;
}

/* ITD: Modify Dualcam By Jesse 190924 Start */
static kal_uint32 Custom1(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM1;
    imgsensor.pclk = imgsensor_info.custom1.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom1.linelength;
    imgsensor.frame_length = imgsensor_info.custom1.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom1.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom1_setting();
    return ERROR_NONE;
}   /*  Custom1   */

static kal_uint32 Custom2(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    printk("E\n");

    spin_lock(&imgsensor_drv_lock);
    imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM2;
    imgsensor.pclk = imgsensor_info.custom2.pclk;
    //imgsensor.video_mode = KAL_FALSE;
    imgsensor.line_length = imgsensor_info.custom2.linelength;
    imgsensor.frame_length = imgsensor_info.custom2.framelength;
    imgsensor.min_frame_length = imgsensor_info.custom2.framelength;
    imgsensor.autoflicker_en = KAL_FALSE;
    spin_unlock(&imgsensor_drv_lock);
    custom2_setting();
    return ERROR_NONE;
}   /*  Custom2   */

static kal_uint32 Custom3(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	printk("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM3;
	imgsensor.pclk = imgsensor_info.custom3.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.custom3.linelength;
	imgsensor.frame_length = imgsensor_info.custom3.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom3.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	write_sensor_crossTalk_data();
	write_sensor_CT_QSC();
	custom3_setting();
	return ERROR_NONE;
}   /*  Custom3*/


static kal_uint32 Custom4(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	printk("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM4;
	imgsensor.pclk = imgsensor_info.custom4.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.custom4.linelength;
	imgsensor.frame_length = imgsensor_info.custom4.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom4.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	custom4_setting();
	return ERROR_NONE;
}   /*  Custom4	*/

static kal_uint32 Custom5(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	printk("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CUSTOM5;
	imgsensor.pclk = imgsensor_info.custom5.pclk;
	//imgsensor.video_mode = KAL_FALSE;
	imgsensor.line_length = imgsensor_info.custom5.linelength;
	imgsensor.frame_length = imgsensor_info.custom5.framelength;
	imgsensor.min_frame_length = imgsensor_info.custom5.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	//wangyang@CAMERA.DRV.2020/11/02,Modify for InSensorZoom
	write_sensor_crossTalk_data();
	write_sensor_CT_QSC();
	custom5_setting();
	return ERROR_NONE;
}/*	Custom5 */
/* ITD: Modify Dualcam By Jesse 190924 End */

static kal_uint32 get_resolution(
		MSDK_SENSOR_RESOLUTION_INFO_STRUCT * sensor_resolution)
{
	sensor_resolution->SensorFullWidth =
		imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight =
		imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth =
		imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight =
		imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth =
		imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight =
		imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth =
		imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight =
		imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth =
		imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight =
		imgsensor_info.slim_video.grabwindow_height;

/* ITD: Modify Dualcam By Jesse 190924 Start */
    sensor_resolution->SensorCustom1Width =
		imgsensor_info.custom1.grabwindow_width;
    sensor_resolution->SensorCustom1Height =
		imgsensor_info.custom1.grabwindow_height;

    sensor_resolution->SensorCustom2Width =
		imgsensor_info.custom2.grabwindow_width;
    sensor_resolution->SensorCustom2Height =
		imgsensor_info.custom2.grabwindow_height;

	sensor_resolution->SensorCustom3Width =
		imgsensor_info.custom3.grabwindow_width;
	sensor_resolution->SensorCustom3Height =
		imgsensor_info.custom3.grabwindow_height;

    sensor_resolution->SensorCustom4Width =
		imgsensor_info.custom4.grabwindow_width;
    sensor_resolution->SensorCustom4Height =
		imgsensor_info.custom4.grabwindow_height;

    sensor_resolution->SensorCustom5Width =
		imgsensor_info.custom5.grabwindow_width;
    sensor_resolution->SensorCustom5Height =
		imgsensor_info.custom5.grabwindow_height;


/* ITD: Modify Dualcam By Jesse 190924 End */

	return ERROR_NONE;
}   /*  get_resolution  */

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
		      MSDK_SENSOR_INFO_STRUCT *sensor_info,
		      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	if (scenario_id == 0)
	printk("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat =
		imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame =
		imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame =
		imgsensor_info.slim_video_delay_frame;

/* ITD: Modify Dualcam By Jesse 190924 Start */
    sensor_info->Custom1DelayFrame = imgsensor_info.custom1_delay_frame;
    sensor_info->Custom2DelayFrame = imgsensor_info.custom2_delay_frame;
    sensor_info->Custom3DelayFrame = imgsensor_info.custom3_delay_frame;
    sensor_info->Custom4DelayFrame = imgsensor_info.custom4_delay_frame;
    sensor_info->Custom5DelayFrame = imgsensor_info.custom5_delay_frame;
/* ITD: Modify Dualcam By Jesse 190924 End */

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;
/* The frame of setting shutter default 0 for TG int */
	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame;
	/* The frame of setting sensor gain */
	sensor_info->AESensorGainDelayFrame =
		imgsensor_info.ae_sensor_gain_delay_frame;
	sensor_info->AEISPGainDelayFrame =
		imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
#if FPT_PDAF_SUPPORT
/*0: NO PDAF, 1: PDAF Raw Data mode, 2:PDAF VC mode*/
	sensor_info->PDAF_Support = 2;
#else
	sensor_info->PDAF_Support = 0;
#endif

	//sensor_info->HDR_Support = 0; /*0: NO HDR, 1: iHDR, 2:mvHDR, 3:zHDR*/
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;  // 0 is default 1x
	sensor_info->SensorHightSampling = 0;   // 0 is default 1x
	sensor_info->SensorPacketECCOrder = 1;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

	break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

	break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

		sensor_info->SensorGrabStartX =
			imgsensor_info.normal_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.normal_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

	break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;
	break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		sensor_info->SensorGrabStartX =
			imgsensor_info.slim_video.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.slim_video.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;
	break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case MSDK_SCENARIO_ID_CUSTOM1:
		sensor_info->SensorGrabStartX =
			imgsensor_info.custom1.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.custom1.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom1.mipi_data_lp2hs_settle_dc;
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		sensor_info->SensorGrabStartX =
			imgsensor_info.custom2.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.custom2.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom2.mipi_data_lp2hs_settle_dc;
	break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		sensor_info->SensorGrabStartX =
			imgsensor_info.custom3.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.custom3.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom3.mipi_data_lp2hs_settle_dc;
	break;

	case MSDK_SCENARIO_ID_CUSTOM4:
		sensor_info->SensorGrabStartX =
			imgsensor_info.custom4.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.custom4.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom4.mipi_data_lp2hs_settle_dc;
	break;
	case MSDK_SCENARIO_ID_CUSTOM5:
		sensor_info->SensorGrabStartX =
			imgsensor_info.custom5.startx;
		sensor_info->SensorGrabStartY =
			imgsensor_info.custom5.starty;
		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.custom5.mipi_data_lp2hs_settle_dc;
	break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	default:
		sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
		sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

		sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount =
			imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
	break;
	}

	return ERROR_NONE;
}   /*  get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
			MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		preview(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		capture(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		normal_video(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		hs_video(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		slim_video(image_window, sensor_config_data);
	break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case MSDK_SCENARIO_ID_CUSTOM1:
		Custom1(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
		Custom2(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		Custom3(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_CUSTOM4:
		Custom4(image_window, sensor_config_data);
	break;
	case MSDK_SCENARIO_ID_CUSTOM5:
		Custom5(image_window, sensor_config_data);
	break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	default:
		printk("Error ScenarioId setting");
		preview(image_window, sensor_config_data);
	return ERROR_INVALID_SCENARIO_ID;
	}

	return ERROR_NONE;
}   /* control() */

static kal_uint32 set_video_mode(UINT16 framerate)
{
	// SetVideoMode Function should fix framerate
	if (framerate == 0)
	// Dynamic frame rate
	return ERROR_NONE;

	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE))
	imgsensor.current_fps = 296;
	else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE))
	imgsensor.current_fps = 146;
	else
	imgsensor.current_fps = framerate;
	spin_unlock(&imgsensor_drv_lock);

	set_max_framerate_video(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable,
			UINT16 framerate)
{
	printk("enable = %d, framerate = %d\n",
		enable, framerate);

	spin_lock(&imgsensor_drv_lock);
	if (enable) //enable auto flicker
	imgsensor.autoflicker_en = KAL_TRUE;
	else //Cancel Auto flick
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	return ERROR_NONE;
}

static kal_uint32 set_max_framerate_by_scenario(
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MUINT32 framerate)
{
	kal_uint32 frameHeight;

	printk("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	if (framerate == 0)
		return ERROR_NONE;

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	    frameHeight = imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frameHeight > imgsensor_info.pre.framelength) ?
			(frameHeight - imgsensor_info.pre.framelength):0;
	    imgsensor.frame_length = imgsensor_info.pre.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if (framerate == 0)
			return ERROR_NONE;
	    frameHeight = imgsensor_info.normal_video.pclk / framerate * 10 /
				imgsensor_info.normal_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.normal_video.framelength) ?
		(frameHeight - imgsensor_info.normal_video.framelength):0;
	    imgsensor.frame_length = imgsensor_info.normal_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    frameHeight = imgsensor_info.cap.pclk / framerate * 10 /
			imgsensor_info.cap.linelength;
	    spin_lock(&imgsensor_drv_lock);

		imgsensor.dummy_line =
			(frameHeight > imgsensor_info.cap.framelength) ?
			(frameHeight - imgsensor_info.cap.framelength):0;
	    imgsensor.frame_length = imgsensor_info.cap.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
	    frameHeight = imgsensor_info.hs_video.pclk / framerate * 10 /
			imgsensor_info.hs_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line =
			(frameHeight > imgsensor_info.hs_video.framelength) ?
			(frameHeight - imgsensor_info.hs_video.framelength):0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
	    frameHeight = imgsensor_info.slim_video.pclk / framerate * 10 /
			imgsensor_info.slim_video.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.slim_video.framelength) ?
			(frameHeight - imgsensor_info.slim_video.framelength):0;
	    imgsensor.frame_length = imgsensor_info.slim_video.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case MSDK_SCENARIO_ID_CUSTOM1:
	    frameHeight = imgsensor_info.custom1.pclk / framerate * 10 /
			imgsensor_info.custom1.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.custom1.framelength) ?
			(frameHeight - imgsensor_info.custom1.framelength):0;
	    imgsensor.frame_length = imgsensor_info.custom1.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
	    frameHeight = imgsensor_info.custom2.pclk / framerate * 10 /
			imgsensor_info.custom2.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.custom2.framelength) ?
			(frameHeight - imgsensor_info.custom2.framelength):0;
	    imgsensor.frame_length = imgsensor_info.custom2.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CUSTOM3:
		frameHeight = imgsensor_info.custom3.pclk / framerate * 10 /
			imgsensor_info.custom3.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.custom3.framelength) ?
			(frameHeight - imgsensor_info.custom3.framelength):0;
		imgsensor.frame_length = imgsensor_info.custom3.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CUSTOM4:
	    frameHeight = imgsensor_info.custom4.pclk / framerate * 10 /
			imgsensor_info.custom4.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.custom4.framelength) ?
			(frameHeight - imgsensor_info.custom4.framelength):0;
	    imgsensor.frame_length = imgsensor_info.custom4.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	case MSDK_SCENARIO_ID_CUSTOM5:
		frameHeight = imgsensor_info.custom5.pclk / framerate * 10 /
			imgsensor_info.custom5.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.custom5.framelength) ?
			(frameHeight - imgsensor_info.custom5.framelength):0;
		imgsensor.frame_length = imgsensor_info.custom5.framelength +
			imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	default:  //coding with  preview scenario by default
	    frameHeight = imgsensor_info.pre.pclk / framerate * 10 /
			imgsensor_info.pre.linelength;
	    spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frameHeight >
			imgsensor_info.pre.framelength) ?
			(frameHeight - imgsensor_info.pre.framelength):0;
	    imgsensor.frame_length = imgsensor_info.pre.framelength +
			imgsensor.dummy_line;
	    imgsensor.min_frame_length = imgsensor.frame_length;
	    spin_unlock(&imgsensor_drv_lock);
		if (imgsensor.frame_length > imgsensor.shutter)
			set_dummy();
	break;
	}
	return ERROR_NONE;
}

static kal_uint32 get_default_framerate_by_scenario(
			enum MSDK_SCENARIO_ID_ENUM scenario_id,
			MUINT32 *framerate)
{
	if (scenario_id == 0)
	printk("[3058]scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
	    *framerate = imgsensor_info.pre.max_framerate;
	break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	    *framerate = imgsensor_info.normal_video.max_framerate;
	break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    *framerate = imgsensor_info.cap.max_framerate;
	break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
	break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
	break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
	case MSDK_SCENARIO_ID_CUSTOM1:
	    *framerate = imgsensor_info.custom1.max_framerate;
	break;
	case MSDK_SCENARIO_ID_CUSTOM2:
	    *framerate = imgsensor_info.custom2.max_framerate;
	break;
	case MSDK_SCENARIO_ID_CUSTOM3:
	    *framerate = imgsensor_info.custom3.max_framerate;
	break;
	case MSDK_SCENARIO_ID_CUSTOM4:
	    *framerate = imgsensor_info.custom4.max_framerate;
	break;
	case MSDK_SCENARIO_ID_CUSTOM5:
	    *framerate = imgsensor_info.custom5.max_framerate;
	break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	default:
	break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
    LOG_INF("%s enable: %s", __func__, enable? "true": "false");

    if (enable) { // for solid color
        write_cmos_sensor(0x3019, 0xf0);
        write_cmos_sensor(0x4308, 0x01); //en
        write_cmos_sensor(0x4300, 0x00); //Gb
        write_cmos_sensor(0x4302, 0x00); //B
        write_cmos_sensor(0x4304, 0x00); //R
        write_cmos_sensor(0x4306, 0x00); //Gr
    } else {
        write_cmos_sensor(0x3019, 0xd2);
        write_cmos_sensor(0x4308, 0x00);
        write_cmos_sensor(0x4300, 0x00);
        write_cmos_sensor(0x4302, 0x00);
        write_cmos_sensor(0x4304, 0x00);
        write_cmos_sensor(0x4306, 0x00);
    }
    spin_lock(&imgsensor_drv_lock);
    imgsensor.test_pattern = enable;
    spin_unlock(&imgsensor_drv_lock);
    return ERROR_NONE;
}

static kal_uint32 get_sensor_temperature(void)
{
/*
	UINT32 temperature = 0;
	INT32 temperature_convert = 0;

	//TEMP_SEN_CTL
	write_cmos_sensor(0x4d12, 0x01);
	temperature = (read_cmos_sensor(0x4d13) << 8) |
		read_cmos_sensor(0x4d13);
	if (temperature < 0xc000)
		temperature_convert = temperature / 256;
	else
		temperature_convert = 192 - temperature / 256;

	if (temperature_convert > 192) {
		//printk("Temperature too high: %d\n",
				//temperature_convert);
		temperature_convert = 192;
	} else if (temperature_convert < -64) {
		//printk("Temperature too low: %d\n",
				//temperature_convert);
		temperature_convert = -64;
	}
*/
	return 20;
	//return temperature_convert;
}

static kal_uint32 seamless_switch(enum MSDK_SCENARIO_ID_ENUM scenario_id,
	kal_uint32 shutter, kal_uint32 gain,
	kal_uint32 shutter_2ndframe, kal_uint32 gain_2ndframe)
{
	int _length = 0;
//	int k = 0;
	_is_seamless = true;
	memset(_i2c_data, 0x0, sizeof(_i2c_data));
	_size_to_write = 0;

	pr_err("seamless_switch %d, %d, %d, %d, %d sizeof(_i2c_data) %d\n",
		scenario_id, shutter, gain, shutter_2ndframe, gain_2ndframe, sizeof(_i2c_data));

	_length = sizeof(addr_data_pair_seamless_switch_step1_ov48b2q) / sizeof(kal_uint16);

	if (_length> _I2C_BUF_SIZE) {
		pr_err("_too much i2c data for fast siwtch\n");
		return ERROR_NONE;
	}

	memcpy((void *)(_i2c_data + _size_to_write),
		addr_data_pair_seamless_switch_step1_ov48b2q,
		sizeof(addr_data_pair_seamless_switch_step1_ov48b2q));
	_size_to_write += _length;

	control(scenario_id, NULL, NULL);
	if(shutter != 0)
		set_shutter(shutter);
	if(gain != 0)
		set_gain(gain);

	_length = sizeof(addr_data_pair_seamless_switch_step2_ov48b2q) / sizeof(kal_uint16);

	if (_size_to_write + _length > _I2C_BUF_SIZE) {
		pr_err("_too much i2c data for fast siwtch\n");
		return ERROR_NONE;
	}

	memcpy((void *)(_i2c_data + _size_to_write),
		addr_data_pair_seamless_switch_step2_ov48b2q,
		sizeof(addr_data_pair_seamless_switch_step2_ov48b2q));
	_size_to_write += _length;

	if(shutter_2ndframe != 0)
		set_shutter(shutter_2ndframe);
	if(gain_2ndframe != 0)
		set_gain(gain_2ndframe);

	_length = sizeof(addr_data_pair_seamless_switch_step3_ov48b2q) / sizeof(kal_uint16);
	if (_size_to_write + _length > _I2C_BUF_SIZE) {
		pr_err("_too much i2c data for fast siwtch\n");
		return ERROR_NONE;
	}
	memcpy((void *)(_i2c_data + _size_to_write),
		addr_data_pair_seamless_switch_step3_ov48b2q,
		sizeof(addr_data_pair_seamless_switch_step3_ov48b2q));
	_size_to_write += _length;

	pr_debug("%s _is_seamless %d, _size_to_write %d\n",
			__func__, _is_seamless, _size_to_write);
#if 0
	for (k = 0; k <_size_to_write; k+=2) {
		pr_debug( "k = %d, 0x%x , 0x%x \n", k,  _i2c_data[k], _i2c_data[k+1]);
	}
#endif

	ov48b2q_table_write_cmos_sensor(
		_i2c_data,
		_size_to_write);

#if 0
	pr_debug("===========================================\n");

	for (k = 0; k <_size_to_write; k+=2) {
		pr_debug( "k = %d, 0x%x , 0x%x \n", k,  _i2c_data[k], read_cmos_sensor(_i2c_data[k]));
	}
#endif
	_is_seamless = false;
	pr_err("exit\n");
	return ERROR_NONE;
}

static kal_uint32 streaming_control(kal_bool enable)
{
	printk("streaming_enable(0=Sw Standby,1=streaming): %d\n", enable);
	if (enable)
		write_cmos_sensor(0x0100, 0X01);
	else
		write_cmos_sensor(0x0100, 0x00);
	return ERROR_NONE;
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
			UINT8 *feature_para, UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16 = (UINT16 *) feature_para;
	UINT16 *feature_data_16 = (UINT16 *) feature_para;
	UINT32 *feature_return_para_32 = (UINT32 *) feature_para;
	UINT32 *feature_data_32 = (UINT32 *) feature_para;
	INT32 *feature_return_para_i32 = (INT32 *) feature_para;
	unsigned long long *feature_data = (unsigned long long *) feature_para;

	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	UINT32 *pAeCtrls = NULL;
	UINT32 *pScenarios = NULL;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;
	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data =
		(MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

#if FPT_PDAF_SUPPORT
	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
#endif
	if (!((feature_id == 3040) || (feature_id == 3058)))
		printk("feature_id = %d\n", feature_id);
	switch (feature_id) {
	case SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS:
		pScenarios = (MUINT32 *)((uintptr_t)(*(feature_data+1)));
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			*pScenarios = MSDK_SCENARIO_ID_CUSTOM5;
			break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			*pScenarios = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_CUSTOM2:
		case MSDK_SCENARIO_ID_CUSTOM4:
		case MSDK_SCENARIO_ID_CUSTOM1:
		case MSDK_SCENARIO_ID_CUSTOM3:
		default:
			*pScenarios = 0xff;
			break;
		}
		pr_debug("SENSOR_FEATURE_GET_SEAMLESS_SCENARIOS %d %d\n", *feature_data, *pScenarios);
		break;
	case SENSOR_FEATURE_SEAMLESS_SWITCH:
		pAeCtrls = (MUINT32 *)((uintptr_t)(*(feature_data+1)));
		if (pAeCtrls)
			seamless_switch((*feature_data),*pAeCtrls,*(pAeCtrls+1),*(pAeCtrls+4),*(pAeCtrls+5));
		else
			seamless_switch((*feature_data), 0, 0, 0, 0);
		break;
	case SENSOR_FEATURE_GET_GAIN_RANGE_BY_SCENARIO:
		*(feature_data + 1) = imgsensor_info.min_gain;
		*(feature_data + 2) = imgsensor_info.max_gain;
		break;
	case SENSOR_FEATURE_GET_BASE_GAIN_ISO_AND_STEP:
		*(feature_data + 0) = imgsensor_info.min_gain_iso;
		*(feature_data + 1) = imgsensor_info.gain_step;
		*(feature_data + 2) = imgsensor_info.gain_type;
		break;
	case SENSOR_FEATURE_GET_MIN_SHUTTER_BY_SCENARIO:
	*(feature_data + 1) = imgsensor_info.min_shutter;
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_CUSTOM1:
		case MSDK_SCENARIO_ID_CUSTOM2:
		case MSDK_SCENARIO_ID_CUSTOM4:
		case MSDK_SCENARIO_ID_CUSTOM5:
			*(feature_data + 2) = 2;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
		default:
			*(feature_data + 2) = 1;
			break;
		}
		break;
#ifdef OPLUS_FEATURE_CAMERA_COMMON
		/*Henry.Chang@camera.driver 20181129, add for sensor Module SET*/
		case SENSOR_FEATURE_SET_SENSOR_OTP:
		{
			kal_int32 ret = IMGSENSOR_RETURN_SUCCESS;
			printk("SENSOR_FEATURE_SET_SENSOR_OTP length :%d\n", (UINT32)*feature_para_len);
			ret = Eeprom_CallWriteService((ACDK_SENSOR_ENGMODE_STEREO_STRUCT *)(feature_para));
			if (ret == ERROR_NONE)
				return ERROR_NONE;
			else
				return ERROR_MSDK_IS_ACTIVATED;
		}
		/*Caohua.Lin@Camera.Driver , 20190318, add for ITS--sensor_fusion*/
		case SENSOR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
			//yinyuting@CAMERA.DRV, 2020/11/19, Add for 20151/20301/20302 ITS fail
			if (IsProject_20151) {
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = DOMESTIC_OFFSET;
			} else {
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = EXPORT_OFFSET;
			}
			break;
#endif
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.cap.pclk;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.normal_video.pclk;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.hs_video.pclk;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.slim_video.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom1.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom2.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom3.pclk;
			break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom4.pclk;
				break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.custom5.pclk;
				break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
				= imgsensor_info.pre.pclk;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.cap.framelength << 16)
				+ imgsensor_info.cap.linelength;
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.normal_video.framelength << 16)
				+ imgsensor_info.normal_video.linelength;
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.hs_video.framelength << 16)
				+ imgsensor_info.hs_video.linelength;
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.slim_video.framelength << 16)
				+ imgsensor_info.slim_video.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM1:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom1.framelength << 16)
				+ imgsensor_info.custom1.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom2.framelength << 16)
				+ imgsensor_info.custom2.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom3.framelength << 16)
				+ imgsensor_info.custom3.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom4.framelength << 16)
				+ imgsensor_info.custom4.linelength;
			break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.custom5.framelength << 16)
				+ imgsensor_info.custom5.linelength;
			break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1))
			= (imgsensor_info.pre.framelength << 16)
				+ imgsensor_info.pre.linelength;
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PERIOD:
	    *feature_return_para_16++ = imgsensor.line_length;
	    *feature_return_para_16 = imgsensor.frame_length;
	    *feature_para_len = 4;
	break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
	    *feature_return_para_32 = imgsensor.pclk;
	    *feature_para_len = 4;
	break;
	case SENSOR_FEATURE_SET_ESHUTTER:
	    set_shutter(*feature_data);
	break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
	    night_mode((BOOL) * feature_data);
	break;
	case SENSOR_FEATURE_SET_GAIN:
	    set_gain((UINT16) *feature_data);
	break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
	break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
	break;
	case SENSOR_FEATURE_SET_REGISTER:
		if(sensor_reg_data->RegAddr == 0xff )
			seamless_switch(sensor_reg_data->RegData, 1920, 369, 960, 369);
		else
			write_cmos_sensor(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
	break;
	case SENSOR_FEATURE_GET_REGISTER:
	    sensor_reg_data->RegData =
			read_cmos_sensor(sensor_reg_data->RegAddr);
	break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
	    *feature_return_para_32 = LENS_DRIVER_ID_DO_NOT_CARE;
	    *feature_para_len = 4;
	break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
	    set_video_mode(*feature_data);
	break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
	    get_imgsensor_id(feature_return_para_32);
	break;
	#ifdef OPLUS_FEATURE_CAMERA_COMMON
	/*Caohua.Lin@CAmera, modify for different module 20180723*/
	case SENSOR_FEATURE_CHECK_MODULE_ID:
		*feature_return_para_32 = imgsensor_info.module_id;
		break;
	#endif
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
	    set_auto_flicker_mode((BOOL)*feature_data_16,
			*(feature_data_16+1));
	break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM)*feature_data,
			*(feature_data+1));
	break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
	    get_default_framerate_by_scenario(
			(enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data+1)));
	break;
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
	break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
	    *feature_return_para_32 = imgsensor_info.checksum_value;
	    *feature_para_len = 4;
	break;
	case SENSOR_FEATURE_SET_FRAMERATE:
	    spin_lock(&imgsensor_drv_lock);
	    imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		printk("current fps :%d\n", imgsensor.current_fps);
	break;
	case SENSOR_FEATURE_GET_CROP_INFO:
	    printk("GET_CROP_INFO scenarioId:%d\n",
			*feature_data_32);

	    wininfo = (struct  SENSOR_WINSIZE_INFO_STRUCT *)
			(uintptr_t)(*(feature_data+1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[1],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[2],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[3],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[4],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
		case MSDK_SCENARIO_ID_CUSTOM1:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[5],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[6],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CUSTOM3:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[7],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CUSTOM4:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[8],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[9],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
/* ITD: Modify Dualcam By Jesse 190924 End */
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			memcpy((void *)wininfo,
				(void *)&imgsensor_winsize_info[0],
				sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
		break;
		}
	break;
	case SENSOR_FEATURE_SET_IHDR_SHUTTER_GAIN:
	    printk("SENSOR_SET_SENSOR_IHDR LE=%d, SE=%d, Gain=%d\n",
			(UINT16)*feature_data, (UINT16)*(feature_data+1),
			(UINT16)*(feature_data+2));
	    ihdr_write_shutter_gain((UINT16)*feature_data,
			(UINT16)*(feature_data+1),
				(UINT16)*(feature_data+2));
	break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
			switch (*feature_data) {
				case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.cap.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.normal_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.hs_video.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.slim_video.mipi_pixel_rate;
				break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
			case MSDK_SCENARIO_ID_CUSTOM1:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom1.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CUSTOM2:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom2.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CUSTOM3:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom3.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CUSTOM4:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom4.mipi_pixel_rate;
				break;
			case MSDK_SCENARIO_ID_CUSTOM5:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.custom5.mipi_pixel_rate;
				break;
/* ITD: Modify Dualcam By Jesse 190924 End */
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) =
					imgsensor_info.pre.mipi_pixel_rate;
				break;
			}
	break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			case MSDK_SCENARIO_ID_CUSTOM4:
				*feature_return_para_32 = 2;
				break;
			default:
				*feature_return_para_32 = 1; /*BINNING_AVERAGED*/
				break;
			}
		pr_debug("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d,\n",
			*feature_return_para_32);
		*feature_para_len = 4;

		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		pr_debug("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16) *feature_data);

		pvcinfo =
	    (struct SENSOR_VC_INFO_STRUCT *) (uintptr_t) (*(feature_data + 1));

		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[2],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[3],
					sizeof(struct SENSOR_VC_INFO_STRUCT));
				break;
		case MSDK_SCENARIO_ID_CUSTOM5:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[4],
					sizeof(struct SENSOR_VC_INFO_STRUCT));
				break;
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CUSTOM1:
		default:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
			       sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		}
		break;

#if FPT_PDAF_SUPPORT
/******************** PDAF START ********************/
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
			case MSDK_SCENARIO_ID_CUSTOM1:
			case MSDK_SCENARIO_ID_CUSTOM2:
			//case MSDK_SCENARIO_ID_CUSTOM5:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 1;
				break;
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data+1)) = 0;
				break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		PDAFinfo = (struct SET_PD_BLOCK_INFO_T *)
			(uintptr_t)(*(feature_data+1));

		switch (*feature_data) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	   	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CUSTOM1:
			imgsensor_pd_info.i4BlockNumX = 248; //4000*3000
			imgsensor_pd_info.i4BlockNumY = 187;
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			imgsensor_pd_info.i4BlockNumX = 248; //4000*2256
			imgsensor_pd_info.i4BlockNumY = 141;
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		//case MSDK_SCENARIO_ID_CUSTOM5:
			imgsensor_pd_info.i4BlockNumX = 248; //4000*2600
			imgsensor_pd_info.i4BlockNumY = 162;
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_CUSTOM2:
			imgsensor_pd_info.i4BlockNumX = 240; //3840*2160
			imgsensor_pd_info.i4BlockNumY = 135;
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
				sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		default:
			memcpy((void *)PDAFinfo, (void *)&imgsensor_pd_info,
							sizeof(struct SET_PD_BLOCK_INFO_T));
			break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_DATA:
		break;
	case SENSOR_FEATURE_SET_PDAF:
			imgsensor.pdaf_mode = *feature_data_16;
		break;
/******************** PDAF END ********************/
#endif
	case SENSOR_FEATURE_GET_TEMPERATURE_VALUE:
		*feature_return_para_i32 = get_sensor_temperature();
		*feature_para_len = 4;
	break;
/* ITD: Modify Dualcam By Jesse 190924 Start */
		case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
			pr_debug("SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME\n");
			set_shutter_frame_length((UINT16)*feature_data, (UINT16)*(feature_data+1), (UINT16) *(feature_data + 2));
			break;
/* ITD: Modify Dualcam By Jesse 190924 End */
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		streaming_control(KAL_FALSE);
		break;

	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	#ifdef OPLUS_FEATURE_CAMERA_COMMON
	/* sunxiaohong@Camer. Add for front frame sync. ALPS05053686. 2020-03-24 */
	case SENSOR_FEATURE_GET_FRAME_CTRL_INFO_BY_SCENARIO:
		/*
		 * 1, if driver support new sw frame sync
		 * set_shutter_frame_length() support third para auto_extend_en
		 */
		*(feature_data + 1) = 1; /* margin info by scenario */
		*(feature_data + 2) = imgsensor_info.margin;
		break;
	#endif
	default:
	break;
	}

	return ERROR_NONE;
}   /*  feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 OV48B_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc != NULL)
	*pfFunc =  &sensor_func;
	return ERROR_NONE;
}
