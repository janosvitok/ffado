/* avdevice.h
 * Copyright (C) 2004 by Daniel Wagner, Pieter Palmers
 *
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
#ifndef AVDEVICE_H
#define AVDEVICE_H

#include "ieee1394service.h"

#include <vector>
using std::vector;

class AvDeviceSubunit;

class AvDevice {
 public:
    AvDevice( octlet_t oGuid );
    virtual ~AvDevice();

    void setNodeId( int iNodeId ) 
	{ m_iNodeId = iNodeId; }
    int getNodeId()
	{ return m_iNodeId; }
    void setPort( int iPort )
	{ m_iPort = iPort; }
    void setGeneration( unsigned int iGeneration )
	{ m_iGeneration = iGeneration; }
    unsigned int getGeneration()
	{ return m_iGeneration; }
    octlet_t getGuid()
	{ return m_oGuid; }

    quadlet_t * avcExecuteTransaction( quadlet_t *request, 
				       unsigned int request_len, 
				       unsigned int response_len );
    
    FBReturnCodes initialize();
    bool isInitialised();

    FBReturnCodes setInputPlugSignalFormat(unsigned char plug, unsigned char fmt, quadlet_t fdf);
    FBReturnCodes getInputPlugSignalFormat(unsigned char plug, unsigned char *fmt, quadlet_t *fdf);  
    FBReturnCodes setOutputPlugSignalFormat(unsigned char plug, unsigned char fmt, quadlet_t fdf);
    FBReturnCodes getOutputPlugSignalFormat(unsigned char plug, unsigned char *fmt, quadlet_t *fdf);  
    
//	getSourcePlugConnection();
	void printConnections();
	    
	unsigned char getNbAsyncSourcePlugs() { return iNbAsyncSourcePlugs; } ;
    unsigned char getNbAsyncDestinationPlugs() { return iNbAsyncDestinationPlugs; } ;
    unsigned char getNbIsoSourcePlugs() { return iNbIsoSourcePlugs; } ; // oPCR
    unsigned char getNbIsoDestinationPlugs() { return iNbIsoDestinationPlugs; } ; // iPCR
    unsigned char getNbExtSourcePlugs() { return iNbExtSourcePlugs; } ;
    unsigned char getNbExtDestinationPlugs() { return iNbExtDestinationPlugs; } ;

 protected:
 	AvDeviceSubunit *getSubunit(unsigned char type, unsigned char id);
 
 private:
    int m_iNodeId;
    raw1394handle_t m_handle;
    int m_iPort;
    bool m_bInitialised;
    octlet_t m_oGuid;
    unsigned int m_iGeneration;  //Which generation this device belongs to
    vector< AvDeviceSubunit * > cSubUnits;
    
    unsigned char iNbAsyncDestinationPlugs;
    unsigned char iNbAsyncSourcePlugs;
    unsigned char iNbIsoDestinationPlugs;
    unsigned char iNbIsoSourcePlugs;
    unsigned char iNbExtDestinationPlugs;
    unsigned char iNbExtSourcePlugs;
};

#endif
