/*
 * Copyright (C) 2005-2008 by Pieter Palmers
 * Copyright (C) 2005-2009 by Jonathan Woithe
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

#include "config.h"

#include "motu/motu_avdevice.h"
#include "motu/motu_mixerdefs.h"
#include "motu/motu_mark3_mixerdefs.h"

#include "devicemanager.h"

#include "libieee1394/configrom.h"
#include "libieee1394/ieee1394service.h"

#include "libavc/avc_definitions.h"

#include "debugmodule/debugmodule.h"

#include "libstreaming/motu/MotuReceiveStreamProcessor.h"
#include "libstreaming/motu/MotuTransmitStreamProcessor.h"
#include "libstreaming/motu/MotuPort.h"

#include "libutil/Time.h"
#include "libutil/Configuration.h"

#include "libcontrol/BasicElements.h"

#include <string>
#include <stdint.h>
#include <assert.h>
#include "libutil/ByteSwap.h"
#include <iostream>
#include <sstream>

#include <libraw1394/csr.h>

namespace Motu {

// Define the supported devices.  Device ordering is arbitary here.
static VendorModelEntry supportedDeviceList[] =
{
//  {vendor_id, model_id, unit_version, unit_specifier_id, model, vendor_name,model_name}
    {FW_VENDORID_MOTU, 0, 0x00000003, 0x000001f2, MOTU_MODEL_828mkII, "MOTU", "828MkII"},
    {FW_VENDORID_MOTU, 0, 0x00000009, 0x000001f2, MOTU_MODEL_TRAVELER, "MOTU", "Traveler"},
    {FW_VENDORID_MOTU, 0, 0x0000000d, 0x000001f2, MOTU_MODEL_ULTRALITE, "MOTU", "UltraLite"},
    {FW_VENDORID_MOTU, 0, 0x0000000f, 0x000001f2, MOTU_MODEL_8PRE, "MOTU", "8pre"},
    {FW_VENDORID_MOTU, 0, 0x00000001, 0x000001f2, MOTU_MODEL_828MkI, "MOTU", "828MkI"},
    {FW_VENDORID_MOTU, 0, 0x00000005, 0x000001f2, MOTU_MODEL_896HD, "MOTU", "896HD"},
    {FW_VENDORID_MOTU, 0, 0x00000015, 0x000001f2, MOTU_MODEL_828mk3, "MOTU", "828Mk3"},
    {FW_VENDORID_MOTU, 0, 0x00000019, 0x000001f2, MOTU_MODEL_ULTRALITEmk3, "MOTU", "UltraLiteMk3"},
    {FW_VENDORID_MOTU, 0, 0x00000030, 0x000001f2, MOTU_MODEL_ULTRALITEmk3_HYB, "MOTU", "UltraLiteMk3-hybrid"},
};

// Ports declarations
const PortEntry Ports_828MKI[] =
{
    {"Analog1", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Analog2", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 31},
    {"SPDIF1", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 34},
    {"SPDIF2", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 37},
    {"ADAT1", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 40},
    {"ADAT2", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 43},
    {"ADAT3", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 46},
    {"ADAT4", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 49},
    {"ADAT5", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 52},
    {"ADAT6", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 55},
    {"ADAT7", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 58},
    {"ADAT8", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 61},
};

const PortEntry Ports_896HD[] =
{
    {"Mix-L", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 10},
    {"Mix-R", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 13},
    {"Phones-L", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 10},
    {"Phones-R", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 13},
    {"Analog1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog1", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 10},
    {"Analog2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog2", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 13},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 31},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 34},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 37},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 31},
    {"MainOut-L", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 40},
    {"MainOut-R", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 43},
    {"unknown-1", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 40},
    {"unknown-2", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 43},
    {"ADAT1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 46},
    {"ADAT2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 49},
    {"ADAT3", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 52},
    {"ADAT4", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 55},
    {"ADAT5", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 58},
    {"ADAT6", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 61},
    {"ADAT7", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 64},
    {"ADAT8", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 67},
    // AES/EBU location with ADAT active
    {"AES/EBU1", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 70},
    {"AES/EBU2", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 73},
    {"AES/EBU1", MOTU_PA_INOUT | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT, 58},
    {"AES/EBU2", MOTU_PA_INOUT | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT, 61},
    // AES/EBU location with no ADAT active
    {"AES/EBU1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_OFF, 46},
    {"AES/EBU2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_OFF, 49},
};

const PortEntry Ports_828MKII[] =
{
    {"Main-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 40},
    {"Main-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 43},
    {"Mix-L", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Mix-R", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"Analog1", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog2", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 31},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 34},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 37},
    {"Phones-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Phones-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"Mic1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 40},
    {"Mic2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 43},
    {"SPDIF1", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 46},
    {"SPDIF2", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 49},
    {"ADAT1", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 52},
    {"ADAT2", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 55},
    {"ADAT3", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 58},
    {"ADAT4", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 61},
    {"ADAT5", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 64},
    {"ADAT6", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 67},
    {"ADAT7", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 70},
    {"ADAT8", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 73},
};

const PortEntry Ports_TRAVELER[] = 
{
    {"Mix-L", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 10},
    {"Mix-R", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 13},
    {"Phones-L", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 10},
    {"Phones-R", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 13},
    {"Analog1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog1", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 10},
    {"Analog2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog2", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 13},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 31},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 34},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 37},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_4x|MOTU_PA_OPTICAL_ANY, 31},
    {"AES/EBU1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 40},
    {"AES/EBU2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 43},
    {"SPDIF1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_OPTICAL_ADAT, 46},
    {"SPDIF2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_OPTICAL_ADAT, 49},
    {"Toslink1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_TOSLINK, 46},
    {"Toslink2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_TOSLINK, 49},
    {"ADAT1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 52},
    {"ADAT2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 55},
    {"ADAT3", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 58},
    {"ADAT4", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ADAT, 61},
    {"ADAT5", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 64},
    {"ADAT6", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 67},
    {"ADAT7", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 70},
    {"ADAT8", MOTU_PA_INOUT | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT, 73},
};

const PortEntry Ports_ULTRALITE[] =
{
    {"Main-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 40},
    {"Main-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 43},
    {"Mix-L", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Mix-R", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"Mic1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Mic2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog1", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog2", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 31},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 34},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 37},
    {"Phones-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Phones-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"SPDIF1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 40},
    {"SPDIF2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 43},
    {"Padding1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY|MOTU_PA_PADDING, 46},
    {"Padding2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY|MOTU_PA_PADDING, 49},
    {"SPDIF1", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 46},
    {"SPDIF2", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 49},
};

const PortEntry Ports_8PRE[] =
{
    {"Analog1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog3", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog4", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog5", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog6", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 31},
    {"Analog7", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 34},
    {"Analog8", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 37},
    {"Mix-L", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Mix-R", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"Main-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Main-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Phones-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Phones-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"Padding1", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_OFF|MOTU_PA_PADDING, 22},
    {"Padding2", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_OFF|MOTU_PA_PADDING, 25},
    {"ADAT1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 40},
    {"ADAT1", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 22},
    {"ADAT2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 43},
    {"ADAT2", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 25},
    {"ADAT3", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 46},
    {"ADAT3", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 28},
    {"ADAT4", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 49},
    {"ADAT4", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 31},
    {"ADAT5", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 52},
    {"ADAT5", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 34},
    {"ADAT6", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 55},
    {"ADAT6", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 37},
    {"ADAT7", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 58},
    {"ADAT7", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 40},
    {"ADAT8", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 61},
    {"ADAT8", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ADAT, 43},
};

const PortEntry Ports_828mk3[] = 
{
    {"Phones-L", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 10},
    {"Phones-R", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 13},
    {"Unknown-L", MOTU_PA_OUT | MOTU_PA_RATE_4x|MOTU_PA_MK3_OPT_ANY, 10},
    {"Unknown-R", MOTU_PA_OUT | MOTU_PA_RATE_4x|MOTU_PA_MK3_OPT_ANY, 13},
    {"Mic-1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 10},
    {"Mic-2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 13},
    {"Analog1", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 16},
    {"Analog2", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 19},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 22},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 25},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 28},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 31},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 34},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 37},
    {"MainOut-L", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 40},
    {"MainOut-R", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 43},
    {"Return-1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 40},
    {"Return-2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_MK3_OPT_ANY, 43},
    {"SPDIF1", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 46},
    {"SPDIF2", MOTU_PA_INOUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 49},
    {"Unknown-1", MOTU_PA_OUT | MOTU_PA_RATE_4x|MOTU_PA_MK3_OPT_ANY, 46},
    {"Unknown-2", MOTU_PA_OUT | MOTU_PA_RATE_4x|MOTU_PA_MK3_OPT_ANY, 49},
    {"Reverb-1", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 52},
    {"Reverb-2", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_ANY, 55},
    //
    // Optical port locations are a bit messy with the Mark 3 devices since
    // there are two optical ports whose modes can be independently set.
    // First take care of the output direction.
    //
    {"Toslink-A1", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ANY, 52},
    {"Toslink-A2", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ANY, 55},
    {"ADAT-A1", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 52},
    {"ADAT-A2", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 55},
    {"ADAT-A3", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 58},
    {"ADAT-A4", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 61},
    {"ADAT-A5", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 64},
    {"ADAT-A6", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 67},
    {"ADAT-A7", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 70},
    {"ADAT-A8", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ANY, 73},
    //
    {"Toslink-B1", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_TOSLINK, 52},
    {"Toslink-B2", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_TOSLINK, 55},
    {"Toslink-B1", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_TOSLINK, 58},
    {"Toslink-B2", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_TOSLINK, 61},
    {"Toslink-B1", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 76},
    {"Toslink-B2", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 79},
    {"Toslink-B1", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 64},
    {"Toslink-B2", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 67},
    {"ADAT-B1", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 76},
    {"ADAT-B2", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 79},
    {"ADAT-B3", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 82},
    {"ADAT-B4", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 85},
    {"ADAT-B5", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 88},
    {"ADAT-B6", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 91},
    {"ADAT-B7", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 94},
    {"ADAT-B8", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 97},
    {"ADAT-B1", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 64},
    {"ADAT-B2", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 67},
    {"ADAT-B3", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"ADAT-B4", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 73},
    //
    {"ADAT-B1", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 58},
    {"ADAT-B2", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 61},
    {"ADAT-B3", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 64},
    {"ADAT-B4", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 67},
    {"ADAT-B5", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"ADAT-B6", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 73},
    {"ADAT-B7", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 76},
    {"ADAT-B8", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 79},
    {"ADAT-B1", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 58},
    {"ADAT-B2", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 61},
    {"ADAT-B3", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 64},
    {"ADAT-B4", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 67},
    //
    {"ADAT-B1", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 52},
    {"ADAT-B2", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 55},
    {"ADAT-B3", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 58},
    {"ADAT-B4", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 61},
    {"ADAT-B5", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 64},
    {"ADAT-B6", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 67},
    {"ADAT-B7", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"ADAT-B8", MOTU_PA_OUT | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 73},
    {"ADAT-B1", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 52},
    {"ADAT-B2", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 55},
    {"ADAT-B3", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 58},
    {"ADAT-B4", MOTU_PA_OUT | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_ADAT, 61},

    // Now deal with the input side of things.  Firstly comes two channels
    // which are yet to be identified at 1x rates.
    {"Unknown-1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_ANY, 58},
    {"Unknown-2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_ANY, 61},

    // Follow up with the optical input port details.
    //
    {"Toslink-A1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ANY, 64},
    {"Toslink-A2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ANY, 67},
    {"Toslink-A1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ANY, 58},
    {"Toslink-A2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_ANY, 61},
    {"ADAT-A1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 64},
    {"ADAT-A2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 67},
    {"ADAT-A3", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 70},
    {"ADAT-A4", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 73},
    {"ADAT-A5", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 76},
    {"ADAT-A6", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 79},
    {"ADAT-A7", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 82},
    {"ADAT-A8", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 85},
    {"ADAT-B1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 88},
    {"ADAT-B2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 91},
    {"ADAT-B3", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 94},
    {"ADAT-B4", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 97},
    {"ADAT-B5", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 100},
    {"ADAT-B6", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 103},
    {"ADAT-B7", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 106},
    {"ADAT-B8", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 109},
    {"ADAT-A1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 58},
    {"ADAT-A2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 61},
    {"ADAT-A3", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 64},
    {"ADAT-A4", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ANY, 67},
    {"ADAT-B1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"ADAT-B2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 73},
    {"ADAT-B3", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 76},
    {"ADAT-B4", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 79},
    {"Unknown-3", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 82},
    {"Unknown-4", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_ADAT|MOTU_PA_MK3_OPT_B_ADAT, 85},
    //
    {"Toslink-B1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_TOSLINK, 64},
    {"Toslink-B2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_TOSLINK, 67},
    {"Toslink-B1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_TOSLINK, 58},
    {"Toslink-B2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_OFF|MOTU_PA_MK3_OPT_B_TOSLINK, 61},
    {"Toslink-B1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_TOSLINK, 70},
    {"Toslink-B2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_TOSLINK, 73},
    {"Toslink-B1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_TOSLINK, 64},
    {"Toslink-B2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_TOSLINK|MOTU_PA_MK3_OPT_B_TOSLINK, 67},
    {"Toslink-B1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 88},
    {"Toslink-B2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 91},
    {"Toslink-B1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 70},
    {"Toslink-B2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_MK3_OPT_A_ADAT|MOTU_PA_MK3_OPT_B_TOSLINK, 73},
    {"ADAT-B1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 64},
    {"ADAT-B2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 67},
    {"ADAT-B3", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"ADAT-B4", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 73},
    {"ADAT-B5", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 76},
    {"ADAT-B6", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 79},
    {"ADAT-B7", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 82},
    {"ADAT-B8", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 85},
    {"ADAT-B1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 58},
    {"ADAT-B2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 61},
    {"ADAT-B3", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 64},
    {"ADAT-B4", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 67},
    {"Unknown-3", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"Unknown-4", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_OFF|MOTU_PA_MK3_OPT_B_ADAT, 73},
    //
    {"ADAT-B1", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"ADAT-B2", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 73},
    {"ADAT-B3", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 76},
    {"ADAT-B4", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 79},
    {"ADAT-B5", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 82},
    {"ADAT-B6", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 85},
    {"ADAT-B7", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 88},
    {"ADAT-B8", MOTU_PA_IN | MOTU_PA_RATE_1x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 91},
    {"ADAT-B1", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 64},
    {"ADAT-B2", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 67},
    {"ADAT-B3", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 70},
    {"ADAT-B4", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 73},
    {"Unknown-3", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 76},
    {"Unknown-4", MOTU_PA_IN | MOTU_PA_RATE_2x|MOTU_PA_OPTICAL_TOSLINK|MOTU_PA_MK3_OPT_B_ADAT, 79},

};

const PortEntry Ports_ULTRALITEmk3[] =
{
    {"Main-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 40},
    {"Main-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 43},
    {"Mix-L", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Mix-R", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"Mic1", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Mic2", MOTU_PA_IN | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog1", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 16},
    {"Analog2", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 19},
    {"Analog3", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 22},
    {"Analog4", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 25},
    {"Analog5", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 28},
    {"Analog6", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 31},
    {"Analog7", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 34},
    {"Analog8", MOTU_PA_INOUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 37},
    {"Phones-L", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 10},
    {"Phones-R", MOTU_PA_OUT | MOTU_PA_RATE_ANY|MOTU_PA_OPTICAL_ANY, 13},
    {"SPDIF1", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 40},
    {"SPDIF2", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 43},
    {"Padding1", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY|MOTU_PA_PADDING, 46},
    {"Padding2", MOTU_PA_IN | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY|MOTU_PA_PADDING, 49},
    {"SPDIF1", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 46},
    {"SPDIF2", MOTU_PA_OUT | MOTU_PA_RATE_1x2x|MOTU_PA_OPTICAL_ANY, 49},
};

/* The order of DevicesProperty entries must match the numeric order of the
 * MOTU model enumeration (EMotuModel).
 */
