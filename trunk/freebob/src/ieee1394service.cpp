/* ieee1394service.cpp
 * Copyright (C) 2004 by Daniel Wagner
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

#include <errno.h>
#include <libavc1394/avc1394.h>
#include <libavc1394/avc1394_vcr.h>
#include <libiec61883/iec61883.h>

#include "ieee1394service.h"
#include "threads.h"
#include "avdevicepool.h"

#include "avdevice.h"
#include "avdescriptor.h"
#include "avmusicidentifierdescriptor.h"
#include "avmusicstatusdescriptor.h"
#include "avinfoblock.h"
#include "avgeneralmusicstatusinfoblock.h"
#include "avnameinfoblock.h"
#include "avaudioinfoblock.h"
#include "avmidiinfoblock.h"
#include "avaudiosyncinfoblock.h"
#include "avsourcepluginfoblock.h"
#include "avoutputplugstatusinfoblock.h"

USE_GLOBAL_DEBUG_MODULE;

Ieee1394Service* Ieee1394Service::m_pInstance = 0;

Ieee1394Service::Ieee1394Service()
    : m_iPort( 0 )
    , m_bInitialised( false )
    , m_bRHThreadRunning( false )
    , m_iGenerationCount( 0 )
{
    setDebugLevel( DEBUG_LEVEL_ALL );
    pthread_mutex_init( &m_mutex, NULL );
}

Ieee1394Service::~Ieee1394Service()
{
    stopRHThread();

    if ( m_rhHandle ) {
        raw1394_destroy_handle( m_rhHandle );
        m_rhHandle = 0;
    }

    if ( m_handle ) {
        raw1394_destroy_handle( m_handle );
        m_handle = 0;
    }

    m_pInstance = 0;
}

FBReturnCodes
Ieee1394Service::initialize()
{
    if ( !m_bInitialised ) {
        m_rhHandle = raw1394_new_handle();
        m_handle = raw1394_new_handle();
        if ( !m_rhHandle || !m_handle ) {
            if ( !errno ) {
                fprintf( stderr,  "libraw1394 not compatible.\n" );
            } else {
                perror ("Could not get 1394 handle");
                fprintf (stderr, "Is ieee1394 and raw1394 driver loaded?\n");
            }
            return eFBRC_Creating1394HandleFailed;
        }

        // Store this instance in the user data pointer, in order
        // to be able to retrieve the instance in the pure C bus reset
        // call back function.
        raw1394_set_userdata( m_rhHandle,  this );

        if ( raw1394_set_port( m_rhHandle,  m_iPort ) < 0 ) {
            perror( "Could not set port" );
            return eFBRC_Setting1394PortFailed;
        }

        if ( raw1394_set_port( m_handle,  m_iPort ) < 0 ) {
            perror( "Could not set port" );
            return eFBRC_Setting1394PortFailed;
        }

        raw1394_set_bus_reset_handler( m_rhHandle,  this->resetHandler );

        startRHThread();

        m_bInitialised = true;

        asyncCall( this, &Ieee1394Service::discoveryDevices,
                   m_iGenerationCount);
    }
    return eFBRC_Success;
}

void
Ieee1394Service::shutdown()
{
    delete this;
}

Ieee1394Service*
Ieee1394Service::instance()
{
    if ( !m_pInstance ) {
        m_pInstance = new Ieee1394Service;
    }
    return m_pInstance;
}

FBReturnCodes
Ieee1394Service::discoveryDevices( unsigned int iGeneration )
{
    //scan bus
    int iNodeCount = raw1394_get_nodecount( m_handle );
    for ( int iNodeId = 0; iNodeId < iNodeCount; ++iNodeId ) {
        rom1394_directory romDir;
        rom1394_get_directory( m_handle, iNodeId, &romDir );
        if ( pMainArguments->verbose ) {
            printRomDirectory( iNodeId, &romDir );
        }

        switch (rom1394_get_node_type( &romDir )) {
	case ROM1394_NODE_TYPE_UNKNOWN:
            debugPrint( DEBUG_LEVEL_INFO,
                        "Node %d has node type UNKNOWN\n", iNodeId );
            break;
	case ROM1394_NODE_TYPE_DC:
            debugPrint( DEBUG_LEVEL_INFO,
                        "Node %d has node type DC\n", iNodeId );
            break;
	case ROM1394_NODE_TYPE_AVC:
            debugPrint( DEBUG_LEVEL_INFO,
                        "Node %d has node type AVC\n", iNodeId );

            if ( pMainArguments->verbose ) {
                printAvcUnitInfo( iNodeId );
            }

            if ( avc1394_check_subunit_type( m_handle, iNodeId,
                                             AVC1394_SUBUNIT_TYPE_AUDIO ) ) {
                octlet_t oGuid = rom1394_get_guid( m_handle, iNodeId );

                AvDevice* pAvDevice = new AvDevice( oGuid );
                if ( !pAvDevice ) {
                    debugError( "Could not create AvDevice instance for "
                                "device with GUID 0x%08x%08x\n",
                                (quadlet_t) (oGuid>>32),
                                (quadlet_t) (oGuid & 0xffffffff) );

                    return eFBRC_CreatingAvDeviceFailed;
                }
                pAvDevice->setNodeId( iNodeId );
                pAvDevice->setPort( m_iPort );

                asyncCall( pAvDevice, &AvDevice::execute,
                           AvDevice::eDeviceDiscovery );

                // XXX Pieter's test code.
                // avDeviceTests( oGuid, m_iPort, iNodeId );
	    }
            break;
	case ROM1394_NODE_TYPE_SBP2:
            debugPrint( DEBUG_LEVEL_INFO,
                        "Node %d has node type SBP2\n", iNodeId);
            break;
	case ROM1394_NODE_TYPE_CPU:
            debugPrint( DEBUG_LEVEL_INFO,
                        "Node %d has node type CPU\n", iNodeId);
            break;
	default:
            debugPrint( DEBUG_LEVEL_INFO,
                        "No matching node type found for node %d\n", iNodeId);
	}
    }
    // XXX dw: removed: AvDevicePool::instance()->removeObsoleteDevices()
    // check if this is still code here is still correct.
    return eFBRC_Success;
}


void
Ieee1394Service::avDeviceTests(octlet_t oGuid, int iPort, int iNodeId)
{
    // PP: just a static try, don't want to mess with the device manager yet...
    // Remark: the AvDevice and AvDescriptor aren't debugged thouroughly yet!
    //         the following code is the only debug I had time for... to be continued! (later this week)
            	debugPrint (DEBUG_LEVEL_INFO, "  Trying to create an AvDevice...\n");

                AvDevice *test=new AvDevice(oGuid);
                test->setNodeId( iNodeId );
                test->setPort( iPort );
      		debugPrint (DEBUG_LEVEL_INFO, "   Created...\n");
		test->initialize();
		if (test->isInitialised()) {
			unsigned char fmt;
			quadlet_t fdf;
			test->getInputPlugSignalFormat(0,&fmt,&fdf);
      			debugPrint (DEBUG_LEVEL_INFO, "   fmt=%02X fdf=%08X\n",fmt,fdf);
			test->getInputPlugSignalFormat(1,&fmt,&fdf);
      			debugPrint (DEBUG_LEVEL_INFO, "   fmt=%02X fdf=%08X\n",fmt,fdf);
			test->getOutputPlugSignalFormat(0,&fmt,&fdf);
      			debugPrint (DEBUG_LEVEL_INFO, "   fmt=%02X fdf=%08X\n",fmt,fdf);
			test->getOutputPlugSignalFormat(1,&fmt,&fdf);
      			debugPrint (DEBUG_LEVEL_INFO, "   fmt=%02X fdf=%08X\n",fmt,fdf);
			test->printConnections();
		}

		debugPrint (DEBUG_LEVEL_INFO, "   Deleting AvDevice...\n");
		delete test;
}


void
Ieee1394Service::printAvcUnitInfo( int iNodeId )
{
    printf( "AVC: video monitor?......%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_VIDEO_MONITOR ) ?
            "yes":"no" );
    printf( "AVC: audio?..............%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_AUDIO ) ?
            "yes":"no" );
    printf( "AVC; printer?............%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_PRINTER ) ?
            "yes":"no" );
    printf( "AVC: disk recorder?......%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_DISC_RECORDER ) ?
            "yes":"no" );
    printf( "AVC: video recorder?.....%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_TAPE_RECORDER ) ?
            "yes":"no" );
    printf( "AVC: vcr?................%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_VCR ) ?
            "yes":"no" );
    printf( "AVC: tuner?..............%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_TUNER ) ?
            "yes":"no" );
    printf( "AVC: CA?.................%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_CA ) ?
            "yes":"no" );
    printf( "AVC: video camera?.......%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_VIDEO_CAMERA ) ?
            "yes":"no" );
    printf( "AVC: panel?..............%s\n",
            avc1394_check_subunit_type(m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_PANEL ) ?
            "yes":"no" );
    printf( "AVC: camera storage?.....%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_CAMERA_STORAGE ) ?
            "yes":"no" );
    printf( "AVC: bulletin board?.....%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_BULLETIN_BOARD ) ?
            "yes":"no" );
    printf( "AVC: vendor specific?....%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_VENDOR_UNIQUE ) ?
            "yes":"no" );
    printf( "AVC: extended?...........%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_EXTENDED ) ?
            "yes":"no" );
    printf( "AVC: unit?...............%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_UNIT ) ?
            "yes":"no" );
    printf( "AVC: music?..............%s\n",
            avc1394_check_subunit_type( m_handle, iNodeId,
                                        AVC1394_SUBUNIT_TYPE_MUSIC ) ?
            "yes":"no" );
}

void
Ieee1394Service::printRomDirectory( int iNodeId,  rom1394_directory* pRomDir )
{
    int iBusInfoBlockLength
        = rom1394_get_bus_info_block_length( m_handle,  iNodeId );
    int iBusId = rom1394_get_bus_id( m_handle,  iNodeId );
    octlet_t oGuid = rom1394_get_guid( m_handle, iNodeId );
    rom1394_bus_options busOptions;
    rom1394_get_bus_options( m_handle, iNodeId, &busOptions );

    printf( "\nNode %d: \n", iNodeId );
    printf( "-------------------------------------------------\n" );
    printf( "bus info block length = %d\n", iBusInfoBlockLength);
    printf( "bus id = 0x%08x\n", iBusId );
    printf( "bus options:\n" );
    printf( "  isochronous resource manager capable: %d\n", busOptions.irmc );
    printf ("  cycle master capable                : %d\n", busOptions.cmc );
    printf ("  isochronous capable                 : %d\n", busOptions.isc );
    printf ("  bus manager capable                 : %d\n", busOptions.bmc );
    printf ("  cycle master clock accuracy         : %d ppm\n", busOptions.cyc_clk_acc );
    printf( "  maximum asynchronous record size    : %d bytes\n", busOptions.max_rec );
    printf("GUID: 0x%08x%08x\n", (quadlet_t) (oGuid>>32),
           (quadlet_t) (oGuid & 0xffffffff) );
    printf( "directory:\n");
    printf( "  node capabilities    : 0x%08x\n", pRomDir->node_capabilities );
    printf( "  vendor id            : 0x%08x\n", pRomDir->vendor_id );
    printf( "  unit spec id         : 0x%08x\n", pRomDir->unit_spec_id );
    printf( "  unit software version: 0x%08x\n", pRomDir->unit_sw_version );
    printf( "  model id             : 0x%08x\n", pRomDir->model_id );
    printf( "  textual leaves       : %s\n",     pRomDir->label );
}

int
Ieee1394Service::resetHandler( raw1394handle_t handle,
                               unsigned int iGeneration )
{
    debugGlobalPrint( DEBUG_LEVEL_SCHEDULER,
                      "Bus reset occured: generation count = %d\n",
                      iGeneration );

    raw1394_update_generation (handle, iGeneration);
    Ieee1394Service* pInstance
        = (Ieee1394Service*) raw1394_get_userdata (handle);
    pInstance->setGenerationCount( iGeneration );

    WorkerThread::instance()->wakeSleepers();
    asyncCall( pInstance, &Ieee1394Service::discoveryDevices, iGeneration );
    return 0;
}

bool
Ieee1394Service::startRHThread()
{
    if ( m_bRHThreadRunning ) {
        return true;
    }
    debugPrint( DEBUG_LEVEL_INFO,
                "Starting bus reset handler thread.\n" );
    pthread_mutex_lock( &m_mutex );
    pthread_create( &m_thread, NULL, rHThread, this );
    pthread_mutex_unlock( &m_mutex );
    m_bRHThreadRunning = true;
    return true;
}

void
Ieee1394Service::stopRHThread()
{
    if ( m_bRHThreadRunning ) {
        debugPrint( DEBUG_LEVEL_INFO,
                    "Stopping bus reset handler thread.\n" );
        pthread_mutex_lock (&m_mutex);
        pthread_cancel (m_thread);
        pthread_join (m_thread, NULL);
        pthread_mutex_unlock (&m_mutex);
    }
    m_bRHThreadRunning = false;
}

void*
Ieee1394Service::rHThread( void* arg )
{
    Ieee1394Service* pIeee1394Service = (Ieee1394Service*) arg;

    while (true) {
        raw1394_loop_iterate (pIeee1394Service->m_rhHandle);
        pthread_testcancel ();
    }

    return NULL;
}

void
Ieee1394Service::setGenerationCount( unsigned int iGeneration )
{
    m_iGenerationCount = iGeneration;
}

unsigned int
Ieee1394Service::getGenerationCount()
{
    return m_iGenerationCount;
}

/* Function to execute an AVC transaction, i.e. send command/status
 * and get response main purpose is wrapping the avc1394 function call
 * to output some debugging comments.
 */
