/* avdescriptor.cpp
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

#include "avdescriptor.h"
#include <string.h>
#include <errno.h>
#include <libavc1394/avc1394.h>
#include <libavc1394/avc1394_vcr.h>
#include "debugmodule.h"

/* should probably be attached to an AVC device, 
   a descriptor is attached to an AVC device anyway.
   
   a descriptor also always has a type
*/

AvDescriptor::AvDescriptor(AvDevice *parent, quadlet_t target, unsigned int type)
{
	cParent=parent;
	iType=type;
	qTarget=target;
	aContents=NULL;
	bLoaded=false;
	iLength=0;
	bOpen=false;
	iAccessType=0;
}

AvDescriptor::~AvDescriptor()
{
	if (bOpen) {
		Close();
	}
	if (aContents) {
		delete aContents;
	}

}

void AvDescriptor::OpenReadOnly() {
	quadlet_t *response;
	quadlet_t request[3];	
	
	if (!cParent) {
		return;
	}
	
	if(isOpen()) {
		Close();
	}
	
	request[0] = AVC1394_CTYPE_CONTROL | qTarget
					| AVC1394_COMMAND_OPEN_DESCRIPTOR | (iType & 0xFF);
	request[1] = 0x01FFFFFF;
	//fprintf(stderr, "Opening descriptor\n");
	
        response =  cParent->avcExecuteTransaction(request, 2, 2);
	
	if ((response[0]&0xFF000000)==AVC1394_RESPONSE_ACCEPTED) {
		bOpen=true;
		iAccessType=0x01;
	} else {
		Close();
		bOpen=false;
	}
}

void AvDescriptor::OpenReadWrite() {
	quadlet_t *response;
	quadlet_t request[3];	
	
	if (!cParent) {
		return;
	}
	
	if(isOpen()) {
		Close();
	}
		
	request[0] = AVC1394_CTYPE_CONTROL | qTarget
					| AVC1394_COMMAND_OPEN_DESCRIPTOR | (iType & 0xFF);
	request[1] = 0x03FFFFFF;
	//fprintf(stderr, "Opening descriptor\n");
	
        response =  cParent->avcExecuteTransaction(request, 2, 2);
	
	if ((response[0]&0xFF000000)==AVC1394_RESPONSE_ACCEPTED) {
		bOpen=true;
		iAccessType=0x03;
	} else {
		Close();
		bOpen=false;
	}
}

bool AvDescriptor::canWrite() {
	if(bOpen && (iAccessType==0x03) && cParent) {
		return true;
	} else {
		return false;
	}
}

void AvDescriptor::Close() {
	quadlet_t *response;
	quadlet_t request[3];	
	if (!cParent) {
		return;
	}	
	request[0] = AVC1394_CTYPE_CONTROL | qTarget
					| AVC1394_COMMAND_OPEN_DESCRIPTOR | (iType & 0xFF);
	request[1] = 0x00FFFFFF;
	//fprintf(stderr, "Opening descriptor\n");
	
        response =  cParent->avcExecuteTransaction(request, 2, 2);
	
	if ((response[0]&0xFF000000)==AVC1394_RESPONSE_ACCEPTED) { // should always be accepted according to spec
		bOpen=false;
	}
}

/* Load the descriptor
 * no error checking yet
 */

void AvDescriptor::Load() {
	quadlet_t *response;
	quadlet_t request[3];	
	unsigned int i=0;
	unsigned int bytes_read=0;
	unsigned char *databuffer;
	unsigned char read_result_status;
	unsigned int data_length_read;
	
	if (!cParent) {
		return;
	}
	
	/* First determine the descriptor length */
	request[0] = AVC1394_CTYPE_CONTROL | qTarget
					| AVC1394_COMMAND_READ_DESCRIPTOR | (iType & 0xFF);
	request[1] = 0xFFFF0000 | (0x02);
	request[2] = (((0)&0xFFFF) << 16) |0x0000FFFF;
	response =  cParent->avcExecuteTransaction(request, 3, 3);

	iLength=response[2] & 0xFFFF;
	
	//fprintf(stderr,"Descriptor length=0x%04X %d\n",*descriptor_length,*descriptor_length);	

	// now get the rest of the descriptor
	aContents=new unsigned char[iLength];
	
	while(bytes_read<iLength) {
		// apparently the lib modifies the request, so redefine it completely
		request[0] = AVC1394_CTYPE_CONTROL | qTarget
						| AVC1394_COMMAND_READ_DESCRIPTOR | (iType & 0xFF);
		request[1] = 0xFFFF0000 | (iLength-bytes_read)&0xFFFF;
		request[2] = (((bytes_read)&0xFFFF) << 16) |0x0000FFFF;
		response =  cParent->avcExecuteTransaction(request, 3, 3);
		data_length_read=(response[1]&0xFFFF);
		read_result_status=((response[1]>>24)&0xFF);
		databuffer=(unsigned char *)(response+3);
		
		for (i=0;(i<data_length_read) && (bytes_read < iLength);i++) {
			*(aContents+bytes_read)=*(databuffer+i);
			bytes_read++;
		}
	}
	
	bLoaded=true;
}

bool AvDescriptor::isOpen() {
	// maybe we should check this on the device instead of mirroring locally...
	// after all it can time out
	return bOpen;
}

bool AvDescriptor::isLoaded() {
	return bLoaded;
}

unsigned int AvDescriptor::getLength() {
	return iLength;
}

unsigned char AvDescriptor::readByte(unsigned int address) {
	if(cParent && bLoaded && aContents) {
		return *(aContents+address);
	} else {
		return 0; // what to do with this?
	}
}

unsigned int AvDescriptor::readWord(unsigned int address) {
	unsigned int word;
	
	if(cParent && bLoaded && aContents) {
		word=(*(aContents+address)<<8)+*(aContents+address+1);
		return *(aContents+address);
	} else {
		return 0; // what to do with this?
	}
}

unsigned int AvDescriptor::readBuffer(unsigned int address, unsigned int length, unsigned char *buffer) {
	if(cParent && bLoaded && aContents && address < iLength) {
		if(address+length>iLength) {
			length=iLength-address;
		}
		memcpy((void*)buffer, (void*)aContents, length);
		return length;
		
	} else {
		return 0;
	}
}
   