const DevicePropertyEntry DevicesProperty[] = {
//  { Ports_map,          N_ELEMENTS( Ports_map ),        MaxSR, MixerDescrPtr, Mark3MixerDescrPtr },
    { Ports_828MKII,      N_ELEMENTS( Ports_828MKII ),       96000, &Mixer_828Mk2, NULL, },
    { Ports_TRAVELER,     N_ELEMENTS( Ports_TRAVELER ),     192000, &Mixer_Traveler, NULL, },
    { Ports_ULTRALITE,    N_ELEMENTS( Ports_ULTRALITE ),     96000, &Mixer_Ultralite, NULL, },
    { Ports_8PRE,         N_ELEMENTS( Ports_8PRE ),          96000, &Mixer_8pre, NULL, },
    { Ports_828MKI,       N_ELEMENTS( Ports_828MKI ),        48000 },
    { Ports_896HD,        N_ELEMENTS( Ports_896HD ),        192000, &Mixer_896HD, NULL, },
    { Ports_828mk3,       N_ELEMENTS( Ports_828mk3 ),       192000 },
    { Ports_ULTRALITEmk3, N_ELEMENTS( Ports_ULTRALITEmk3 ), 192000 }, // Ultralite mk3
    { Ports_ULTRALITEmk3, N_ELEMENTS( Ports_ULTRALITEmk3 ), 192000 }, // Ultralite mk3 hybrid
};

MotuDevice::MotuDevice( DeviceManager& d, std::auto_ptr<ConfigRom>( configRom ))
    : FFADODevice( d, configRom )
    , m_motu_model( MOTU_MODEL_NONE )
    , m_iso_recv_channel ( -1 )
    , m_iso_send_channel ( -1 )
    , m_rx_bandwidth ( -1 )
    , m_tx_bandwidth ( -1 )
    , m_receiveProcessor ( 0 )
    , m_transmitProcessor ( 0 )
    , m_MixerContainer ( NULL )
    , m_ControlContainer ( NULL )
{
    debugOutput( DEBUG_LEVEL_VERBOSE, "Created Motu::MotuDevice (NodeID %d)\n",
                 getConfigRom().getNodeId() );
}

MotuDevice::~MotuDevice()
{
    delete m_receiveProcessor;
    delete m_transmitProcessor;

    // Free ieee1394 bus resources if they have been allocated
    if (m_iso_recv_channel>=0 && !get1394Service().freeIsoChannel(m_iso_recv_channel)) {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Could not free recv iso channel %d\n", m_iso_recv_channel);
    }
    if (m_iso_send_channel>=0 && !get1394Service().freeIsoChannel(m_iso_send_channel)) {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Could not free send iso channel %d\n", m_iso_send_channel);
    }

    destroyMixer();
}

bool
MotuDevice::probe( Util::Configuration& c, ConfigRom& configRom, bool generic)
{
    if(generic) return false;

    unsigned int vendorId = configRom.getNodeVendorId();
    unsigned int unitVersion = configRom.getUnitVersion();
    unsigned int unitSpecifierId = configRom.getUnitSpecifierId();

    for ( unsigned int i = 0;
          i < ( sizeof( supportedDeviceList )/sizeof( VendorModelEntry ) );
          ++i )
    {
        if ( ( supportedDeviceList[i].vendor_id == vendorId )
             && ( supportedDeviceList[i].unit_version == unitVersion )
             && ( supportedDeviceList[i].unit_specifier_id == unitSpecifierId )
           )
        {
            return true;
        }
    }

    return false;
}

FFADODevice *
MotuDevice::createDevice(DeviceManager& d, std::auto_ptr<ConfigRom>( configRom ))
{
    return new MotuDevice(d, configRom);
}