quadlet_t*
Ieee1394Service::avcExecuteTransaction( int node_id,
                                        quadlet_t* request,
                                        unsigned int request_len,
                                        unsigned int response_len )
{
    quadlet_t* response;
    unsigned char* request_pos;
    unsigned int i;

    response = avc1394_transaction_block( m_handle,
					  node_id,
					  request,
					  request_len,
					  2 );
    if ( request ) {
	debugPrint( DEBUG_LEVEL_TRANSFERS,
		    "------- TRANSACTION START -------\n" );
	debugPrint( DEBUG_LEVEL_TRANSFERS,"  REQUEST:     " );
	/* request is in machine byte order. this function is for
	 * intel architecure */
	for ( i = 0; i < request_len; i++ ) {
	    request_pos = (unsigned char *)(request+i);
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS,
			     "0x%02X%02X%02X%02X ",
			     *(request_pos),
			     *(request_pos+1),
			     *(request_pos+2),
			     *(request_pos+3));
	}
	debugPrintShort( DEBUG_LEVEL_TRANSFERS, "\n" );
	debugPrint( DEBUG_LEVEL_TRANSFERS, "      => " );
	debugPrintShort( DEBUG_LEVEL_TRANSFERS, "                     " );

	request_pos = (unsigned char *)(request);

	debugPrintShort( DEBUG_LEVEL_TRANSFERS,
			 "subunit_type=%02X  subunit_id=%02X  opcode=%02X",
			 ((*(request_pos+1))>>3)&0x1F,
			 (*(request_pos+1))&0x07,
			 (*(request_pos+2))&0xFF );
	debugPrintShort (DEBUG_LEVEL_TRANSFERS,"\n");
    }

    if ( response ) {
	/* response is in order of receiving, i.e. msb first */
	debugPrint(DEBUG_LEVEL_TRANSFERS, "  -> RESPONSE: " );
	for ( i = 0; i < response_len; i++ ) {
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "0x%08X ", response[i] );
	}

	debugPrintShort( DEBUG_LEVEL_TRANSFERS,"\n" );
	debugPrint( DEBUG_LEVEL_TRANSFERS,"      => " );
	switch (response[0]&0xFF000000) {
	case AVC1394_RESPONSE_NOT_IMPLEMENTED:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "Not Implemented      " );
	    break;
	case AVC1394_RESPONSE_ACCEPTED:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "Accepted             " );
	    break;
	case AVC1394_RESPONSE_REJECTED:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "Rejected             " );
	    break;
	case AVC1394_RESPONSE_IN_TRANSITION:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "In Transition        " );
	    break;
	case AVC1394_RESPONSE_IMPLEMENTED:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "Implemented / Stable " );
	    break;
	case AVC1394_RESPONSE_CHANGED:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "Changed              " );
	    break;
	case AVC1394_RESPONSE_INTERIM:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "Interim              " );
	    break;
	default:
	    debugPrintShort( DEBUG_LEVEL_TRANSFERS, "Unknown response     " );
	    break;
	}
	debugPrintShort( DEBUG_LEVEL_TRANSFERS,
			 "subunit_type=%02X  subunit_id=%02X  opcode=%02X",
			 (response[0]>>19)&0x1F,
			 (response[0]>>16)&0x07,
			 (response[0]>>8)&0xFF );
	debugPrintShort( DEBUG_LEVEL_TRANSFERS, "\n" );
    }
    debugPrint( DEBUG_LEVEL_TRANSFERS, "------- TRANSACTION END -------\n" );
    return response;
}
