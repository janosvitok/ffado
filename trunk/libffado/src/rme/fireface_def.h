/*
 * Copyright (C) 2009 by Jonathan Woithe
 *
 * This file is part of FFADO
 * FFADO = Free Firewire (pro-)audio drivers for linux
 *
 * FFADO is based upon FreeBoB.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* This file contains definitions relating to the RME Fireface interfaces
 * (Fireface 400 and Fireface 800).  Naming convention:
 *   RME_FF_     identifier applies to both FF400 and FF800
 *   RME_FF400_  identifier specific to the FF400
 *   RME_FF800_  identifier specific to the FF800
 */

#ifndef _FIREFACE_DEF
#define _FIREFACE_DEF

/* Identifiers for the Fireface models */
#define RME_FF400               0
#define RME_FF800               1

/* The Command Buffer Address (CBA) is different for the two interfaces */
#define RME_FF400_CMD_BUFFER    0x80100500
#define RME_FF800_CMD_BUFFER    0xfc88f000

/* Offsets for registers at fixed offsets from the device's command buffer address */
#define RME_FF_DDS_SRATE_OFS      (0*4)
#define RME_FF_CONF1_OFS          (5*4)
#define RME_FF_CONF2_OFS          (6*4)
#define RME_FF_CONF3_OFS          (7*4)
#define RME_FF400_FLASH_CMD_OFS   (8*4)       // Write only
#define RME_FF400_FLASH_STAT_OFS  (8*4)       // Read only

/* General register definitions */
#define RME_FF400_CONF_REG          (RME_FF400_CMD_BUFFER + RME_FF_CONF1_OFS)
#define RME_FF800_CONF_REG          (RME_FF800_CMD_BUFFER + RME_FF_CONF1_OFS)

#define RME_FF400_STREAM_INIT_REG   (RME_FF400_CMD_BUFFER)           // 3 quadlets wide
#define RME_FF400_STREAM_SRATE      (RME_FF400_CMD_BUFFER)
#define RME_FF400_STREAM_CONF0      (RME_FF400_CMD_BUFFER+4)
#define RME_FF400_STREAM_CONF1      (RME_FF400_CMD_BUFFER+8)
#define RME_FF800_STREAM_INIT_REG   0x20000001cLL                    // 3 quadlets wide
#define RME_FF800_STREAM_SRATE      0x20000001cLL
#define RME_FF800_STREAM_CONF0      (0x20000001cLL+4)
#define RME_FF800_STREAM_CONF1      (0x20000001cLL+8)
#define RME_FF400_STREAM_START_REG  (RME_FF400_CMD_BUFFER + 0x001c)  // 1 quadlet
#define RME_FF800_STREAM_START_REG  0x200000028LL                    // 1 quadlet
#define RME_FF400_STREAM_END_REG    (RME_FF400_CMD_BUFFER + 0x0004)  // 4 quadlets wide
#define RME_FF800_STREAM_END_REG    0x200000034LL                    // 3 quadlets wide

#define RME_FF800_HOST_LED_REG      0x200000324LL

#define RME_FF800_REVISION_REG      0x200000100LL

#define RME_FF_CHANNEL_MUTE_MASK     0x801c0000    // Write only
#define RME_FF_STATUS_REG0           0x801c0000    // Read only
#define RME_FF_STATUS_REG1           0x801c0004    // Read only

/* Addresses of various blocks in memory-mapped flash */
#define RME_FF400_FLASH_SETTINGS_ADDR       0x00060000
#define RME_FF400_FLASH_MIXER_VOLUME_ADDR   0x00070000
#define RME_FF400_FLASH_MIXER_PAN_ADDR      0x00070800
#define RME_FF400_FLASH_MIXER_HW_ADDR       0x00071000  /* Hardware volume settings, MIDI enable, submix */
#define RME_FF800_FLASH_MIXER_SHADOW_ADDR  0x3000e0000LL
#define RME_FF800_FLASH_MIXER_VOLUME_ADDR  0x3000e2000LL
#define RME_FF800_FLASH_MIXER_PAN_ADDR     0x3000e2800LL
#define RME_FF800_FLASH_MIXER_HW_ADDR      0x3000e3000LL  /* H/w volume settings, MIDI enable, submix */
#define RME_FF800_FLASH_SETTINGS_ADDR      0x3000f0000LL