bool
MotuDevice::discover()
{
    unsigned int vendorId = getConfigRom().getNodeVendorId();
    unsigned int unitVersion = getConfigRom().getUnitVersion();
    unsigned int unitSpecifierId = getConfigRom().getUnitSpecifierId();

    for ( unsigned int i = 0;
          i < ( sizeof( supportedDeviceList )/sizeof( VendorModelEntry ) );
          ++i )
    {
        if ( ( supportedDeviceList[i].vendor_id == vendorId )
             && ( supportedDeviceList[i].unit_version == unitVersion )
             && ( supportedDeviceList[i].unit_specifier_id == unitSpecifierId )
           )
        {
            m_model = &(supportedDeviceList[i]);
            m_motu_model=supportedDeviceList[i].model;
        }
    }

    if (m_model == NULL) {
        return false;
    }

    debugOutput( DEBUG_LEVEL_VERBOSE, "found %s %s\n",
        m_model->vendor_name, m_model->model_name);

    if (!buildMixer()) {
        debugWarning("Could not build mixer\n");
    }

    return true;
}

enum FFADODevice::eStreamingState
MotuDevice::getStreamingState()
{
    unsigned int val = ReadRegister(MOTU_REG_ISOCTRL);
    /* Streaming is active if either bit 22 (Motu->PC streaming
     * enable) or bit 30 (PC->Motu streaming enable) is set.
     */
    debugOutput(DEBUG_LEVEL_VERBOSE, "MOTU_REG_ISOCTRL: %08x\n", val);

    if((val & 0x40400000) != 0) {
        return eSS_Both;
    } else if ((val & 0x40000000) != 0) {
        return eSS_Receiving;
    } else if ((val & 0x00400000) != 0) {
        return eSS_Sending;
    } else {
        return eSS_Idle;
    }
}

int
MotuDevice::getSamplingFrequency( ) {
/*
 * Retrieve the current sample rate from the MOTU device.
 */
    quadlet_t q = 0;
    int rate = 0;

    if (m_motu_model == MOTU_MODEL_828MkI) {
        /* The original MOTU interfaces did things rather differently */
        q = ReadRegister(MOTU_G1_REG_CONFIG);
        if ((q & MOTU_G1_RATE_MASK) == MOTU_G1_RATE_44100)
            rate = 44100;
        else
            rate = 48000;
        return rate;
    }

    q = ReadRegister(MOTU_REG_CLK_CTRL);
    switch (q & MOTU_RATE_BASE_MASK) {
        case MOTU_RATE_BASE_44100:
            rate = 44100;
            break;
        case MOTU_RATE_BASE_48000:
            rate = 48000;
            break;
    }
    switch (q & MOTU_RATE_MULTIPLIER_MASK) {
        case MOTU_RATE_MULTIPLIER_2X:
            rate *= 2;
            break;
        case MOTU_RATE_MULTIPLIER_4X:
            rate *= 4;
            break;
    }
    return rate;
}

int
MotuDevice::getConfigurationId()
{
    return 0;
}

unsigned int
MotuDevice::getHwClockSource()
{
    unsigned int reg;

    if (m_motu_model == MOTU_MODEL_828MkI) {
        reg = ReadRegister(MOTU_G1_REG_CONFIG);
        switch (reg & MOTU_G1_CLKSRC_MASK) {
            case MOTU_G1_CLKSRC_INTERNAL: return MOTU_CLKSRC_INTERNAL;
            case MOTU_G1_CLKSRC_ADAT_9PIN: return MOTU_CLKSRC_ADAT_9PIN;
            case MOTU_G1_CLKSRC_SPDIF: return MOTU_CLKSRC_SPDIF_TOSLINK;
        }
        return MOTU_CLKSRC_NONE;
    }

    reg = ReadRegister(MOTU_REG_CLK_CTRL);
    if (getDeviceGeneration() == MOTU_DEVICE_G2) {
        switch (reg & MOTU_G2_CLKSRC_MASK) {
            case MOTU_G2_CLKSRC_INTERNAL: return MOTU_CLKSRC_INTERNAL;
            case MOTU_G2_CLKSRC_ADAT_OPTICAL: return MOTU_CLKSRC_ADAT_OPTICAL;
            case MOTU_G2_CLKSRC_SPDIF_TOSLINK: return MOTU_CLKSRC_SPDIF_TOSLINK;
            case MOTU_G2_CLKSRC_SMPTE: return MOTU_CLKSRC_SMPTE;
            case MOTU_G2_CLKSRC_WORDCLOCK: return MOTU_CLKSRC_WORDCLOCK;
            case MOTU_G2_CLKSRC_ADAT_9PIN: return MOTU_CLKSRC_ADAT_9PIN;
            case MOTU_G2_CLKSRC_AES_EBU: return MOTU_CLKSRC_AES_EBU;
        }
    } else {
        /* Handle G3 devices */
        switch (reg & MOTU_G3_CLKSRC_MASK) {
            case MOTU_G3_CLKSRC_INTERNAL: return MOTU_CLKSRC_INTERNAL;
            case MOTU_G3_CLKSRC_SPDIF: return MOTU_CLKSRC_SPDIF_TOSLINK;
            case MOTU_G3_CLKSRC_SMPTE: return MOTU_CLKSRC_SMPTE;
            case MOTU_G3_CLKSRC_WORDCLOCK: return MOTU_CLKSRC_WORDCLOCK;
            case MOTU_G3_CLKSRC_OPTICAL_A: return MOTU_CLKSRC_OPTICAL_A;
            case MOTU_G3_CLKSRC_OPTICAL_B: return MOTU_CLKSRC_OPTICAL_B;
        }
    }
    return MOTU_CLKSRC_NONE;
}

