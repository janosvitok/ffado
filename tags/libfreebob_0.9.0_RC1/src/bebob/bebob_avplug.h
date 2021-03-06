/* bebob_avplug.h
 * Copyright (C) 2005,06 by Daniel Wagner
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

#ifndef BEBOB_AVPLUG_H
#define BEBOB_AVPLUG_H

#include "libfreebobavc/avc_signal_source.h"
#include "libfreebobavc/avc_extended_stream_format.h"
#include "libfreebobavc/avc_extended_plug_info.h"
#include "libfreebobavc/avc_extended_cmd_generic.h"
#include "libfreebobavc/avc_definitions.h"
#include "libfreebobavc/avc_generic.h"
#include "libfreebob/xmlparser.h"

#include "debugmodule/debugmodule.h"

class Ieee1394Service;

namespace BeBoB {

class AvPlugManager;
class AvPlug;

typedef std::vector<AvPlug*> AvPlugVector;

class AvPlug {
public:

    enum EAvPlugAddressType {
	eAPA_PCR,
	eAPA_ExternalPlug,
	eAPA_AsynchronousPlug,
	eAPA_SubunitPlug,
	eAPA_FunctionBlockPlug,
	eAPA_Undefined,
    };

    enum EAvPlugType {
        eAPT_IsoStream,
        eAPT_AsyncStream,
        eAPT_Midi,
        eAPT_Sync,
        eAPT_Analog,
        eAPT_Digital,
        eAPT_Unknown,
    };

    enum EAvPlugDirection {
	eAPD_Input,
	eAPD_Output,
	eAPD_Unknown,
    };

    // \todo This constructors sucks. too many parameters. fix it.
    AvPlug( Ieee1394Service& ieee1394Service,
	    int m_nodeId,
            AvPlugManager& plugManager,
	    AVCCommand::ESubunitType subunitType,
	    subunit_id_t subunitId,
	    function_block_type_t functionBlockType,
	    function_block_type_t functionBlockId,
	    EAvPlugAddressType plugAddressType,
	    EAvPlugDirection plugDirection,
	    plug_id_t plugId,
	    int verboseLevel );
    AvPlug( const AvPlug& rhs );
    virtual ~AvPlug();

    bool discover();
    bool discoverConnections();

    bool inquireConnnection( AvPlug& plug );

    int getGlobalId() const
	{ return m_globalId; }
    plug_id_t getPlugId() const
	{ return m_id; }
    AVCCommand::ESubunitType getSubunitType() const
	{ return m_subunitType; }
    subunit_id_t getSubunitId() const
	{ return m_subunitId; }
    const char* getName() const
	{ return m_name.c_str(); }
    EAvPlugDirection getPlugDirection() const
	{ return m_direction; }
    sampling_frequency_t getSamplingFrequency() const
	{ return m_samplingFrequency; }
    int getSampleRate() const; // 22050, 24000, 32000, ...
    int getNrOfChannels() const;
    int getNrOfStreams() const;

    EAvPlugDirection getDirection() const
        { return m_direction; }
    EAvPlugAddressType getPlugAddressType() const
        { return m_addressType; }
    EAvPlugType getPlugType() const
	{ return m_infoPlugType; }

    function_block_type_t getFunctionBlockType() const
	{ return m_functionBlockType; }
    function_block_id_t getFunctionBlockId() const
        { return m_functionBlockId; }

    const AvPlugVector& getInputConnections() const
        { return m_inputConnections; }
    const AvPlugVector& getOutputConnections() const
        { return m_outputConnections; }

    bool addXmlDescription( xmlNodePtr conectionSet );
    bool addXmlDescriptionStreamFormats( xmlNodePtr streamFormats );

    static PlugAddress::EPlugDirection convertPlugDirection(
	EAvPlugDirection direction);

    void showPlug() const;

protected:
    bool discoverPlugType();
    bool discoverName();
    bool discoverNoOfChannels();
    bool discoverChannelPosition();
    bool discoverChannelName();
    bool discoverClusterInfo();
    bool discoverStreamFormat();
    bool discoverSupportedStreamFormats();
    bool discoverConnectionsInput();
    bool discoverConnectionsOutput();

    ExtendedPlugInfoCmd setPlugAddrToPlugInfoCmd();

    ExtendedStreamFormatCmd setPlugAddrToStreamFormatCmd(
	ExtendedStreamFormatCmd::ESubFunction subFunction);

    SignalSourceCmd setSrcPlugAddrToSignalCmd();

    void setDestPlugAddrToSignalCmd(
	SignalSourceCmd& signalSourceCmd, AvPlug& plug );

    void debugOutputClusterInfos( int debugLevel );

    bool copyClusterInfo(ExtendedPlugInfoPlugChannelPositionSpecificData&
                         channelPositionData );

    bool addPlugConnection( AvPlugVector& connections, AvPlug& plug );

    bool discoverConnectionsFromSpecificData(
        EAvPlugDirection discoverDirection,
        PlugAddressSpecificData* plugAddress,
        AvPlugVector& connections );

    AvPlug* getPlugDefinedBySpecificData(
        UnitPlugSpecificDataPlugAddress* pUnitPlugAddress,
        SubunitPlugSpecificDataPlugAddress* pSubunitPlugAddress,
        FunctionBlockPlugSpecificDataPlugAddress* pFunctionBlockPlugAddress );

    EAvPlugDirection toggleDirection( EAvPlugDirection direction ) const;

private:
    Ieee1394Service*             m_1394Service;
    int                          m_nodeId;
    AVCCommand::ESubunitType     m_subunitType;
    subunit_id_t                 m_subunitId;
    function_block_type_t        m_functionBlockType;
    function_block_id_t          m_functionBlockId;
    EAvPlugAddressType           m_addressType;
    EAvPlugDirection             m_direction;
    plug_id_t                    m_id;

    // Info plug type
    EAvPlugType m_infoPlugType;

    // Number of channels
    nr_of_channels_t             m_nrOfChannels;

    // Plug name
    std::string                  m_name;

    // Channel & Cluster Info
    struct ChannelInfo {
        stream_position_t          m_streamPosition;
        stream_position_location_t m_location;
	std::string                m_name;
    };
    typedef std::vector<ChannelInfo> ChannelInfoVector;

    struct ClusterInfo {
	int                      m_index;
	port_type_t              m_portType;
	std::string              m_name;

        nr_of_channels_t         m_nrOfChannels;
        ChannelInfoVector        m_channelInfos;
	stream_format_t          m_streamFormat;
    };
    typedef std::vector<ClusterInfo> ClusterInfoVector;

    ClusterInfoVector        m_clusterInfos;

    // Sampling frequency
    sampling_frequency_t m_samplingFrequency;

    // Supported stream formats
    struct FormatInfo {
        FormatInfo()
            : m_samplingFrequency( eSF_DontCare )
            , m_isSyncStream( false )
            , m_audioChannels( 0 )
            , m_midiChannels( 0 )
            , m_index( 0xff )
            {}
	sampling_frequency_t  m_samplingFrequency;
	bool                  m_isSyncStream;
	number_of_channels_t  m_audioChannels;
	number_of_channels_t  m_midiChannels;
	byte_t                m_index;
    };
    typedef std::vector<FormatInfo> FormatInfoVector;

    FormatInfoVector         m_formatInfos;

    const ClusterInfo* getClusterInfoByIndex( int index ) const;

    AvPlugVector             m_inputConnections;
    AvPlugVector             m_outputConnections;

    AvPlugManager*           m_plugManager;

    int                      m_verboseLevel;

    int                      m_globalId;

    static int               m_globalIdCounter;

    DECLARE_DEBUG_MODULE;
};

const char* avPlugAddressTypeToString( AvPlug::EAvPlugAddressType addressType );
const char* avPlugTypeToString( AvPlug::EAvPlugType type );
const char* avPlugDirectionToString( AvPlug::EAvPlugDirection direction );

class AvPlugManager
{
public:
    AvPlugManager( int verboseLevel );
    AvPlugManager( const AvPlugManager& rhs );
    ~AvPlugManager();

    bool addPlug( AvPlug& plug );
    bool remPlug( AvPlug& plug );

    void showPlugs() const;

    AvPlug* getPlug( AVCCommand::ESubunitType subunitType,
                     subunit_id_t subunitId,
		     function_block_type_t functionBlockType,
		     function_block_id_t functionBlockId,
                     AvPlug::EAvPlugAddressType plugAddressType,
                     AvPlug::EAvPlugDirection plugDirection,
                     plug_id_t plugId ) const;
    AvPlugVector getPlugsByType( AVCCommand::ESubunitType subunitType,
				 subunit_id_t subunitId,
				 function_block_type_t functionBlockType,
				 function_block_id_t functionBlockId,
				 AvPlug::EAvPlugAddressType plugAddressType,
				 AvPlug::EAvPlugDirection plugDirection,
				 AvPlug::EAvPlugType type) const;

private:
    int          m_verboseLevel;
    AvPlugVector m_plugs;

    DECLARE_DEBUG_MODULE;
};

class AvPlugCluster {
public:
    AvPlugCluster();
    virtual ~AvPlugCluster();

    std::string  m_name;

    AvPlugVector m_avPlugs;
};

class AvPlugConnection {
public:
    AvPlugConnection( AvPlug& srcPlug, AvPlug& destPlug );

    AvPlug& getSrcPlug() const
        { return *m_srcPlug; }
    AvPlug& getDestPlug() const
        { return *m_destPlug; }

private:
    AvPlug* m_srcPlug;
    AvPlug* m_destPlug;
};

typedef std::vector<AvPlugConnection*> AvPlugConnectionVector;
typedef std::vector<AvPlugConnection> AvPlugConnectionOwnerVector;

}

#endif
