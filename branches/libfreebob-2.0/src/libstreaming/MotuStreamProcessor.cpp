/* $Id$ */

/*
 *   FreeBob Streaming API
 *   FreeBob = Firewire (pro-)audio for linux
 *
 *   http://freebob.sf.net
 *
 *   Copyright (C) 2005,2006 Pieter Palmers <pieterpalmers@users.sourceforge.net>
 *   Copyright (C) 2005,2006 Pieter Palmers <pieterpalmers@users.sourceforge.net>
 *
 *   This program is free software {} you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation {} either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY {} without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program {} if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 *
 */

#include "MotuStreamProcessor.h"
#include "Port.h"
#include "MotuPort.h"

namespace FreebobStreaming {

IMPL_DEBUG_MODULE( MotuTransmitStreamProcessor, MotuTransmitStreamProcessor, DEBUG_LEVEL_NORMAL );
IMPL_DEBUG_MODULE( MotuReceiveStreamProcessor, MotuReceiveStreamProcessor, DEBUG_LEVEL_NORMAL );

/* transmit */
MotuTransmitStreamProcessor::MotuTransmitStreamProcessor(int port, int framerate)
	: TransmitStreamProcessor(port, framerate),m_dimension(0) {


}

MotuTransmitStreamProcessor::~MotuTransmitStreamProcessor() {
	freebob_ringbuffer_free(m_event_buffer);
	free(m_cluster_buffer);
}

bool MotuTransmitStreamProcessor::init() {

	debugOutput( DEBUG_LEVEL_VERBOSE, "Initializing (%p)...\n");
	// call the parent init
	// this has to be done before allocating the buffers, 
	// because this sets the buffersizes from the processormanager
	if(!TransmitStreamProcessor::init()) {
		debugFatal("Could not do base class init (%p)\n",this);
		return false;
	}
	

	return true;
}

void MotuTransmitStreamProcessor::setVerboseLevel(int l) {
	setDebugLevel(l); // sets the debug level of the current object
	TransmitStreamProcessor::setVerboseLevel(l); // also set the level of the base class
}


enum raw1394_iso_disposition
MotuTransmitStreamProcessor::getPacket(unsigned char *data, unsigned int *length,
	              unsigned char *tag, unsigned char *sy,
	              int cycle, unsigned int dropped, unsigned int max_length) {

	enum raw1394_iso_disposition retval = RAW1394_ISO_OK;
	
	// signal that we are running
	// this is to allow the manager to wait untill all streams are up&running
	// it can take some time before the devices start to transmit.
	// if we would transmit ourselves, we'd have instant buffer underrun
	// this works in cooperation with the m_disabled value
	
	// TODO: add code here to detect that a stream is running
	// NOTE: xmit streams are most likely 'always' ready
	m_running=true;
	
	// don't process the stream when it is not enabled.
	// however, maybe we do have to generate (semi) valid packets
	if(m_disabled) {
		*length = 0; 
		*tag = 1; // TODO: is this correct for MOTU?
		*sy = 0;
		return RAW1394_ISO_OK;
	}
	
    debugOutput( DEBUG_LEVEL_VERY_VERBOSE, "get packet...\n");
	
	// construct the packet cip

    // TODO: calculate read_size here.
    // note: an 'event' is one sample from all channels + possibly other midi and control data
    int nevents=0; // TODO: determine
	int read_size=nevents*m_dimension*sizeof(quadlet_t); // assumes each channel takes one quadlet

    // we read the packet data from a ringbuffer, because of efficiency
    // that allows us to construct the packets one period at once
	if ((freebob_ringbuffer_read(m_event_buffer,(char *)(data+8),read_size)) < 
				read_size) 
	{
        /* there is no more data in the ringbuffer */
        debugWarning("Transmit buffer underrun (cycle %d, FC=%d, PC=%d)\n", 
                 cycle, m_framecounter, m_handler->getPacketCount());
        
        // signal underrun
        m_xruns++;

        retval=RAW1394_ISO_DEFER; // make raw1394_loop_iterate exit its inner loop
        *length=0;
        nevents=0;

    } else {
        retval=RAW1394_ISO_OK;
        *length = read_size + 8;
        
        // process all ports that should be handled on a per-packet base
        // this is MIDI for AMDTP (due to the need of DBC, which is lost 
        // when putting the events in the ringbuffer)
        // for motu this might also be control data, however as control
        // data isn't time specific I would also include it in the period
        // based processing
        
        int dbc=0;//get this from your packet, if you need it. otherwise change encodePacketPorts
        if (!encodePacketPorts((quadlet_t *)(data+8), nevents, dbc)) {
            debugWarning("Problem encoding Packet Ports\n");
        }
    }
    
    *tag = 1; // TODO: is this correct for MOTU?
    *sy = 0;
    
    // update the frame counter
    incrementFrameCounter(nevents);
    // keep this at the end, because otherwise the raw1394_loop_iterate functions inner loop
    // keeps requesting packets, that are not nescessarily ready
    if(m_framecounter>m_period) {
       retval=RAW1394_ISO_DEFER;
    }
	
    return retval;

}

bool MotuTransmitStreamProcessor::isOnePeriodReady()
{ 
     // TODO: this is the way you can implement sync
     //       only when this returns true, one period will be
     //       transferred to the audio api side.
     //       you can delay this moment as long as you
     //       want (provided that there is enough buffer space)
     
     // this implementation just waits until there is one period of samples
     // transmitted from the buffer
     return (m_framecounter > (int)m_period); 
}
 
bool MotuTransmitStreamProcessor::prefill() {
    // this is needed because otherwise there is no data to be 
    // sent when the streaming starts
    
    int i=m_nb_buffers;
    while(i--) {
        if(!transferSilence(m_period)) {
            debugFatal("Could not prefill transmit stream\n");
            return false;
        }
    }
    
    return true;
    
}

bool MotuTransmitStreamProcessor::reset() {

    debugOutput( DEBUG_LEVEL_VERBOSE, "Resetting...\n");

    // reset the event buffer, discard all content
    freebob_ringbuffer_reset(m_event_buffer);
    
    // reset all non-device specific stuff
    // i.e. the iso stream and the associated ports
    if(!TransmitStreamProcessor::reset()) {
        debugFatal("Could not do base class reset\n");
        return false;
    }

    // we should prefill the event buffer
    if (!prefill()) {
        debugFatal("Could not prefill buffers\n");
        return false;    
    }
    
    return true;
}

bool MotuTransmitStreamProcessor::prepare() {
    
    debugOutput( DEBUG_LEVEL_VERBOSE, "Preparing...\n");
    
    // prepare all non-device specific stuff
    // i.e. the iso stream and the associated ports
    if(!TransmitStreamProcessor::prepare()) {
        debugFatal("Could not prepare base class\n");
        return false;
    }
    
    // allocate the event buffer
    unsigned int ringbuffer_size_frames=m_nb_buffers * m_period;
    
    if( !(m_event_buffer=freebob_ringbuffer_create(
            (m_dimension * ringbuffer_size_frames) * sizeof(quadlet_t)))) {
        debugFatal("Could not allocate memory event ringbuffer");
        return false;
    }

    // allocate the temporary cluster buffer
    // this is needed for the efficient transfer() routine
    // it's size has to be equal to one 'event'
    if( !(m_cluster_buffer=(char *)calloc(m_dimension,sizeof(quadlet_t)))) {
        debugFatal("Could not allocate temporary cluster buffer");
        freebob_ringbuffer_free(m_event_buffer);
        return false;
    }

    // set the parameters of ports we can:
    // we want the audio ports to be period buffered,
    // and the midi ports to be packet buffered
    for ( PortVectorIterator it = m_Ports.begin();
          it != m_Ports.end();
          ++it )
    {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Setting up port %s\n",(*it)->getName().c_str());
        if(!(*it)->setBufferSize(m_period)) {
            debugFatal("Could not set buffer size to %d\n",m_period);
            return false;
        }
        
        
        switch ((*it)->getPortType()) {
            case Port::E_Audio:
                if(!(*it)->setSignalType(Port::E_PeriodSignalled)) {
                    debugFatal("Could not set signal type to PeriodSignalling");
                    return false;
                }
                
                break;
            case Port::E_Midi:
                if(!(*it)->setSignalType(Port::E_PacketSignalled)) {
                    debugFatal("Could not set signal type to PacketSignalling");
                    return false;
                }
                
                break;
                
            case Port::E_Control:
                if(!(*it)->setSignalType(Port::E_PeriodSignalled)) {
                    debugFatal("Could not set signal type to PeriodSignalling");
                    return false;
                }
                
                break;
            default:
                debugWarning("Unsupported port type specified\n");
                break;
        }
    }

    // the API specific settings of the ports are already set before
    // this routine is called, therefore we can init&prepare the ports
    if(!initPorts()) {
        debugFatal("Could not initialize ports!\n");
        return false;
    }

    if(!preparePorts()) {
        debugFatal("Could not initialize ports!\n");
        return false;
    }

    // we should prefill the event buffer
    if (!prefill()) {
        debugFatal("Could not prefill buffers\n");
        return false;    
    }

    return true;

}

bool MotuTransmitStreamProcessor::transferSilence(unsigned int size) {
    
    // this function should tranfer 'size' frames of 'silence' to the event buffer
    unsigned int write_size=size*sizeof(quadlet_t)*m_dimension;
    char *dummybuffer=(char *)calloc(sizeof(quadlet_t),size*m_dimension);
    transmitSilenceBlock(dummybuffer, size, 0);

    if (freebob_ringbuffer_write(m_event_buffer,(char *)(dummybuffer),write_size) < write_size) {
        debugWarning("Could not write to event buffer\n");
    }
    
    free(dummybuffer);
    
    return true;
}

bool MotuTransmitStreamProcessor::transfer() {
    m_PeriodStat.mark(freebob_ringbuffer_read_space(m_event_buffer)/(4*m_dimension));

    debugOutput( DEBUG_LEVEL_VERY_VERBOSE, "Transferring period...\n");
    // TODO: improve
/* a naive implementation would look like this:

    unsigned int write_size=m_period*sizeof(quadlet_t)*m_dimension;
    char *dummybuffer=(char *)calloc(sizeof(quadlet_t),m_period*m_dimension);
    
    transmitBlock(dummybuffer, m_period, 0, 0);

    if (freebob_ringbuffer_write(m_event_buffer,(char *)(dummybuffer),write_size) < write_size) {
        debugWarning("Could not write to event buffer\n");
    }


    free(dummybuffer);
*/
/* but we're not that naive anymore... */
    int xrun;
    unsigned int offset=0;
    
    freebob_ringbuffer_data_t vec[2];
    // we received one period of frames
    // this is period_size*dimension of events
    int events2write=m_period*m_dimension;
    int bytes2write=events2write*sizeof(quadlet_t);

    /* write events2write bytes to the ringbuffer 
    *  first see if it can be done in one read.
    *  if so, ok. 
    *  otherwise write up to a multiple of clusters directly to the buffer
    *  then do the buffer wrap around using ringbuffer_write
    *  then write the remaining data directly to the buffer in a third pass 
    *  Make sure that we cannot end up on a non-cluster aligned position!
    */
    int cluster_size=m_dimension*sizeof(quadlet_t);

    while(bytes2write>0) {
        int byteswritten=0;
        
        unsigned int frameswritten=(m_period*cluster_size-bytes2write)/cluster_size;
        offset=frameswritten;
        
        freebob_ringbuffer_get_write_vector(m_event_buffer, vec);
            
        if(vec[0].len==0) { // this indicates a full event buffer
            debugError("XMT: Event buffer overrun in processor %p\n",this);
            break;
        }
            
        /* if we don't take care we will get stuck in an infinite loop
        * because we align to a cluster boundary later
        * the remaining nb of bytes in one write operation can be 
        * smaller than one cluster
        * this can happen because the ringbuffer size is always a power of 2
        */
        if(vec[0].len<cluster_size) {
            
            // encode to the temporary buffer
            xrun = transmitBlock(m_cluster_buffer, 1, offset);
            
            if(xrun<0) {
                // xrun detected
                debugError("XMT: Frame buffer underrun in processor %p\n",this);
                break;
            }
                
            // use the ringbuffer function to write one cluster 
            // the write function handles the wrap around.
            freebob_ringbuffer_write(m_event_buffer,
                         m_cluster_buffer,
                         cluster_size);
                
            // we advanced one cluster_size
            bytes2write-=cluster_size;
                
        } else { // 
            
            if(bytes2write>vec[0].len) {
                // align to a cluster boundary
                byteswritten=vec[0].len-(vec[0].len%cluster_size);
            } else {
                byteswritten=bytes2write;
            }
                
            xrun = transmitBlock(vec[0].buf,
                         byteswritten/cluster_size,
                         offset);
            
            if(xrun<0) {
                    // xrun detected
                debugError("XMT: Frame buffer underrun in processor %p\n",this);
                break;
            }

            freebob_ringbuffer_write_advance(m_event_buffer, byteswritten);
            bytes2write -= byteswritten;
        }

        // the bytes2write should always be cluster aligned
        assert(bytes2write%cluster_size==0);

    }

    return true;
}
/* 
 * write received events to the stream ringbuffers.
 */

int MotuTransmitStreamProcessor::transmitBlock(char *data, 
                       unsigned int nevents, unsigned int offset)
{
    int problem=0;

    for ( PortVectorIterator it = m_PeriodPorts.begin();
          it != m_PeriodPorts.end();
          ++it )
    {
        // if this port is disabled, don't process it
        if((*it)->isDisabled()) {continue;};
        
        //FIXME: make this into a static_cast when not DEBUG?

        MotuPortInfo *pinfo=dynamic_cast<MotuPortInfo *>(*it);
        assert(pinfo); // this should not fail!!

/*      This is the AMDTP way, the motu way is different
        Leaving this in as reference
        
        switch(pinfo->getFormat()) {
        
        
        case MotuPortInfo::E_MBLA:
            if(encodePortToMBLAEvents(static_cast<MotuAudioPort *>(*it), (quadlet_t *)data, offset, nevents)) {
                debugWarning("Could not encode port %s to MBLA events",(*it)->getName().c_str());
                problem=1;
            }
            break;
        case MotuPortInfo::E_SPDIF: // still unimplemented
            break;
        default: // ignore
            break;
        }
*/
    }
    return problem;

}

int MotuTransmitStreamProcessor::transmitSilenceBlock(char *data, 
                       unsigned int nevents, unsigned int offset)
{
    int problem=0;

    for ( PortVectorIterator it = m_PeriodPorts.begin();
          it != m_PeriodPorts.end();
          ++it )
    {

        //FIXME: make this into a static_cast when not DEBUG?

        MotuPortInfo *pinfo=dynamic_cast<MotuPortInfo *>(*it);
        assert(pinfo); // this should not fail!!

/* this is the same as the non-silence version, except that is doesn't read from the port buffers
        switch(pinfo->getFormat()) {
        

        case MotuPortInfo::E_MBLA:
            if(encodeSilencePortToMBLAEvents(static_cast<MotuAudioPort *>(*it), (quadlet_t *)data, offset, nevents)) {
                debugWarning("Could not encode port %s to MBLA events",(*it)->getName().c_str());
                problem=1;
            }
            break;
        case MotuPortInfo::E_SPDIF: // still unimplemented
            break;
        default: // ignore
            break;
        }
        */
    }
    return problem;

}

/**
 * @brief decode a packet for the packet-based ports
 *
 * @param data Packet data
 * @param nevents number of events in data (including events of other ports & port types)
 * @param dbc DataBlockCount value for this packet
 * @return true if all successfull
 */
bool MotuTransmitStreamProcessor::encodePacketPorts(quadlet_t *data, unsigned int nevents, unsigned int dbc)
{
    bool ok=true;
    char byte;
    
    quadlet_t *target_event=NULL;
    int j;
    
    for ( PortVectorIterator it = m_PacketPorts.begin();
          it != m_PacketPorts.end();
          ++it )
    {

#ifdef DEBUG
        MotuPortInfo *pinfo=dynamic_cast<MotuPortInfo *>(*it);
        assert(pinfo); // this should not fail!!

        // the only packet type of events for AMDTP is MIDI in mbla
//         assert(pinfo->getFormat()==MotuPortInfo::E_Midi);
#endif
        
        MotuMidiPort *mp=static_cast<MotuMidiPort *>(*it);
        
        // TODO: decode the midi (or other type) stuff here

    }
        
    return ok;
}

/* Left in as reference, this is highly AMDTP related

basic idea:

iterate over the ports
- get port buffer address
- loop over events
  * pick right sample in event based upon PortInfo
  * convert sample from Port format (E_Int24, E_Float, ..) to native format

not that in order to use the 'efficient' transfer method, you have to make sure that
you can start from an offset (expressed in frames).

int MotuTransmitStreamProcessor::encodePortToMBLAEvents(MotuAudioPort *p, quadlet_t *data, 
                       unsigned int offset, unsigned int nevents)
{
    unsigned int j=0;

    quadlet_t *target_event;

    target_event=(quadlet_t *)(data + p->getPosition());

    switch(p->getDataType()) {
        default:
        case Port::E_Int24:
            {
                quadlet_t *buffer=(quadlet_t *)(p->getBufferAddress());

                assert(nevents + offset <= p->getBufferSize());

                buffer+=offset;

                for(j = 0; j < nevents; j += 1) { // decode max nsamples
                    *target_event = htonl((*(buffer) & 0x00FFFFFF) | 0x40000000);
                    buffer++;
                    target_event += m_dimension;
                }
            }
            break;
        case Port::E_Float:
            {
                const float multiplier = (float)(0x7FFFFF00);
                float *buffer=(float *)(p->getBufferAddress());

                assert(nevents + offset <= p->getBufferSize());

                buffer+=offset;

                for(j = 0; j < nevents; j += 1) { // decode max nsamples		
    
                    // don't care for overflow
                    float v = *buffer * multiplier;  // v: -231 .. 231
                    unsigned int tmp = ((int)v);
                    *target_event = htonl((tmp >> 8) | 0x40000000);
                    
                    buffer++;
                    target_event += m_dimension;
                }
            }
            break;
    }

    return 0;
}
*/
/*
int MotuTransmitStreamProcessor::encodeSilencePortToMBLAEvents(MotuAudioPort *p, quadlet_t *data, 
                       unsigned int offset, unsigned int nevents)
{
    unsigned int j=0;

    quadlet_t *target_event;

    target_event=(quadlet_t *)(data + p->getPosition());

    switch(p->getDataType()) {
        default:
        case Port::E_Int24:
        case Port::E_Float:
            {
                for(j = 0; j < nevents; j += 1) { // decode max nsamples
                    *target_event = htonl(0x40000000);
                    target_event += m_dimension;
                }
            }
            break;
    }

    return 0;
}
*/

/* --------------------- RECEIVE ----------------------- */

MotuReceiveStreamProcessor::MotuReceiveStreamProcessor(int port, int framerate)
    : ReceiveStreamProcessor(port, framerate), m_dimension(0) {


}

MotuReceiveStreamProcessor::~MotuReceiveStreamProcessor() {
    freebob_ringbuffer_free(m_event_buffer);
    free(m_cluster_buffer);

}

bool MotuReceiveStreamProcessor::init() {

    // call the parent init
    // this has to be done before allocating the buffers, 
    // because this sets the buffersizes from the processormanager
    if(!ReceiveStreamProcessor::init()) {
        debugFatal("Could not do base class init (%d)\n",this);
        return false;
    }

    return true;
}

enum raw1394_iso_disposition 
MotuReceiveStreamProcessor::putPacket(unsigned char *data, unsigned int length, 
                  unsigned char channel, unsigned char tag, unsigned char sy, 
                  unsigned int cycle, unsigned int dropped) {
    
    enum raw1394_iso_disposition retval=RAW1394_ISO_OK;
    
    if(1 /* if there are events in this packet */) {
    
        unsigned int nevents=0; // TODO: calculate the number of events here
        
        // signal that we're running
		if(nevents) m_running=true;
        
        // don't process the stream when it is not enabled.
        if(m_disabled) {
            return RAW1394_ISO_OK;
        }
        
        debugOutput( DEBUG_LEVEL_VERY_VERBOSE, "put packet...\n");
        
        // see transmit processor
        unsigned int write_size=nevents*sizeof(quadlet_t)*m_dimension;
        
        // add the data payload (events) to the ringbuffer
        if (freebob_ringbuffer_write(m_event_buffer,(char *)(data+8),write_size) < write_size) 
        {
            debugWarning("Receive buffer overrun (cycle %d, FC=%d, PC=%d)\n", 
                 cycle, m_framecounter, m_handler->getPacketCount());
            m_xruns++;

            retval=RAW1394_ISO_DEFER;
        } else {
            retval=RAW1394_ISO_OK;
            // process all ports that should be handled on a per-packet base
            // this is MIDI for AMDTP (due to the need of DBC)
            int dbc=0; //get this from packet if needed, else change decodePacketPorts
            if (!decodePacketPorts((quadlet_t *)(data+8), nevents, dbc)) {
                debugWarning("Problem decoding Packet Ports\n");
                retval=RAW1394_ISO_DEFER;
            }
            
            // time stamp processing can be done here
        
        }

        // update the frame counter
        incrementFrameCounter(nevents);
        // keep this at the end, because otherwise the raw1394_loop_iterate functions inner loop
        // keeps requesting packets without going to the xmit handler, leading to xmit starvation
        if(m_framecounter>m_period) {
           retval=RAW1394_ISO_DEFER;
        }
        
    } else { // no events in packet
        // discard packet
        // can be important for sync though
    }
    
    return retval;
}

bool MotuReceiveStreamProcessor::isOnePeriodReady() { 
     // TODO: this is the way you can implement sync
     //       only when this returns true, one period will be
     //       transferred to the audio api side.
     //       you can delay this moment as long as you
     //       want (provided that there is enough buffer space)
     
     // this implementation just waits until there is one period of samples
     // received into the buffer
    if(m_framecounter > (int)m_period) {
        return true;
    }
    return false;
}

void MotuReceiveStreamProcessor::setVerboseLevel(int l) {
	setDebugLevel(l);
 	ReceiveStreamProcessor::setVerboseLevel(l);

}


bool MotuReceiveStreamProcessor::reset() {

	debugOutput( DEBUG_LEVEL_VERBOSE, "Resetting...\n");

	// reset the event buffer, discard all content
	freebob_ringbuffer_reset(m_event_buffer);
	
	// reset all non-device specific stuff
	// i.e. the iso stream and the associated ports
	if(!ReceiveStreamProcessor::reset()) {
		debugFatal("Could not do base class reset\n");
		return false;
	}
	
	return true;
}

bool MotuReceiveStreamProcessor::prepare() {

	// prepare all non-device specific stuff
	// i.e. the iso stream and the associated ports
	if(!ReceiveStreamProcessor::prepare()) {
		debugFatal("Could not prepare base class\n");
		return false;
	}
	
	debugOutput( DEBUG_LEVEL_VERBOSE, "Preparing...\n");

    // setup any specific stuff here

    // allocate the event buffer
    unsigned int ringbuffer_size_frames=m_nb_buffers * m_period;
    
    if( !(m_event_buffer=freebob_ringbuffer_create(
            (m_dimension * ringbuffer_size_frames) * sizeof(quadlet_t)))) {
		debugFatal("Could not allocate memory event ringbuffer");
		return false;
	}

	// allocate the temporary cluster buffer
	if( !(m_cluster_buffer=(char *)calloc(m_dimension,sizeof(quadlet_t)))) {
		debugFatal("Could not allocate temporary cluster buffer");
		freebob_ringbuffer_free(m_event_buffer);
		return false;
	}

	// set the parameters of ports we can:
	// we want the audio ports to be period buffered,
	// and the midi ports to be packet buffered
	for ( PortVectorIterator it = m_Ports.begin();
		  it != m_Ports.end();
		  ++it )
	{
		debugOutput(DEBUG_LEVEL_VERBOSE, "Setting up port %s\n",(*it)->getName().c_str());
		
		if(!(*it)->setBufferSize(m_period)) {
			debugFatal("Could not set buffer size to %d\n",m_period);
			return false;
		}

		switch ((*it)->getPortType()) {
			case Port::E_Audio:
				if(!(*it)->setSignalType(Port::E_PeriodSignalled)) {
					debugFatal("Could not set signal type to PeriodSignalling");
					return false;
				}
				break;
			case Port::E_Midi:
				if(!(*it)->setSignalType(Port::E_PacketSignalled)) {
					debugFatal("Could not set signal type to PacketSignalling");
					return false;
				}
				break;
			case Port::E_Control:
				if(!(*it)->setSignalType(Port::E_PeriodSignalled)) {
					debugFatal("Could not set signal type to PeriodSignalling");
					return false;
				}
				break;
			default:
				debugWarning("Unsupported port type specified\n");
				break;
		}

	}

    // the API specific settings of the ports are already set before
    // this routine is called, therefore we can init&prepare the ports
	if(!initPorts()) {
		debugFatal("Could not initialize ports!\n");
		return false;
	}

	if(!preparePorts()) {
		debugFatal("Could not initialize ports!\n");
		return false;
	}
	
	return true;

}

bool MotuReceiveStreamProcessor::transfer() {

    // the same idea as the transmit processor
    
	debugOutput( DEBUG_LEVEL_VERY_VERBOSE, "Transferring period...\n");
	
/* another naive section:	
	unsigned int read_size=m_period*sizeof(quadlet_t)*m_dimension;
	char *dummybuffer=(char *)calloc(sizeof(quadlet_t),m_period*m_dimension);
	if (freebob_ringbuffer_read(m_event_buffer,(char *)(dummybuffer),read_size) < read_size) {
		debugWarning("Could not read from event buffer\n");
	}

	receiveBlock(dummybuffer, m_period, 0);

	free(dummybuffer);
*/
	int xrun;
	unsigned int offset=0;
	
	freebob_ringbuffer_data_t vec[2];
	// we received one period of frames on each connection
	// this is period_size*dimension of events

	int events2read=m_period*m_dimension;
	int bytes2read=events2read*sizeof(quadlet_t);
	/* read events2read bytes from the ringbuffer 
	*  first see if it can be done in one read. 
	*  if so, ok. 
	*  otherwise read up to a multiple of clusters directly from the buffer
	*  then do the buffer wrap around using ringbuffer_read
	*  then read the remaining data directly from the buffer in a third pass 
	*  Make sure that we cannot end up on a non-cluster aligned position!
	*/
	int cluster_size=m_dimension*sizeof(quadlet_t);
	
	while(bytes2read>0) {
		unsigned int framesread=(m_period*cluster_size-bytes2read)/cluster_size;
		offset=framesread;
		
		int bytesread=0;

		freebob_ringbuffer_get_read_vector(m_event_buffer, vec);
			
		if(vec[0].len==0) { // this indicates an empty event buffer
			debugError("RCV: Event buffer underrun in processor %p\n",this);
			break;
		}
			
		/* if we don't take care we will get stuck in an infinite loop
		* because we align to a cluster boundary later
		* the remaining nb of bytes in one read operation can be smaller than one cluster
		* this can happen because the ringbuffer size is always a power of 2
			*/
		if(vec[0].len<cluster_size) {
			// use the ringbuffer function to read one cluster 
			// the read function handles wrap around
			freebob_ringbuffer_read(m_event_buffer,m_cluster_buffer,cluster_size);

			xrun = receiveBlock(m_cluster_buffer, 1, offset);
				
			if(xrun<0) {
				// xrun detected
				debugError("RCV: Frame buffer overrun in processor %p\n",this);
				break;
			}
				
				// we advanced one cluster_size
			bytes2read-=cluster_size;
				
		} else { // 
			
			if(bytes2read>vec[0].len) {
					// align to a cluster boundary
				bytesread=vec[0].len-(vec[0].len%cluster_size);
			} else {
				bytesread=bytes2read;
			}
				
			xrun = receiveBlock(vec[0].buf, bytesread/cluster_size, offset);
				
			if(xrun<0) {
					// xrun detected
				debugError("RCV: Frame buffer overrun in processor %p\n",this);
				break;
			}

			freebob_ringbuffer_read_advance(m_event_buffer, bytesread);
			bytes2read -= bytesread;
		}
			
		// the bytes2read should always be cluster aligned
		assert(bytes2read%cluster_size==0);
	}

	return true;
}

/**
 * \brief write received events to the port ringbuffers.
 */
int MotuReceiveStreamProcessor::receiveBlock(char *data, 
					   unsigned int nevents, unsigned int offset)
{
	int problem=0;

	for ( PortVectorIterator it = m_PeriodPorts.begin();
          it != m_PeriodPorts.end();
          ++it )
    {

        if((*it)->isDisabled()) {continue;};

		//FIXME: make this into a static_cast when not DEBUG?

		MotuPortInfo *pinfo=dynamic_cast<MotuPortInfo *>(*it);
		assert(pinfo); // this should not fail!!
		
/* AMDTP, left as reference
		switch(pinfo->getFormat()) {
		
		case MotuPortInfo::E_MBLA:
			if(decodeMBLAEventsToPort(static_cast<MotuAudioPort *>(*it), (quadlet_t *)data, offset, nevents)) {
				debugWarning("Could not decode packet MBLA to port %s",(*it)->getName().c_str());
				problem=1;
			}
			break;
		case MotuPortInfo::E_SPDIF: // still unimplemented
			break;
	// midi is a packet based port, don't process
	//	case MotuPortInfo::E_Midi:
	//		break;

		default: // ignore
			break;
		}
*/
    }
	return problem;

}

/**
 * @brief decode a packet for the packet-based ports
 *
 * @param data Packet data
 * @param nevents number of events in data (including events of other ports & port types)
 * @param dbc DataBlockCount value for this packet
 * @return true if all successfull
 */
bool MotuReceiveStreamProcessor::decodePacketPorts(quadlet_t *data, unsigned int nevents, unsigned int dbc)
{
	bool ok=true;
	
	quadlet_t *target_event=NULL;
	int j;
	
	for ( PortVectorIterator it = m_PacketPorts.begin();
          it != m_PacketPorts.end();
          ++it )
	{

#ifdef DEBUG
		MotuPortInfo *pinfo=dynamic_cast<MotuPortInfo *>(*it);
		assert(pinfo); // this should not fail!!

		// the only packet type of events for AMDTP is MIDI in mbla
// 		assert(pinfo->getFormat()==MotuPortInfo::E_Midi);
#endif
		MotuMidiPort *mp=static_cast<MotuMidiPort *>(*it);
		

        // do decoding here

	}
    	
	return ok;
}

/* for reference

int MotuReceiveStreamProcessor::decodeMBLAEventsToPort(MotuAudioPort *p, quadlet_t *data, 
					   unsigned int offset, unsigned int nevents)
{
	unsigned int j=0;

// 	printf("****************\n");
// 	hexDumpQuadlets(data,m_dimension*4);
// 	printf("****************\n");

	quadlet_t *target_event;

	target_event=(quadlet_t *)(data + p->getPosition());

	switch(p->getDataType()) {
		default:
		case Port::E_Int24:
			{
				quadlet_t *buffer=(quadlet_t *)(p->getBufferAddress());

				assert(nevents + offset <= p->getBufferSize());

				buffer+=offset;

				for(j = 0; j < nevents; j += 1) { // decode max nsamples
					*(buffer)=(ntohl((*target_event) ) & 0x00FFFFFF);
					buffer++;
					target_event+=m_dimension;
				}
			}
			break;
		case Port::E_Float:
			{
				const float multiplier = 1.0f / (float)(0x7FFFFF);
				float *buffer=(float *)(p->getBufferAddress());

				assert(nevents + offset <= p->getBufferSize());

				buffer+=offset;

				for(j = 0; j < nevents; j += 1) { // decode max nsamples		
	
					unsigned int v = ntohl(*target_event) & 0x00FFFFFF;
					// sign-extend highest bit of 24-bit int
					int tmp = (int)(v << 8) / 256;
		
					*buffer = tmp * multiplier;
				
					buffer++;
					target_event+=m_dimension;
				}
			}
			break;
	}

	return 0;
}
*/
} // end of namespace FreebobStreaming