bool
MotuDevice::setClockCtrlRegister(signed int samplingFrequency, unsigned int clock_source)
{
/*
 * Set the MOTU device's samplerate and/or clock source via the clock
 * control register.  If samplingFrequency <= 0 it remains unchanged.  If
 * clock_source is MOTU_CLKSRC_UNCHANGED the clock source remains unchanged.
 */
    const char *src_name;
    quadlet_t q, new_rate=0xffffffff;
    signed int i, supported=true, cancel_adat=false;
    quadlet_t reg;
    unsigned int rate_mask = 0;
    unsigned int old_clock_src = getHwClockSource();
    signed int device_gen = getDeviceGeneration();

    /* Don't touch anything if there's nothing to do */
    if (samplingFrequency<=0 && clock_source==MOTU_CLKSRC_NONE)
        return true;

    if ( samplingFrequency > DevicesProperty[m_motu_model-1].MaxSampleRate )
       return false; 

    /* The original MOTU devices do things differently; they are much
     * simpler than the later interfaces.
     */
    if (m_motu_model == MOTU_MODEL_828MkI) {
        reg = ReadRegister(MOTU_G1_REG_CONFIG);
        if (samplingFrequency > 0) {
            reg &= ~MOTU_G1_RATE_MASK;
            switch (samplingFrequency) {
                case 44100:
                    reg |= MOTU_G1_RATE_44100;
                    break;
                case 48000:
                    reg |= MOTU_G1_RATE_48000;
                default:
                    // Unsupported rate
                    return false;
            }
        }
        if (clock_source != MOTU_CLKSRC_UNCHANGED) {
            switch (clock_source) {
                case MOTU_CLKSRC_INTERNAL:
                    clock_source = MOTU_G1_CLKSRC_INTERNAL; break;
                case MOTU_CLKSRC_SPDIF_TOSLINK:
                    clock_source = MOTU_G1_CLKSRC_SPDIF; break;
                case MOTU_CLKSRC_ADAT_9PIN:
                    clock_source = MOTU_G1_CLKSRC_ADAT_9PIN; break;
                default:
                    // Unsupported clock source
                    return false;
            }
            reg &= ~MOTU_G1_CLKSRC_MASK;
            reg |= clock_source;
        }
        if (WriteRegister(MOTU_G1_REG_CONFIG, reg) != 0)
            return false;
        return true;
    }

    /* The rest of this function deals with later generation devices */

    reg = ReadRegister(MOTU_REG_CLK_CTRL);

    /* The method of controlling the sampling rate is the same for G2/G3 
     * devices but the actual bits used in the rate control register differ.
     */
    if (device_gen == MOTU_DEVICE_G2) {
        rate_mask = MOTU_RATE_BASE_MASK | MOTU_RATE_MULTIPLIER_MASK;
        switch ( samplingFrequency ) {
            case -1: break;
            case 44100: new_rate = MOTU_RATE_BASE_44100 | MOTU_RATE_MULTIPLIER_1X; break;
            case 48000: new_rate = MOTU_RATE_BASE_48000 | MOTU_RATE_MULTIPLIER_1X; break;
            case 88200: new_rate = MOTU_RATE_BASE_44100 | MOTU_RATE_MULTIPLIER_2X; break;
            case 96000: new_rate = MOTU_RATE_BASE_48000 | MOTU_RATE_MULTIPLIER_2X; break;
            case 176400: new_rate = MOTU_RATE_BASE_44100 | MOTU_RATE_MULTIPLIER_4X; break;
            case 192000: new_rate = MOTU_RATE_BASE_48000 | MOTU_RATE_MULTIPLIER_4X; break;
            default:
                supported=false;
        }
    } else 
    if (device_gen == MOTU_DEVICE_G3) {
        rate_mask = MOTU_G3_RATE_BASE_MASK | MOTU_G3_RATE_MULTIPLIER_MASK;
        switch ( samplingFrequency ) {
            case -1: break;
            case 44100: new_rate = MOTU_G3_RATE_BASE_44100 | MOTU_G3_RATE_MULTIPLIER_1X; break;
            case 48000: new_rate = MOTU_G3_RATE_BASE_48000 | MOTU_G3_RATE_MULTIPLIER_1X; break;
            case 88200: new_rate = MOTU_G3_RATE_BASE_44100 | MOTU_G3_RATE_MULTIPLIER_2X; break;
            case 96000: new_rate = MOTU_G3_RATE_BASE_48000 | MOTU_G3_RATE_MULTIPLIER_2X; break;
            case 176400: new_rate = MOTU_G3_RATE_BASE_44100 | MOTU_G3_RATE_MULTIPLIER_4X; break;
            case 192000: new_rate = MOTU_G3_RATE_BASE_48000 | MOTU_G3_RATE_MULTIPLIER_4X; break;
            default:
                supported=false;
        }
    }
    /* ADAT output is only possible for sample rates up to 96 kHz.  For
     * anything higher, force the ADAT channels off.
     */
    if (samplingFrequency > 96000) {
      cancel_adat = true;
    }

    // Sanity check the clock source
    if (clock_source>MOTU_CLKSRC_LAST && clock_source!=MOTU_CLKSRC_UNCHANGED)
        supported = false;

    // Update the clock control register.  FIXME: while this is now rather
    // comprehensive there may still be a need to manipulate MOTU_REG_CLK_CTRL
    // a little more than we do.
    if (supported) {

        // If optical port must be disabled (because a 4x sample rate has
        // been selected) then do so before changing the sample rate.  At
        // this stage it will be up to the user to re-enable the optical
        // port if the sample rate is set to a 1x or 2x rate later.
        if (cancel_adat) {
            setOpticalMode(MOTU_CTRL_DIR_INOUT, MOTU_OPTICAL_MODE_OFF, MOTU_OPTICAL_MODE_OFF);
        }

        // Set up new frequency if requested
        if (new_rate != 0xffffffff) {
            reg &= ~rate_mask;
            reg |= new_rate;
        }

        // Set up new clock source if required
        if (clock_source != MOTU_CLKSRC_UNCHANGED) {
            if (device_gen == MOTU_DEVICE_G2) {
                reg &= ~MOTU_G2_CLKSRC_MASK;
                switch (clock_source) {
                    case MOTU_CLKSRC_INTERNAL: reg |= MOTU_G2_CLKSRC_INTERNAL; break;
                    case MOTU_CLKSRC_ADAT_OPTICAL: reg |= MOTU_G2_CLKSRC_ADAT_OPTICAL; break;
                    case MOTU_CLKSRC_SPDIF_TOSLINK: reg |= MOTU_G2_CLKSRC_SPDIF_TOSLINK; break;
                    case MOTU_CLKSRC_SMPTE: reg |= MOTU_G2_CLKSRC_SMPTE; break;
                    case MOTU_CLKSRC_WORDCLOCK: reg |= MOTU_G2_CLKSRC_WORDCLOCK; break;
                    case MOTU_CLKSRC_ADAT_9PIN: reg |= MOTU_G2_CLKSRC_ADAT_9PIN; break;
                    case MOTU_CLKSRC_AES_EBU: reg |= MOTU_G2_CLKSRC_AES_EBU; break;
                }
            } else {
                reg &= ~MOTU_G3_CLKSRC_MASK;
                switch (clock_source) {
                    case MOTU_CLKSRC_INTERNAL: reg |= MOTU_G3_CLKSRC_INTERNAL; break;
                    case MOTU_CLKSRC_SPDIF_TOSLINK: reg |= MOTU_G3_CLKSRC_SPDIF; break;
                    case MOTU_CLKSRC_SMPTE: reg |= MOTU_G3_CLKSRC_SMPTE; break;
                    case MOTU_CLKSRC_WORDCLOCK: reg |= MOTU_G3_CLKSRC_WORDCLOCK; break;
                    case MOTU_CLKSRC_OPTICAL_A: reg |= MOTU_G3_CLKSRC_OPTICAL_A; break; 
                    case MOTU_CLKSRC_OPTICAL_B: reg |= MOTU_G3_CLKSRC_OPTICAL_B; break;
                }
            }
        } else {
            /* Use the device's current clock source to set the clock
             * source name registers, which must be done even if we aren't
             * changing the clock source.
             */
            clock_source = old_clock_src;
        }

        // Bits 24-26 of MOTU_REG_CLK_CTRL behave a little differently
        // depending on the model.  In addition, different bit patterns are
        // written depending on whether streaming is enabled, disabled or is
        // changing state.  For now we go with the combination used when
        // streaming is enabled since it seems to work for the other states
        // as well.  Since device muting can be effected by these bits, we
        // may utilise this in future during streaming startup to prevent
        // noises during stabilisation.
        //
        // For most models (possibly all except the Ultralite) all 3 bits
        // can be zero and audio is still output.
        //
        // For the Traveler, if bit 26 is set (as it is under other OSes),
        // bit 25 functions as a device mute bit: if set, audio is output
        // while if 0 the entire device is muted.  If bit 26 is unset,
        // setting bit 25 doesn't appear to be detrimental.
        //
        // For the Ultralite, other OSes leave bit 26 unset.  However, unlike
        // other devices bit 25 seems to function as a mute bit in this case.
        //
        // The function of bit 24 is currently unknown.  Other OSes set it
        // for all devices so we will too.
        reg &= 0xf8ffffff;
        if (m_motu_model == MOTU_MODEL_TRAVELER)
            reg |= 0x04000000;
        reg |= 0x03000000;
        if (WriteRegister(MOTU_REG_CLK_CTRL, reg) == 0) {
            supported=true;
        } else {
            supported=false;
        }
        // A write to the rate/clock control register requires the
        // textual name of the current clock source be sent to the
        // clock source name registers.  This appears to be the same for
        // both G2 and G3 devices.
        switch (clock_source) {
            case MOTU_CLKSRC_INTERNAL:
                src_name = "Internal        ";
                break;
            case MOTU_CLKSRC_ADAT_OPTICAL:
                src_name = "ADAT Optical    ";
                break;
            case MOTU_CLKSRC_SPDIF_TOSLINK: {
                unsigned int p0_mode;
                if (device_gen < MOTU_DEVICE_G3) {
                    getOpticalMode(MOTU_DIR_IN, &p0_mode, NULL);
                } else
                    p0_mode = MOTU_OPTICAL_MODE_OFF;
                if (p0_mode == MOTU_OPTICAL_MODE_TOSLINK)
                    src_name = "TOSLink         ";
                else
                    src_name = "SPDIF           ";
                break;
            }
            case MOTU_CLKSRC_SMPTE:
                src_name = "SMPTE           ";
                break;
            case MOTU_CLKSRC_WORDCLOCK:
                src_name = "Word Clock In   ";
                break;
            case MOTU_CLKSRC_ADAT_9PIN:
                src_name = "ADAT 9-pin      ";
                break;
            case MOTU_CLKSRC_AES_EBU:
                src_name = "AES-EBU         ";
                break;
            case MOTU_CLKSRC_OPTICAL_A: {
                unsigned int p0_mode;
                getOpticalMode(MOTU_DIR_IN, &p0_mode, NULL);
                if (p0_mode == MOTU_OPTICAL_MODE_TOSLINK)
                    src_name = "Toslink-A       ";
                else
                    src_name = "ADAT-A Optical  ";
                break;
            }
            case MOTU_CLKSRC_OPTICAL_B: {
                unsigned int p1_mode;
                getOpticalMode(MOTU_DIR_IN, NULL, &p1_mode);
                if (p1_mode == MOTU_OPTICAL_MODE_TOSLINK)
                    src_name = "Toslink-B       ";
                else
                    src_name = "ADAT-B Optical  ";
                break;
            }
            default:
                src_name = "Unknown         ";
        }
        for (i=0; i<16; i+=4) {
            q = (src_name[i]<<24) | (src_name[i+1]<<16) |
                (src_name[i+2]<<8) | src_name[i+3];
            WriteRegister(MOTU_REG_CLKSRC_NAME0+i, q);
        }
    }
    return supported;
}

bool
MotuDevice::setSamplingFrequency( int samplingFrequency )
{
/*
 * Set the MOTU device's samplerate.
 */
    return setClockCtrlRegister(samplingFrequency, MOTU_CLKSRC_UNCHANGED);
}

std::vector<int>
MotuDevice::getSupportedSamplingFrequencies()
{
    std::vector<int> frequencies;
    signed int max_freq = DevicesProperty[m_motu_model-1].MaxSampleRate;

    /* All MOTUs support 1x rates.  All others must be conditional. */
    frequencies.push_back(44100);
    frequencies.push_back(48000);

    if (88200 <= max_freq)
        frequencies.push_back(88200);
    if (96000 <= max_freq)
        frequencies.push_back(96000);
    if (176400 <= max_freq)
        frequencies.push_back(176400);
    if (192000 <= max_freq)
        frequencies.push_back(192000);
    return frequencies;
}

FFADODevice::ClockSource
MotuDevice::clockIdToClockSource(unsigned int id) {
    ClockSource s;
    signed int device_gen = getDeviceGeneration();
    s.id = id;

    // Assume a clock source is valid/active unless otherwise overridden.
    s.valid = true;
    s.locked = true;
    s.active = true;

    switch (id) {
        case MOTU_CLKSRC_INTERNAL:
            s.type = eCT_Internal;
            s.description = "Internal sync";
            break;
        case MOTU_CLKSRC_ADAT_OPTICAL:
            s.type = eCT_ADAT;
            s.description = "ADAT optical";
            s.valid = s.active = s.locked = (device_gen!=MOTU_DEVICE_G1);
            break;
        case MOTU_CLKSRC_SPDIF_TOSLINK:
            s.type = eCT_SPDIF;
            if (device_gen < MOTU_DEVICE_G3)
                s.description = "SPDIF/Toslink";
            else
                s.description = "SPDIF";
            break;
        case MOTU_CLKSRC_SMPTE:
            s.type = eCT_SMPTE;
            s.description = "SMPTE";
            // Since we don't currently know how to deal with SMPTE on these
            // devices make sure the SMPTE clock source is disabled.
            s.valid = false;
            s.active = false;
            s.locked = false;
            break;
        case MOTU_CLKSRC_WORDCLOCK:
            s.type = eCT_WordClock;
            s.description = "Wordclock";
            s.valid = s.active = s.locked = (device_gen!=MOTU_DEVICE_G1);
            break;
        case MOTU_CLKSRC_ADAT_9PIN:
            s.type = eCT_ADAT;
            s.description = "ADAT 9-pin";
            break;
        case MOTU_CLKSRC_AES_EBU:
            s.type = eCT_AES;
            s.description = "AES/EBU";
            s.valid = s.active = s.locked = (device_gen!=MOTU_DEVICE_G1);
            break;
        case MOTU_CLKSRC_OPTICAL_A:
            s.type = eCT_ADAT;
            s.description = "ADAT/Toslink port A";
            break;
        case MOTU_CLKSRC_OPTICAL_B:
            s.type = eCT_ADAT;
            s.description = "ADAT/Toslink port B";
            break;
        default:
            s.type = eCT_Invalid;
    }

    s.slipping = false;
    return s;
}

