/* avdevicesubunit.h
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

#ifndef AVDEVICESUBUNIT_H
#define AVDEVICESUBUNIT_H

class AvDeviceSubunit {
 public:
    AvDeviceSubunit(AvDevice *parent, unsigned char target, unsigned char id);
    virtual ~AvDeviceSubunit();
    bool isValid();
    unsigned char getNbDestinationPlugs();
    unsigned char getNbSourcePlugs();

    virtual void test();
    void printOutputPlugConnections();
    
 protected:
 	bool bValid;    
 	unsigned char iNbDestinationPlugs;
 	unsigned char iNbSourcePlugs;

    unsigned char iTarget;
    unsigned char iId;
    AvDevice *cParent;
   
 private:

};

#endif