/* Flash control registers */
#define RME_FF400_FLASH_BLOCK_ADDR_REG      0x80100288
#define RME_FF400_FLASH_BLOCK_SIZE_REG      0x8010028c
#define RME_FF400_FLASH_CMD_REG             (RME_FF400_CMD_BUFFER + RME_FF400_FLASH_CMD_OFS)
#define RME_FF400_FLASH_STAT_REG            (RME_FF400_CMD_BUFFER + RME_FF400_FLASH_STAT_OFS)
#define RME_FF400_FLASH_WRITE_BUFFER        0x80100290
#define RME_FF400_FLASH_READ_BUFFER         0x80100290

/* Erase control registers on the FF800 */
#define RME_FF800_FLASH_ERASE_VOLUME_REG    0x3fffffff4LL
#define RME_FF800_FLASH_ERASE_SETTINGS_REG  0x3fffffff0LL
#define RME_FF800_FLASH_ERASE_FIRMWARE_REG  0x3fffffff8LL
#define RME_FF800_FLASH_ERASE_CONFIG_REG    0x3fffffffcLL

/* Flags and special values */
#define RME_FF400_FLASH_CMD_WRITE           0x00000001
#define RME_FF400_FLASH_CMD_READ            0x00000002
#define RME_FF400_FLASH_CMD_ERASE_VOLUME    0x0000000e
#define RME_FF400_FLASH_CMD_ERASE_SETTINGS  0x0000000d
#define RME_FF400_FLASH_CMD_ERASE_CONFIG    0x0000000c
#define RME_FF400_FLASH_CMD_GET_REVISION    0x0000000f


/* Defines for components of the control registers */
// Configuration register 0
#define CR0_PHANTOM_MIC0        0x00000001
#define CR0_PHANTOM_MIC2        0x00000002
#define CR0_SPEAKER_EMU_FPGA    0x00000004
#define CR0_ILEVEL_FPGA_CTRL0   0x00000008
#define CR0_ILEVEL_FPGA_CTRL1   0x00000010
#define CR0_ILEVEL_FPGA_CTRL2   0x00000020
#define CR0_ZEROBIT06           0x00000040
#define CR0_PHANTOM_MIC1        0x00000080
#define CR0_PHANTOM_MIC3        0x00000100
#define CR0_ZEROBIT09           0x00000200
#define CR0_INSTR_DRIVE_FPGA    0x00000200
#define CRO_OLEVEL_FPGA_CTRL_0  0x00000400
#define CRO_OLEVEL_FPGA_CTRL_1  0x00000800
#define CRO_OLEVEL_FPGA_CTRL_2  0x00001000
#define CR0_ZEROBIT13           0x00002000
#define CRO_ZEROBIT14           0x00004000
#define CRO_ZEROBIT15           0x00008000
#define CRO_PHLEVEL_CTRL0       0x00010000
#define CRO_PHLEVEL_CTRL1       0x00020000