FFADODevice::ClockSourceVector
MotuDevice::getSupportedClockSources() {
    FFADODevice::ClockSourceVector r;
    ClockSource s;
    signed int device_gen = getDeviceGeneration();

    /* Form a list of clocks supported by MOTU interfaces */

    /* All interfaces support an internal clock */
    s = clockIdToClockSource(MOTU_CLKSRC_INTERNAL);
    r.push_back(s);

    if (device_gen == MOTU_DEVICE_G2) {
        s = clockIdToClockSource(MOTU_CLKSRC_ADAT_OPTICAL);
        r.push_back(s);
    }

    s = clockIdToClockSource(MOTU_CLKSRC_SPDIF_TOSLINK);
    r.push_back(s);
    s = clockIdToClockSource(MOTU_CLKSRC_SMPTE);
    r.push_back(s);
    s = clockIdToClockSource(MOTU_CLKSRC_WORDCLOCK);
    r.push_back(s);

    /* The 9-pin ADAT sync was only present on selected G2
     * devices.
     */
    if (m_motu_model==MOTU_MODEL_828mkII || m_motu_model==MOTU_MODEL_TRAVELER ||
        m_motu_model==MOTU_MODEL_896HD) {
        s = clockIdToClockSource(MOTU_CLKSRC_ADAT_9PIN);
        r.push_back(s);
    }
    /* AES/EBU is present on the G2/G3 Travelers and 896HDs */
    if (m_motu_model==MOTU_MODEL_TRAVELER || m_motu_model==MOTU_MODEL_TRAVELERmk3 ||
        m_motu_model==MOTU_MODEL_896HD || m_motu_model==MOTU_MODEL_896HDmk3) {
        s = clockIdToClockSource(MOTU_CLKSRC_AES_EBU);
        r.push_back(s);
    }

    /* Dual-port ADAT is a feature of the G3 devices, and then only some */
    if (m_motu_model==MOTU_MODEL_828mk3 || m_motu_model==MOTU_MODEL_TRAVELERmk3 ||
        m_motu_model==MOTU_MODEL_896HDmk3) {
        s = clockIdToClockSource(MOTU_CLKSRC_OPTICAL_A);
        r.push_back(s);
        s = clockIdToClockSource(MOTU_CLKSRC_OPTICAL_B);
        r.push_back(s);
    }

    return r;
}

bool
MotuDevice::setActiveClockSource(ClockSource s) {
    debugOutput(DEBUG_LEVEL_VERBOSE, "setting clock source to id: %d\n",s.id);

    // FIXME: this could do with some error checking
    return setClockCtrlRegister(-1, s.id);
}

FFADODevice::ClockSource
MotuDevice::getActiveClockSource() {
    ClockSource s;
    quadlet_t clock_id = getHwClockSource();
    s = clockIdToClockSource(clock_id);
    s.active = true;
    return s;
}

bool
MotuDevice::lock() {

    return true;
}


bool
MotuDevice::unlock() {

    return true;
}

void
MotuDevice::showDevice()
{
    debugOutput(DEBUG_LEVEL_VERBOSE,
        "%s %s at node %d\n", m_model->vendor_name, m_model->model_name,
        getNodeId());
}

bool
MotuDevice::prepare() {

    int samp_freq = getSamplingFrequency();
    unsigned int optical_in_mode_a, optical_out_mode_a;
    unsigned int optical_in_mode_b, optical_out_mode_b;
    unsigned int event_size_in = getEventSize(MOTU_DIR_IN);
    unsigned int event_size_out= getEventSize(MOTU_DIR_OUT);

    debugOutput(DEBUG_LEVEL_NORMAL, "Preparing MotuDevice...\n" );

    getOpticalMode(MOTU_DIR_IN, &optical_in_mode_a, &optical_in_mode_b);
    getOpticalMode(MOTU_DIR_OUT, &optical_out_mode_a, &optical_out_mode_b);

    // Explicitly set the optical mode, primarily to ensure that the
    // MOTU_REG_OPTICAL_CTRL register is initialised.  We need to do this to
    // because some interfaces (the Ultralite for example) appear to power
    // up without this set to anything sensible.  In this case, writes to
    // MOTU_REG_ISOCTRL fail more often than not, which is bad.
    setOpticalMode(MOTU_DIR_IN, optical_in_mode_a, optical_in_mode_b);
    setOpticalMode(MOTU_DIR_OUT, optical_out_mode_a, optical_out_mode_b);

    // Allocate bandwidth if not previously done.
    // FIXME: The bandwidth allocation calculation can probably be
    // refined somewhat since this is currently based on a rudimentary
    // understanding of the ieee1394 iso protocol.
    // Currently we assume the following.
    //   * Ack/iso gap = 0.05 us
    //   * DATA_PREFIX = 0.16 us
    //   * DATA_END    = 0.26 us
    // These numbers are the worst-case figures given in the ieee1394
    // standard.  This gives approximately 0.5 us of overheads per packet -
    // around 25 bandwidth allocation units (from the ieee1394 standard 1
    // bandwidth allocation unit is 125/6144 us).  We further assume the
    // MOTU is running at S400 (which it should be) so one allocation unit
    // is equivalent to 1 transmitted byte; thus the bandwidth allocation
    // required for the packets themselves is just the size of the packet. 
    // We used to allocate based on the maximum packet size (1160 bytes at
    // 192 kHz for the traveler) but now do this based on the actual device
    // state by utilising the result from getEventSize() and remembering
    // that each packet has an 8 byte CIP header.  Note that bandwidth is
    // allocated on a *per stream* basis - it must be allocated for both the
    // transmit and receive streams.  While most MOTU modules are close to
    // symmetric in terms of the number of in/out channels there are
    // exceptions, so we deal with receive and transmit bandwidth separately.
    signed int n_events_per_packet = samp_freq<=48000?8:(samp_freq<=96000?16:32);
    m_rx_bandwidth = 25 + (n_events_per_packet*event_size_in);
    m_tx_bandwidth = 25 + (n_events_per_packet*event_size_out);

    // Assign iso channels if not already done
    if (m_iso_recv_channel < 0)
        m_iso_recv_channel = get1394Service().allocateIsoChannelGeneric(m_rx_bandwidth);

    if (m_iso_send_channel < 0)
        m_iso_send_channel = get1394Service().allocateIsoChannelGeneric(m_tx_bandwidth);

    debugOutput(DEBUG_LEVEL_VERBOSE, "recv channel = %d, send channel = %d\n",
        m_iso_recv_channel, m_iso_send_channel);

    if (m_iso_recv_channel<0 || m_iso_send_channel<0) {
        // be nice and deallocate
        if (m_iso_recv_channel >= 0)
            get1394Service().freeIsoChannel(m_iso_recv_channel);
        if (m_iso_send_channel >= 0)
            get1394Service().freeIsoChannel(m_iso_send_channel);

        debugFatal("Could not allocate iso channels!\n");
        return false;
    }

    // get the device specific and/or global SP configuration
    Util::Configuration &config = getDeviceManager().getConfiguration();
    // base value is the config.h value
    float recv_sp_dll_bw = STREAMPROCESSOR_DLL_BW_HZ;
    float xmit_sp_dll_bw = STREAMPROCESSOR_DLL_BW_HZ;

    // we can override that globally
    config.getValueForSetting("streaming.spm.recv_sp_dll_bw", recv_sp_dll_bw);
    config.getValueForSetting("streaming.spm.xmit_sp_dll_bw", xmit_sp_dll_bw);

    // or override in the device section
    config.getValueForDeviceSetting(getConfigRom().getNodeVendorId(), getConfigRom().getModelId(), "recv_sp_dll_bw", recv_sp_dll_bw);
    config.getValueForDeviceSetting(getConfigRom().getNodeVendorId(), getConfigRom().getModelId(), "xmit_sp_dll_bw", xmit_sp_dll_bw);

    m_receiveProcessor=new Streaming::MotuReceiveStreamProcessor(*this, event_size_in);
    m_receiveProcessor->setVerboseLevel(getDebugLevel());

    // The first thing is to initialize the processor.  This creates the
    // data structures.
    if(!m_receiveProcessor->init()) {
        debugFatal("Could not initialize receive processor!\n");
        return false;
    }

    if(!m_receiveProcessor->setDllBandwidth(recv_sp_dll_bw)) {
        debugFatal("Could not set DLL bandwidth\n");
        delete m_receiveProcessor;
        m_receiveProcessor = NULL;
        return false;
    }

    // Now we add ports to the processor
    debugOutput(DEBUG_LEVEL_VERBOSE,"Adding ports to receive processor\n");

    char *buff;
    Streaming::Port *p=NULL;

    // retrieve the ID
    std::string id=std::string("dev?");
    if(!getOption("id", id)) {
        debugWarning("Could not retrieve id parameter, defaulting to 'dev?'\n");
    }

    // Add audio capture ports
    if (!addDirPorts(Streaming::Port::E_Capture, samp_freq, optical_in_mode_a, optical_in_mode_b)) {
        return false;
    }

    // Add MIDI port.  The MOTU only has one MIDI input port, with each
    // MIDI byte sent using a 3 byte sequence starting at byte 4 of the
    // event data.
    asprintf(&buff,"%s_cap_MIDI0",id.c_str());
    p = new Streaming::MotuMidiPort(*m_receiveProcessor, buff,
        Streaming::Port::E_Capture, 4);
    if (!p) {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Skipped port %s\n", buff);
    }
    free(buff);

    // example of adding an control port:
//    asprintf(&buff,"%s_cap_%s",id.c_str(),"myportnamehere");
//    p=new Streaming::MotuControlPort(
//            buff,
//            Streaming::Port::E_Capture,
//            0 // you can add all other port specific stuff you
//              // need to pass by extending MotuXXXPort and MotuPortInfo
//    );
//    free(buff);
//
//    if (!p) {
//        debugOutput(DEBUG_LEVEL_VERBOSE, "Skipped port %s\n",buff);
//    } else {
//
//        if (!m_receiveProcessor->addPort(p)) {
//            debugWarning("Could not register port with stream processor\n");
//            return false;
//        } else {
//            debugOutput(DEBUG_LEVEL_VERBOSE, "Added port %s\n",buff);
//        }
//    }

    // Do the same for the transmit processor
    m_transmitProcessor=new Streaming::MotuTransmitStreamProcessor(*this, event_size_out);

    m_transmitProcessor->setVerboseLevel(getDebugLevel());

    if(!m_transmitProcessor->init()) {
        debugFatal("Could not initialize transmit processor!\n");
        return false;
    }

    if(!m_transmitProcessor->setDllBandwidth(xmit_sp_dll_bw)) {
        debugFatal("Could not set DLL bandwidth\n");
        delete m_transmitProcessor;
        m_transmitProcessor = NULL;
        return false;
    }

    // Now we add ports to the processor
    debugOutput(DEBUG_LEVEL_VERBOSE,"Adding ports to transmit processor\n");

    // Add audio playback ports
    if (!addDirPorts(Streaming::Port::E_Playback, samp_freq, optical_out_mode_a, optical_out_mode_b)) {
        return false;
    }

    // Add MIDI port.  The MOTU only has one output MIDI port, with each
    // MIDI byte transmitted using a 3 byte sequence starting at byte 4
    // of the event data.
    asprintf(&buff,"%s_pbk_MIDI0",id.c_str());
    p = new Streaming::MotuMidiPort(*m_transmitProcessor, buff,
        Streaming::Port::E_Playback, 4);
    if (!p) {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Skipped port %s\n", buff);
    }
    free(buff);

    // example of adding an control port:
//    asprintf(&buff,"%s_pbk_%s",id.c_str(),"myportnamehere");
//
//    p=new Streaming::MotuControlPort(
//            buff,
//            Streaming::Port::E_Playback,
//            0 // you can add all other port specific stuff you
//              // need to pass by extending MotuXXXPort and MotuPortInfo
//    );
//    free(buff);
//
//    if (!p) {
//        debugOutput(DEBUG_LEVEL_VERBOSE, "Skipped port %s\n",buff);
//    } else {
//        if (!m_transmitProcessor->addPort(p)) {
//            debugWarning("Could not register port with stream processor\n");
//            return false;
//        } else {
//            debugOutput(DEBUG_LEVEL_VERBOSE, "Added port %s\n",buff);
//        }
//    }

    return true;
}

