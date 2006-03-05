/* avdevicesubunit.cpp
 * Copyright (C) 2005 by Daniel Wagner
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

#include "functionblock.h"
#include "avdevicesubunit.h"
#include "avdevice.h"
#include "avplug.h"

#include "libfreebobavc/avc_plug_info.h"
#include "libfreebobavc/avc_extended_stream_format.h"
#include "libfreebobavc/serialize.h"

IMPL_DEBUG_MODULE( AvDeviceSubunit, AvDeviceSubunit, DEBUG_LEVEL_VERBOSE );

////////////////////////////////////////////

AvDeviceSubunit::AvDeviceSubunit( AvDevice& avDevice,
                                  AVCCommand::ESubunitType type,
                                  subunit_t id,
                                  bool verbose)
    : m_avDevice( &avDevice )
    , m_sbType( type )
    , m_sbId( id )
    , m_verbose( verbose )
{
    if ( m_verbose ) {
        setDebugLevel( DEBUG_LEVEL_VERBOSE );
    }
}

AvDeviceSubunit::~AvDeviceSubunit()
{
    for ( AvPlugVector::iterator it = m_plugs.begin();
          it != m_plugs.end();
          ++it )
    {
        delete *it;
    }
}

bool
AvDeviceSubunit::discover()
{
    if ( !discoverPlugs() ) {
        debugError( "plug discovering failed\n" );
        return false;
    }

    return true;
}

bool
AvDeviceSubunit::discoverPlugs()
{
    PlugInfoCmd plugInfoCmd( m_avDevice->get1394Service(),
                             PlugInfoCmd::eSF_SerialBusIsochronousAndExternalPlug );
    plugInfoCmd.setNodeId( m_avDevice->getNodeId() );
    plugInfoCmd.setCommandType( AVCCommand::eCT_Status );
    plugInfoCmd.setSubunitType( m_sbType );
    plugInfoCmd.setSubunitId( m_sbId );

    if ( !plugInfoCmd.fire() ) {
        debugError( "plug info command failed\n" );
        return false;
    }

    debugOutput( DEBUG_LEVEL_NORMAL, "\n"
                 "\tnumber of source plugs = %d\n "
                 "\tnumber of destination output plugs = %d\n",
                 plugInfoCmd.m_sourcePlugs,
                 plugInfoCmd.m_destinationPlugs );

    if ( !discoverPlugs(  AvPlug::eAPD_Input,
                          plugInfoCmd.m_destinationPlugs ) )
    {
        debugError( "destination plug discovering failed\n" );
        return false;
    }

    if ( !discoverPlugs(  AvPlug::eAPD_Output,
                          plugInfoCmd.m_sourcePlugs ) )
    {
        debugError( "source plug discovering failed\n" );
        return false;
    }

    return true;
}

bool
AvDeviceSubunit::discoverConnections()
{
    // XXX does not work because function blocks are missing.
    /*
    for ( AvPlugVector::iterator it = m_plugs.begin();
          it != m_plugs.end();
          ++it )
    {
        AvPlug* plug = *it;
        if ( plug->discoverConnections() ) {
            debugError( "plug connection discovering failed ('%s')\n",
                        plug->getName() );
            return false;
        }
    }
    */
    return true;
}

bool
AvDeviceSubunit::discoverPlugs(AvPlug::EAvPlugDirection plugDirection,
                               plug_id_t plugMaxId )
{
    for ( int plugIdx = 0;
          plugIdx < plugMaxId;
          ++plugIdx )
    {
        AVCCommand::ESubunitType subunitType =
            static_cast<AVCCommand::ESubunitType>( getSubunitType() );
        AvPlug* plug = new AvPlug( *m_avDevice->get1394Service(),
                                   m_avDevice->getNodeId(),
                                   m_avDevice->getPlugManager(),
                                   subunitType,
                                   getSubunitId(),
                                   AvPlug::eAPA_SubunitPlug,
                                   plugDirection,
                                   plugIdx,
                                   m_verbose );
        if ( !plug || !plug->discover() ) {
            debugError( "plug discover failed\n" );
            return false;
        }

        debugOutput( DEBUG_LEVEL_NORMAL, "plug '%s' found\n",
                     plug->getName() );
        m_plugs.push_back( plug );
    }
    return true;
}

bool
AvDeviceSubunit::addPlug( AvPlug& plug )
{
    m_plugs.push_back( &plug );
    return true;
}


AvPlug*
AvDeviceSubunit::getPlug(AvPlug::EAvPlugDirection direction, plug_id_t plugId)
{
    for ( AvPlugVector::iterator it = m_plugs.begin();
          it != m_plugs.end();
          ++it )
    {
        AvPlug* plug = *it;
        if ( ( plug->getPlugId() == plugId )
            && ( plug->getDirection() == direction ) )
        {
            return plug;
        }
    }
    return 0;
}

////////////////////////////////////////////

AvDeviceSubunitAudio::AvDeviceSubunitAudio( AvDevice& avDevice,
                                            subunit_t id,
                                            bool verbose )
    : AvDeviceSubunit( avDevice, AVCCommand::eST_Audio, id, verbose )
{
}

AvDeviceSubunitAudio::~AvDeviceSubunitAudio()
{
    for ( FunctionBlockVector::iterator it = m_functions.begin();
          it != m_functions.end();
          ++it )
    {
        delete *it;
    }
}

bool
AvDeviceSubunitAudio::discover()
{
    if ( !AvDeviceSubunit::discover() ) {
        return false;
    }

    if ( !discoverFunctionBlocks() ) {
        debugError( "function block discovering failed\n" );
        return false;
    }

    return true;
}

bool
AvDeviceSubunitAudio::discoverConnections()
{
    if ( !AvDeviceSubunit::discoverConnections() ) {
        return false;
    }

    for ( FunctionBlockVector::iterator it = m_functions.begin();
          it != m_functions.end();
          ++it )
    {
        FunctionBlock* function = *it;
        if ( function->discoverConnections() ) {
            debugError( "functionblock connection discovering failed ('%s')\n",
                        function->getName() );
            return false;
        }
    }

    return true;
}

const char*
AvDeviceSubunitAudio::getName()
{
    return "AudioSubunit";
}

bool
AvDeviceSubunitAudio::discoverFunctionBlocks()
{
    // printf( "XXX discover functions\n" );
    return true;
}


////////////////////////////////////////////

AvDeviceSubunitMusic::AvDeviceSubunitMusic( AvDevice& avDevice,
                                            subunit_t id,
                                            bool verbose )
    : AvDeviceSubunit( avDevice, AVCCommand::eST_Music, id, verbose )
{
}

AvDeviceSubunitMusic::~AvDeviceSubunitMusic()
{
}

const char*
AvDeviceSubunitMusic::getName()
{
    return "MusicSubunit";
}