#define CR0_PHANTOM_FF400_MIC0  CR0_PHANTOM_MIC0
#define CR0_PHANTOM_FF400_MIC1  CR0_PHANTOM_MIC1
#define CR0_PHANTOM_FF800_MIC7  CR0_PHANTOM_MIC0
#define CR0_PHANTOM_FF800_MIC8  CR0_PHANTOM_MIC1
#define CR0_PHANTOM_FF800_MIC9  CR0_PHANTOM_MIC2
#define CR0_PHANTOM_FF800_MIC10 CR0_PHANTOM_MIC3
#define CR0_ILEVEL_FPGA_LOGAIN  CR0_ILEVEL_FPGA_CTRL0
#define CR0_ILEVEL_FPGA_4dBU    CR0_ILEVEL_FPGA_CTRL1
#define CR0_ILEVEL_FPGA_m10dBV  CR0_ILEVEL_FPGA_CTRL2
#define CR0_OLEVEL_FPGA_HIGAIN  CRO_OLEVEL_FPGA_CTRL_0
#define CR0_OLEVEL_FPGA_4dBU    CRO_OLEVEL_FPGA_CTRL_1
#define CR0_OLEVEL_FPGA_m10dBV  CRO_OLEVEL_FPGA_CTRL_2
#define CR0_PHLEVEL_4dBU        0
#define CRO_PHLEVEL_m10dBV      CRO_PHLEVEL_CTRL0
#define CRO_PHLEVEL_HIGAIN      CRO_PHLEVEL_CTRL1

// Configuration register 1
#define CR1_ILEVEL_CPLD_CTRL0   0x00000001
#define CR1_ILEVEL_CPLD_CTRL1   0x00000002
#define CR1_INPUT0_REAR         0x00000004
#define CR1_OLEVEL_CPLD_CTRL0   0x00000008
#define CR1_OLEVEL_CPLD_CTRL1   0x00000010
#define CR1_INPUT1_FRONT        0x00000020
#define CR1_INPUT1_REAR         0x00000040
#define CR1_INPUT2_FRONT        0x00000080
#define CR1_INPUT2_REAR         0x00000100
#define CR1_INSTR_DRIVE         0x00000200
#define CR1_ZEROBIT10           0x00000400
#define CR1_INPUT0_FRONT        0x00000800

#define CR1_ILEVEL_CPLD_LOGAIN  0
#define CR1_ILEVEL_CPLD_4dBU    CR1_ILEVEL_CPLD_CTRL1
#define CR1_ILEVEL_CPLD_m10dBV  (CR1_ILEVEL_CPLD_CTRL0 | CR1_ILEVEL_CPLD_CTRL1)
#define CR1_OLEVEL_CPLD_m10dBV  CR1_OLEVEL_CPLD_CTRL0
#define CR1_OLEVEL_CPLD_HIGAIN  CR1_OLEVEL_CPLD_CTRL1
#define CR1_OLEVEL_CPLD_4dBU    (CR1_OLEVEL_CPLD_CTRL0 | CR1_OLEVEL_CPLD_CTRL1)
#define CR1_FF800_INPUT1_FRONT  CR1_INPUT0_FRONT
#define CR1_FF800_INPUT1_REAR   CR1_INPUT0_REAR
#define CR1_FF800_INPUT7_FRONT  CR1_INPUT1_FRONT
#define CR1_FF800_INPUT7_REAR   CR1_INPUT1_REAR
#define CR1_FF800_INPUT8_FRONT  CR1_INPUT2_FRONT
#define CR1_FF800_INPUT8_REAR   CR1_INPUT2_REAR

// Configuration register 2
#define CR2_CLOCKMODE_MASTER    0x00000000
#define CR2_CLOCKMODE_AUTOSYNC  0x00000001
#define CR2_FREQ0               0x00000002
#define CR2_FREQ1               0x00000004
#define CR2_DSPEED              0x00000008
#define CR2_QSSPEED             0x00000010
#define CR2_SPDIF_OUT_PRO       0x00000020
#define CR2_SPDIF_OUT_EMP       0x00000040
#define CR2_SPDIF_OUT_NONAUDIO  0x00000080
#define CR2_SPDIF_OUT_ADAT2     0x00000100
#define CR2_SPDIF_IN_COAX       0x00000000
#define CR2_SPDIF_IN_ADAT2      0x00000200
#define CR2_SYNC_REF0           0x00000400
#define CR2_SYNC_REF1           0x00000800
#define CR2_SYNC_REF2           0x00001000
#define CR2_WORD_CLOCK_1x       0x00002000
#define CR2_TOGGLE_TCO          0x00004000  // Normally set to 0
#define CR2_P12DB_AN0           0x00010000  // Normally set to 0
#define CR2_FF400_BIT           0x04000000  // Set on FF400, clear on FF800
#define CR2_TMS                 0x40000000  // Unit option, normally 0
#define CR2_DROP_AND_STOP       0x80000000  // Normally set to 1