int
MotuDevice::getStreamCount() {
     return 2; // one receive, one transmit
}

Streaming::StreamProcessor *
MotuDevice::getStreamProcessorByIndex(int i) {
    switch (i) {
    case 0:
        return m_receiveProcessor;
    case 1:
         return m_transmitProcessor;
    default:
        return NULL;
    }
    return 0;
}

bool
MotuDevice::startStreamByIndex(int i) {

quadlet_t isoctrl = ReadRegister(MOTU_REG_ISOCTRL);

    if (m_motu_model == MOTU_MODEL_828MkI) {
        // The 828MkI device does this differently.
        // To be implemented
        return false;
    }

    // NOTE: this assumes that you have two streams
    switch (i) {
    case 0:
        // TODO: do the stuff that is nescessary to make the device
        // receive a stream

        // Set the streamprocessor channel to the one obtained by
        // the connection management
        m_receiveProcessor->setChannel(m_iso_recv_channel);

        // Mask out current transmit settings of the MOTU and replace
        // with new ones.  Turn bit 24 on to enable changes to the
        // MOTU's iso transmit settings when the iso control register
        // is written.  Bit 23 enables iso transmit from the MOTU.
        isoctrl &= 0xff00ffff;
        isoctrl |= (m_iso_recv_channel << 16);
        isoctrl |= 0x00c00000;
        WriteRegister(MOTU_REG_ISOCTRL, isoctrl);
        break;
    case 1:
        // TODO: do the stuff that is nescessary to make the device
        // transmit a stream

        // Set the streamprocessor channel to the one obtained by
        // the connection management
        m_transmitProcessor->setChannel(m_iso_send_channel);

        // Mask out current receive settings of the MOTU and replace
        // with new ones.  Turn bit 31 on to enable changes to the
        // MOTU's iso receive settings when the iso control register
        // is written.  Bit 30 enables iso receive by the MOTU.
        isoctrl &= 0x00ffffff;
        isoctrl |= (m_iso_send_channel << 24);
        isoctrl |= 0xc0000000;
        WriteRegister(MOTU_REG_ISOCTRL, isoctrl);
        break;

    default: // Invalid stream index
        return false;
    }

    return true;
}

bool
MotuDevice::stopStreamByIndex(int i) {

quadlet_t isoctrl = ReadRegister(MOTU_REG_ISOCTRL);

    // TODO: connection management: break connection
    // cfr the start function

    if (m_motu_model == MOTU_MODEL_828MkI) {
        // The 828MkI device does this differently.
        // To be implemented
        return false;
    }

    // NOTE: this assumes that you have two streams
    switch (i) {
    case 0:
        // Turn bit 22 off to disable iso send by the MOTU.  Turn
        // bit 23 on to enable changes to the MOTU's iso transmit
        // settings when the iso control register is written.
        isoctrl &= 0xffbfffff;
        isoctrl |= 0x00800000;
        WriteRegister(MOTU_REG_ISOCTRL, isoctrl);
        break;
    case 1:
        // Turn bit 30 off to disable iso receive by the MOTU.  Turn
        // bit 31 on to enable changes to the MOTU's iso receive
        // settings when the iso control register is written.
        isoctrl &= 0xbfffffff;
        isoctrl |= 0x80000000;
        WriteRegister(MOTU_REG_ISOCTRL, isoctrl);
        break;

    default: // Invalid stream index
        return false;
    }

    return true;
}

signed int MotuDevice::getIsoRecvChannel(void) {
    return m_iso_recv_channel;
}

signed int MotuDevice::getIsoSendChannel(void) {
    return m_iso_send_channel;
}

signed int MotuDevice::getDeviceGeneration(void) {
    if (m_motu_model == MOTU_MODEL_828MkI)
        return MOTU_DEVICE_G1;
    if (m_motu_model==MOTU_MODEL_828mk3 ||
        m_motu_model==MOTU_MODEL_ULTRALITEmk3 ||
        m_motu_model==MOTU_MODEL_ULTRALITEmk3_HYB)
        return MOTU_DEVICE_G3;
    return MOTU_DEVICE_G2;
}

