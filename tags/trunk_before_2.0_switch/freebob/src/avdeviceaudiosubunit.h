/* avdeviceaudiosubunit.h
 * Copyright (C) 2004 by Pieter Palmers
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

#include "ieee1394service.h"
#include "avdevice.h"
#include "avdevicesubunit.h"

#ifndef AVDEVICEAUDIOSUBUNIT_H
#define AVDEVICEAUDIOSUBUNIT_H

//class AvMusicStatusDescriptor;
class AvAudioSubunitIdentifierDescriptor;

class AvDeviceAudioSubunit : public AvDeviceSubunit {
 public:
    AvDeviceAudioSubunit(AvDevice *parent, unsigned char id);
    virtual ~AvDeviceAudioSubunit();


 private:

	AvAudioSubunitIdentifierDescriptor *cIdentifierDescriptor;
	
 	DECLARE_DEBUG_MODULE;

};

#endif