#define CR2_SYNC_ADAT1          0x0
#define CR2_SYNC_ADAT2          (CR2_SYNC_REF0)
#define CR2_SYNC_SPDIF          (CR2_SYNC_REF0 | CR2_SYNC_REF1)
#define CR2_SYNC_WORDCLOCK      (CR2_SYNC_REF2)
#define CR2_SYNC_TCO            (CR2_SYNC_REF0 | CR2_SYNC_REF2)

/* Structure used to store device settings in the device flash RAM.  This
 * structure mirrors the layout in the Fireface's flash, so it cannot be
 * altered.  Fields named as unused_* are not utilised at present.
 */
typedef struct {
    uint32_t unused_device_id;
    uint32_t unused_device_rev;
    uint32_t unused_asio_latency;
    uint32_t unused_samples_per_frame;
    uint32_t spdif_input_mode;
    uint32_t spdif_output_emphasis;
    uint32_t spdif_output_pro;
    uint32_t clock_mode;
    uint32_t spdif_output_nonaudio;
    uint32_t sync_ref;
    uint32_t spdif_output_mode;
    uint32_t unused_check_input;
    uint32_t unused_status;
    uint32_t unused_register[4];
    uint32_t unused_iso_rx_channel;
    uint32_t unused_iso_tx_channel;
    uint32_t unused_timecode;
    uint32_t unused_device_type;
    uint32_t unused_number_of_devices;
    uint32_t tms;
    uint32_t unused_speed;
    uint32_t unused_channels_avail_hi;
    uint32_t unused_channels_avail_lo;
    uint32_t limit_bandwidth;
    uint32_t unused_bandwidth_allocated;
    uint32_t stop_on_dropout;
    uint32_t input_level;
    uint32_t output_level;
    uint32_t mic_level[2];
    uint32_t mic_phantom[4];
    uint32_t instrument;
    uint32_t filter;
    uint32_t fuzz;
    uint32_t unused_sync_align;
    uint32_t unused_device_index;
    uint32_t unused_advanced_dialog;
    uint32_t sample_rate;
    uint32_t unused_interleaved;
    uint32_t unused_sn;
    uint32_t word_clock_single_speed;
    uint32_t unused_num_channels;
    uint32_t unused_dropped_samples;
    uint32_t p12db_an[10];
} FF_device_flash_settings_t;

// Defines used to interpret device flash settings
#define FF_DEV_FLASH_INVALID                   0xffffffff
#define FF_DEV_FLASH_SPDIF_INPUT_COAX          0x00000001   // To be confirmed
#define FF_DEV_FLASH_SPDIF_INPUT_OPTICAL       0x00000000   // To be confirmed
#define FF_DEV_FLASH_SPDIF_OUTPUT_COAX         0x00000000   // To be confirmed
#define FF_DEV_FLASH_SPDIF_OUTPUT_OPTICAL      0x00000001   // To be confirmed
#define FF_DEV_FLASH_SPDIF_OUTPUT_EMPHASIS_ON  0x00000001
#define FF_DEV_FLASH_SPDIF_OUTPUT_PRO_ON       0x00000001
#define FF_DEV_FLASH_SPDIF_OUTPUT_NONAUDIO_ON  0x00000001
#define FF_DEV_FLASH_CLOCK_MODE_MASTER         0x00000000
#define FF_DEV_FLASH_CLOCK_MODE_SLAVE          0x00000001
#define FF_DEV_FLASH_MIC_PHANTOM_ON            0x00000001

typedef struct {
    uint32_t mic_phantom[4];
} FF_software_settings_t;

#endif