unsigned int MotuDevice::getOpticalMode(unsigned int dir,
  unsigned int *port_a_mode, unsigned int *port_b_mode) {
    // Only the "Mark 3" (aka G3) MOTU devices had more than one optical
    // port.  Therefore the "port_b_mode" parameter is unused by all
    // devices other than the Mark 3 devices.
    //
    // If a mode parameter pointer is NULL it will not be returned.
    unsigned int reg;
    unsigned int mask, shift;

    if (port_b_mode != NULL)
        *port_b_mode = MOTU_OPTICAL_MODE_NONE;
    if (getDeviceGeneration()!=MOTU_DEVICE_G3 && port_a_mode==NULL)
        return 0;

    if (m_motu_model == MOTU_MODEL_828MkI) {
        // The early devices used a different register layout.  
        reg = ReadRegister(MOTU_G1_REG_CONFIG);
        mask = (dir==MOTU_DIR_IN)?MOTU_G1_OPT_IN_MODE_MASK:MOTU_G1_OPT_OUT_MODE_MASK;
        shift = (dir==MOTU_DIR_IN)?MOTU_G1_OPT_IN_MODE_BIT0:MOTU_G1_OPT_OUT_MODE_BIT0;
        switch ((reg & mask) >> shift) {
            case MOTU_G1_OPTICAL_OFF: *port_a_mode = MOTU_OPTICAL_MODE_OFF; break;
            case MOTU_G1_OPTICAL_TOSLINK: *port_a_mode = MOTU_OPTICAL_MODE_TOSLINK; break;
            // MOTU_G1_OPTICAL_OFF and MOTU_G1_OPTICAL_ADAT seem to be
            // identical, so currently we don't know how to differentiate
            // these two modes.
            // case MOTU_G1_OPTICAL_ADAT: return MOTU_OPTICAL_MODE_ADAT;
        }
        return 0;
    }

    if (getDeviceGeneration() == MOTU_DEVICE_G3) {
        unsigned int mask, enable, toslink;
        /* The Ultralite Mk3s don't have any optical ports.  All others have 2. */
        if (m_motu_model==MOTU_MODEL_ULTRALITEmk3 || m_motu_model==MOTU_MODEL_ULTRALITEmk3_HYB) {
            if (port_a_mode != NULL)
                *port_a_mode = MOTU_OPTICAL_MODE_NONE;
            if (port_b_mode != NULL)
                *port_b_mode = MOTU_OPTICAL_MODE_NONE;
            return 0;
        }
        reg = ReadRegister(MOTU_G3_REG_OPTICAL_CTRL);
        if (port_a_mode != NULL) {
            mask = (dir==MOTU_DIR_IN)?MOTU_G3_OPT_A_IN_MASK:MOTU_G3_OPT_A_OUT_MASK;
            enable = (dir==MOTU_DIR_IN)?MOTU_G3_OPT_A_IN_ENABLE:MOTU_G3_OPT_A_OUT_ENABLE;
            toslink = (dir==MOTU_DIR_IN)?MOTU_G3_OPT_A_IN_TOSLINK:MOTU_G3_OPT_A_OUT_TOSLINK;
            if ((reg & enable) == 0)
              *port_a_mode = MOTU_OPTICAL_MODE_OFF;
            else
            if ((reg * toslink) == 0)
              *port_a_mode = MOTU_OPTICAL_MODE_TOSLINK;
            else
              *port_a_mode = MOTU_OPTICAL_MODE_ADAT;
        }
        if (port_b_mode != NULL) {
            mask = (dir==MOTU_DIR_IN)?MOTU_G3_OPT_B_IN_MASK:MOTU_G3_OPT_B_OUT_MASK;
            enable = (dir==MOTU_DIR_IN)?MOTU_G3_OPT_B_IN_ENABLE:MOTU_G3_OPT_B_OUT_ENABLE;
            toslink = (dir==MOTU_DIR_IN)?MOTU_G3_OPT_B_IN_TOSLINK:MOTU_G3_OPT_B_OUT_TOSLINK;
            if ((reg & enable) == 0)
              *port_b_mode = MOTU_OPTICAL_MODE_OFF;
            else
            if ((reg * toslink) == 0)
              *port_b_mode = MOTU_OPTICAL_MODE_TOSLINK;
            else
              *port_b_mode = MOTU_OPTICAL_MODE_ADAT;
        }
        return 0;
    }

    reg = ReadRegister(MOTU_REG_ROUTE_PORT_CONF);
    mask = (dir==MOTU_DIR_IN)?MOTU_G2_OPTICAL_IN_MODE_MASK:MOTU_G2_OPTICAL_OUT_MODE_MASK;
    shift = (dir==MOTU_DIR_IN)?MOTU_G2_OPTICAL_IN_MODE_BIT0:MOTU_G2_OPTICAL_OUT_MODE_BIT0;
    switch ((reg & mask) >> shift) {
        case MOTU_G2_OPTICAL_MODE_OFF: *port_a_mode = MOTU_OPTICAL_MODE_OFF; break;
        case MOTU_G2_OPTICAL_MODE_ADAT: *port_a_mode = MOTU_OPTICAL_MODE_ADAT; break;
        case MOTU_G2_OPTICAL_MODE_TOSLINK: *port_a_mode = MOTU_OPTICAL_MODE_TOSLINK; break;
    }
    return 0;
}

signed int MotuDevice::setOpticalMode(unsigned int dir, 
  unsigned int port_a_mode, unsigned int port_b_mode) {
    // Only the "Mark 3" (aka G3) MOTU devices had more than one optical port.
    // Therefore the "port B" mode is ignored for all devices other than
    // the Mark 3 devices.
    unsigned int reg, g2mode;
    unsigned int opt_ctrl = 0x0000002;

    /* THe 896HD doesn't have an SPDIF/TOSLINK optical mode, so don't try to
     * set it
     */
    if (m_motu_model==MOTU_MODEL_896HD && port_a_mode==MOTU_OPTICAL_MODE_TOSLINK)
        return -1;

    if (getDeviceGeneration()!=MOTU_DEVICE_G3 && port_a_mode==MOTU_OPTICAL_MODE_KEEP)
        return 0;

    if (m_motu_model == MOTU_MODEL_828MkI) {
        // The earlier MOTUs handle this differently.
        unsigned int mask, shift, g1mode = 0;
        reg = ReadRegister(MOTU_G1_REG_CONFIG);
        mask = (dir==MOTU_DIR_IN)?MOTU_G1_OPT_IN_MODE_MASK:MOTU_G1_OPT_OUT_MODE_MASK;
        shift = (dir==MOTU_DIR_IN)?MOTU_G1_OPT_IN_MODE_BIT0:MOTU_G1_OPT_OUT_MODE_BIT0;
        switch (port_a_mode) {
            case MOTU_OPTICAL_MODE_OFF: g1mode = MOTU_G1_OPTICAL_OFF; break;
            case MOTU_OPTICAL_MODE_ADAT: g1mode = MOTU_G1_OPTICAL_ADAT; break;
            // See comment in getOpticalMode() about mode ambiguity
            // case MOTU_OPTICAL_MODE_TOSLINK: g1mode = MOTU_G1_OPTICAL_TOSLINK; break;
        }
        reg = (reg & ~mask) | (g1mode << shift);
        return WriteRegister(MOTU_G1_REG_CONFIG, reg);
    }

    /* The G3 devices are also quite a bit different to the G2 units */
    if (getDeviceGeneration() == MOTU_DEVICE_G3) {
        unsigned int mask, enable, toslink;
        reg = ReadRegister(MOTU_G3_REG_OPTICAL_CTRL);
        if (port_a_mode != MOTU_OPTICAL_MODE_KEEP) {
            mask = enable = toslink = 0;
            if (dir & MOTU_DIR_IN) {
                 mask |= MOTU_G3_OPT_A_IN_MASK;
                 enable |= MOTU_G3_OPT_A_IN_ENABLE;
                 toslink |= MOTU_G3_OPT_A_IN_TOSLINK;
            }
            if (dir & MOTU_DIR_OUT) {
                 mask |= MOTU_G3_OPT_A_OUT_MASK;
                 enable |= MOTU_G3_OPT_A_OUT_ENABLE;
                 toslink |= MOTU_G3_OPT_A_OUT_TOSLINK;
            }
            reg = (reg & ~mask) | enable;
            switch (port_a_mode) {
                case MOTU_OPTICAL_MODE_OFF: reg &= ~enable; break;
                case MOTU_OPTICAL_MODE_TOSLINK: reg |= toslink; break;
            }
        }
        if (port_b_mode != MOTU_OPTICAL_MODE_KEEP) {
            mask = enable = toslink = 0;
            if (dir & MOTU_DIR_IN) {
                 mask |= MOTU_G3_OPT_B_IN_MASK;
                 enable |= MOTU_G3_OPT_B_IN_ENABLE;
                 toslink |= MOTU_G3_OPT_B_IN_TOSLINK;
            }
            if (dir & MOTU_DIR_OUT) {
                 mask |= MOTU_G3_OPT_B_OUT_MASK;
                 enable |= MOTU_G3_OPT_B_OUT_ENABLE;
                 toslink |= MOTU_G3_OPT_B_OUT_TOSLINK;
            }
            reg = (reg & ~mask) | enable;
            switch (port_a_mode) {
                case MOTU_OPTICAL_MODE_OFF: reg &= ~enable; break;
                case MOTU_OPTICAL_MODE_TOSLINK: reg |= toslink; break;
            }
            reg = (reg & ~mask) | enable;
            switch (port_b_mode) {
                case MOTU_OPTICAL_MODE_OFF: reg &= ~enable; break;
                case MOTU_OPTICAL_MODE_TOSLINK: reg |= toslink; break;
            }
        }
        return WriteRegister(MOTU_G3_REG_OPTICAL_CTRL, reg);
    }

    reg = ReadRegister(MOTU_REG_ROUTE_PORT_CONF);

    // Map from user mode to values sent to the device registers.
    g2mode = 0;
    switch (port_a_mode) {
        case MOTU_OPTICAL_MODE_OFF: g2mode = MOTU_G2_OPTICAL_MODE_OFF; break;
        case MOTU_OPTICAL_MODE_ADAT: g2mode = MOTU_G2_OPTICAL_MODE_ADAT; break;
        case MOTU_OPTICAL_MODE_TOSLINK: g2mode = MOTU_G2_OPTICAL_MODE_TOSLINK; break;
    }

    // Set up the optical control register value according to the current
    // optical port modes.  At this stage it's not completely understood
    // what the "Optical control" register does, so the values it's set to
    // are more or less "magic" numbers.
    if ((reg & MOTU_G2_OPTICAL_IN_MODE_MASK) != (MOTU_G2_OPTICAL_MODE_ADAT<<MOTU_G2_OPTICAL_IN_MODE_BIT0))
        opt_ctrl |= 0x00000080;
    if ((reg & MOTU_G2_OPTICAL_OUT_MODE_MASK) != (MOTU_G2_OPTICAL_MODE_ADAT<<MOTU_G2_OPTICAL_OUT_MODE_BIT0))
        opt_ctrl |= 0x00000040;

    if (dir & MOTU_DIR_IN) {
        reg &= ~MOTU_G2_OPTICAL_IN_MODE_MASK;
        reg |= (g2mode << MOTU_G2_OPTICAL_IN_MODE_BIT0) & MOTU_G2_OPTICAL_IN_MODE_MASK;
        if (g2mode != MOTU_G2_OPTICAL_MODE_ADAT)
            opt_ctrl |= 0x00000080;
        else
            opt_ctrl &= ~0x00000080;
    }
    if (dir & MOTU_DIR_OUT) {
        reg &= ~MOTU_G2_OPTICAL_OUT_MODE_MASK;
        reg |= (g2mode << MOTU_G2_OPTICAL_OUT_MODE_BIT0) & MOTU_G2_OPTICAL_OUT_MODE_MASK;
        if (g2mode != MOTU_G2_OPTICAL_MODE_ADAT)
            opt_ctrl |= 0x00000040;
        else
            opt_ctrl &= ~0x00000040;
    }

    /* Setting bit 25 in the route/port configuration register enables the
     * setting of the optical mode.  Bit 24 allows the phones assign to be
     * set using the lower 8 bits of the register.  This function has no
     * business setting that, so make sure bit 24 is masked off.
     */
    reg |= 0x02000000;
    reg &= ~0x01000000;

    // FIXME: there seems to be more to it than this, but for
    // the moment at least this seems to work.
    WriteRegister(MOTU_REG_ROUTE_PORT_CONF, reg);
    return WriteRegister(MOTU_REG_OPTICAL_CTRL, opt_ctrl);
}

