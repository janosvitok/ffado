/* motu_avdevice.h
 * Copyright (C) 2006 by Pieter Palmers
 * Copyright (C) 2006 Jonathan Woithe
 *
 * This file is part of FreeBob.
 *
 * FreeBob is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * FreeBob is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeBob; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA.
 */

#ifndef MOTUDEVICE_H
#define MOTUDEVICE_H

#include "iavdevice.h"

#include "debugmodule/debugmodule.h"
#include "libfreebobavc/avc_definitions.h"
#include "libfreebob/xmlparser.h"

#include "libstreaming/MotuStreamProcessor.h"

#define MOTUFW_BASE_ADDR                0xfffff0000000ULL

#define MOTUFW_RATE_BASE_44100          (0<<3)
#define MOTUFW_RATE_BASE_48000          (1<<3)
#define MOTUFW_RATE_MULTIPLIER_1X       (0<<4)
#define MOTUFW_RATE_MULTIPLIER_2X       (1<<4)
#define MOTUFW_RATE_MULTIPLIER_4X       (2<<4)
#define MOTUFW_RATE_BASE_MASK           (0x00000008)
#define MOTUFW_RATE_MULTIPLIER_MASK     (0x00000030)

#define MOTUFW_OPTICAL_MODE_OFF		(0<<8)
#define MOTUFW_OPTICAL_MODE_ADAT	(1<<8)
#define MOTUFW_OPTICAL_MODE_TOSLINK	(2<<8)
#define MOTUFW_OPTICAL_MODE_MASK	(0x00000300)

#define MOTUFW_CLKSRC_MASK		0x00000007
#define MOTUFW_CLKSRC_INTERNAL		0
#define MOTUFW_CLKSRC_ADAT_OPTICAL	1
#define MOTUFW_CLKSRC_SPDIF_TOSLINK	2
#define MOTUFW_CLKSRC_SMTPE		3
#define MOTUFW_CLKSRC_WORDCLOCK		4
#define MOTUFW_CLKSRC_ADAT_9PIN		5
#define MOTUFW_CLKSRC_AES_EBU		7

/* Device registers */
#define MOTUFW_REG_ISOCTRL		0x0b00
#define MOTUFW_REG_RATECTRL		0x0b14
#define MOTUFW_REG_ROUTE_PORT_CONF      0x0c04
#define MOTUFW_REG_CLKSRC_NAME0		0x0c60

class ConfigRom;
class Ieee1394Service;

namespace Motu {

class MotuDevice : public IAvDevice {
public:
    enum EMotuModel {
      MOTUFW_MODEL_NONE     = 0x0000,
      MOTUFW_MODEL_828mkII  = 0x0001,
      MOTUFW_MODEL_TRAVELER = 0x0002,
    };

    MotuDevice( Ieee1394Service& ieee1394Service,
		  int nodeId,
		  int verboseLevel );
    virtual ~MotuDevice();

    virtual bool discover();
    virtual ConfigRom& getConfigRom() const;

    // obsolete, will be removed soon, unused
    virtual bool addXmlDescription( xmlNodePtr deviceNode ) {return true;};

    virtual void showDevice() const;

    virtual bool setSamplingFrequency( ESamplingFrequency samplingFrequency );
    virtual int getSamplingFrequency( );

    virtual bool setId(unsigned int id);

    virtual int getStreamCount();
    virtual FreebobStreaming::StreamProcessor *getStreamProcessorByIndex(int i);

    virtual bool prepare();

    virtual int startStreamByIndex(int i);
    virtual int stopStreamByIndex(int i);

    signed int getIsoRecvChannel(void);
    signed int getIsoSendChannel(void);
    unsigned int getOpticalMode(void);
    signed int setOpticalMode(unsigned int mode);

    signed int getEventSize(void);
  
protected:
    Ieee1394Service* m_1394Service;
    ConfigRom*       m_configRom;
    signed int       m_motu_model;
    int              m_nodeId;
    int              m_verboseLevel;
    signed int m_id;
    signed int m_iso_recv_channel, m_iso_send_channel;
    signed int m_bandwidth;
    
	FreebobStreaming::MotuReceiveStreamProcessor *m_receiveProcessor;
	FreebobStreaming::MotuTransmitStreamProcessor *m_transmitProcessor;

private:
	bool addPort(FreebobStreaming::StreamProcessor *s_processor,
		char *name, 
		enum FreebobStreaming::Port::E_Direction direction,
		int position, int size);
	bool MotuDevice::addDirPorts(
		enum FreebobStreaming::Port::E_Direction direction,
		unsigned int sample_rate, unsigned int optical_mode);
        
	unsigned int ReadRegister(unsigned int reg);
	signed int WriteRegister(unsigned int reg, quadlet_t data);

        // IEEE1394 Vendor IDs.  One would expect only MOTU, but you never
        // know if a clone might appear some day.
        enum EVendorId {
                MOTUFW_VENDOR_MOTU = 0x000001f2,
        };
        
        // IEEE1394 Unit directory version IDs for different MOTU hardware
        enum EUnitVersionId {
                MOTUFW_UNITVER_828mkII  = 0x00000003,
                MOTUFW_UNITVER_TRAVELER = 0x00000009,
        };

    // debug support
    DECLARE_DEBUG_MODULE;
};

}

#endif