signed int MotuDevice::getEventSize(unsigned int direction) {
//
// Return the size in bytes of a single event sent to (dir==MOTU_OUT) or
// from (dir==MOTU_IN) the MOTU as part of an iso data packet.
//
// FIXME: for performance it may turn out best to calculate the event
// size in setOpticalMode and cache the result in a data field.  However,
// as it stands this will not adapt to dynamic changes in sample rate - we'd
// need a setFrameRate() for that.
//
// At the very least an event consists of the SPH (4 bytes) and the control/MIDI
// bytes (6 bytes).
// Note that all audio channels are sent using 3 bytes.
signed int sample_rate = getSamplingFrequency();
unsigned int optical_mode_a, optical_mode_b;
signed int size = 4+6;

unsigned int i;
unsigned int dir = direction==Streaming::Port::E_Capture?MOTU_PA_IN:MOTU_PA_OUT;
unsigned int flags = 0;
unsigned int port_flags;

    getOpticalMode(direction, &optical_mode_a, &optical_mode_b);

    if ( sample_rate > 96000 )
        flags |= MOTU_PA_RATE_4x;
    else if ( sample_rate > 48000 )
        flags |= MOTU_PA_RATE_2x;
    else
        flags |= MOTU_PA_RATE_1x;

    switch (optical_mode_a) {
        case MOTU_OPTICAL_MODE_NONE: flags |= MOTU_PA_OPTICAL_ANY; break;
        case MOTU_OPTICAL_MODE_OFF: flags |= MOTU_PA_OPTICAL_OFF; break;
        case MOTU_OPTICAL_MODE_ADAT: flags |= MOTU_PA_OPTICAL_ADAT; break;
        case MOTU_OPTICAL_MODE_TOSLINK: flags |= MOTU_PA_OPTICAL_TOSLINK; break;
    }
    switch (optical_mode_b) {
        case MOTU_OPTICAL_MODE_NONE: flags |= MOTU_PA_MK3_OPT_B_ANY; break;
        case MOTU_OPTICAL_MODE_OFF: flags |= MOTU_PA_MK3_OPT_B_OFF; break;
        case MOTU_OPTICAL_MODE_ADAT: flags |= MOTU_PA_MK3_OPT_B_ADAT; break;
        case MOTU_OPTICAL_MODE_TOSLINK: flags |= MOTU_PA_MK3_OPT_B_TOSLINK; break;
    }

    // Don't test for padding port flag here since we need to include such
    // pseudo-ports when calculating the event size.
    for (i=0; i < DevicesProperty[m_motu_model-1].n_port_entries; i++) {
        port_flags = DevicesProperty[m_motu_model-1].port_entry[i].port_flags;
        /* Make sure the optical port tests return true for devices without
         * one or both optical ports.
         */
        if (optical_mode_a == MOTU_OPTICAL_MODE_NONE) {
            port_flags |= MOTU_PA_OPTICAL_ANY;
        }
        if (optical_mode_b == MOTU_OPTICAL_MODE_NONE) {
            port_flags |= MOTU_PA_MK3_OPT_B_ANY;
        }
        if (( port_flags & dir ) &&
	   ( port_flags & MOTU_PA_RATE_MASK & flags ) &&
	   ( port_flags & MOTU_PA_MK3_OPT_B_MASK & flags ) &&
	   ( port_flags & MOTU_PA_OPTICAL_MASK & flags )) {
            size += 3;
        }
    }

    // Finally round size up to the next quadlet boundary
    return ((size+3)/4)*4;
}
/* ======================================================================= */

bool MotuDevice::addPort(Streaming::StreamProcessor *s_processor,
  char *name, enum Streaming::Port::E_Direction direction,
  int position, int size) {
/*
 * Internal helper function to add a MOTU port to a given stream processor.
 * This just saves the unnecessary replication of what is essentially
 * boilerplate code.  Note that the port name is freed by this function
 * prior to exit.
 */
Streaming::Port *p=NULL;

    p = new Streaming::MotuAudioPort(*s_processor, name, direction, position, size);

    if (!p) {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Skipped port %s\n",name);
    }
    free(name);
    return true;
}
/* ======================================================================= */

bool MotuDevice::addDirPorts(
  enum Streaming::Port::E_Direction direction, unsigned int sample_rate, 
  unsigned int optical_a_mode, unsigned int optical_b_mode) {
/*
 * Internal helper method: adds all required ports for the given direction
 * based on the indicated sample rate and optical mode.
 *
 * Notes: currently ports are not created if they are disabled due to sample
 * rate or optical mode.  However, it might be better to unconditionally
 * create all ports and just disable those which are not active.
 */
const char *mode_str = direction==Streaming::Port::E_Capture?"cap":"pbk";
Streaming::StreamProcessor *s_processor;
unsigned int i;
char *buff;
unsigned int dir = direction==Streaming::Port::E_Capture?MOTU_PA_IN:MOTU_PA_OUT;
unsigned int flags = 0;
unsigned int port_flags;


    if ( sample_rate > 96000 )
        flags |= MOTU_PA_RATE_4x;
    else if ( sample_rate > 48000 )
        flags |= MOTU_PA_RATE_2x;
    else
        flags |= MOTU_PA_RATE_1x;

    switch (optical_a_mode) {
        case MOTU_OPTICAL_MODE_NONE: flags |= MOTU_PA_OPTICAL_ANY; break;
        case MOTU_OPTICAL_MODE_OFF: flags |= MOTU_PA_OPTICAL_OFF; break;
        case MOTU_OPTICAL_MODE_ADAT: flags |= MOTU_PA_OPTICAL_ADAT; break;
        case MOTU_OPTICAL_MODE_TOSLINK: flags |= MOTU_PA_OPTICAL_TOSLINK; break;
    }
    switch (optical_b_mode) {
        case MOTU_OPTICAL_MODE_NONE: flags |= MOTU_PA_MK3_OPT_B_ANY; break;
        case MOTU_OPTICAL_MODE_OFF: flags |= MOTU_PA_MK3_OPT_B_OFF; break;
        case MOTU_OPTICAL_MODE_ADAT: flags |= MOTU_PA_MK3_OPT_B_ADAT; break;
        case MOTU_OPTICAL_MODE_TOSLINK: flags |= MOTU_PA_MK3_OPT_B_TOSLINK; break;
    }

    // retrieve the ID
    std::string id=std::string("dev?");
    if(!getOption("id", id)) {
        debugWarning("Could not retrieve id parameter, defaulting to 'dev?'\n");
    }

    if (direction == Streaming::Port::E_Capture) {
        s_processor = m_receiveProcessor;
    } else {
        s_processor = m_transmitProcessor;
    }

    for (i=0; i < DevicesProperty[m_motu_model-1].n_port_entries; i++) {
        port_flags = DevicesProperty[m_motu_model-1].port_entry[i].port_flags;
        /* For devices without one or more optical ports, ensure the tests
         * on the optical ports always returns "true".
         */
        if (optical_a_mode == MOTU_OPTICAL_MODE_NONE)
            port_flags |= MOTU_PA_OPTICAL_ANY;
        if (optical_b_mode == MOTU_OPTICAL_MODE_NONE)
            port_flags |= MOTU_PA_MK3_OPT_B_ANY;

        if (( port_flags & dir ) &&
	   ( port_flags & MOTU_PA_RATE_MASK & flags ) &&
	   ( port_flags & MOTU_PA_OPTICAL_MASK & flags ) &&
	   ( port_flags & MOTU_PA_MK3_OPT_B_MASK & flags ) &&
	   !( port_flags & MOTU_PA_PADDING )) {
	    asprintf(&buff,"%s_%s_%s" , id.c_str(), mode_str,
              DevicesProperty[m_motu_model-1].port_entry[i].port_name);
            if (!addPort(s_processor, buff, direction, DevicesProperty[m_motu_model-1].port_entry[i].port_offset, 0))
                return false;
        }
    }
    
    return true;
}
/* ======================================================================== */

unsigned int MotuDevice::ReadRegister(fb_nodeaddr_t reg) {
/*
 * Attempts to read the requested register from the MOTU.
 */

    quadlet_t quadlet = 0;

    /* If the supplied register has no upper bits set assume it's a G1/G2
     * register which is assumed to be relative to MOTU_REG_BASE_ADDR.
     */
    if ((reg & MOTU_REG_BASE_ADDR) == 0)
        reg |= MOTU_REG_BASE_ADDR;

    // Note: 1394Service::read() expects a physical ID, not the node id
    if (get1394Service().read(0xffc0 | getNodeId(), reg, 1, &quadlet) <= 0) {
        debugError("Error doing motu read from register 0x%012llx\n",reg);
    }

    return CondSwapFromBus32(quadlet);
}

signed int MotuDevice::WriteRegister(fb_nodeaddr_t reg, quadlet_t data) {
/*
 * Attempts to write the given data to the requested MOTU register.
 */

    unsigned int err = 0;
    data = CondSwapToBus32(data);

    /* If the supplied register has no upper bits set assume it's a G1/G2
     * register which is assumed to be relative to MOTU_REG_BASE_ADDR.
     */
    if ((reg & MOTU_REG_BASE_ADDR) == 0)
        reg |= MOTU_REG_BASE_ADDR;

    // Note: 1394Service::write() expects a physical ID, not the node id
    if (get1394Service().write(0xffc0 | getNodeId(), reg, 1, &data) <= 0) {
        err = 1;
        debugError("Error doing motu write to register 0x%012llx\n",reg);
    }

    SleepRelativeUsec(100);
    return (err==0)?0:-1;
}

}
