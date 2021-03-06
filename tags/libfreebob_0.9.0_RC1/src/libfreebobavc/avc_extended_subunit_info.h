/* avc_extended_subunit_info.h
 * Copyright (C) 2006 by Daniel Wagner
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

#ifndef AVCEXTENDEDSUBUNITINFO_H
#define AVCEXTENDEDSUBUNITINFO_H

#include "avc_generic.h"

#include <libavc1394/avc1394.h>

#include <string>
#include <vector>

class ExtendedSubunitInfoPageData: public IBusData
{
 public:
    enum ESpecialPurpose {
        eSP_InputGain       = 0x00,
        eSP_OutputVolume    = 0x01,
        eSP_NoSpecialPupose = 0xff,
    };

    ExtendedSubunitInfoPageData();
    virtual ~ExtendedSubunitInfoPageData();

    virtual bool serialize( IOSSerialize& se );
    virtual bool deserialize( IISDeserialize& de );
    virtual ExtendedSubunitInfoPageData* clone() const;

    function_block_type_t            m_functionBlockType;
    function_block_id_t              m_functionBlockId;
    function_block_special_purpose_t m_functionBlockSpecialPupose;
    no_of_input_plugs_t              m_noOfInputPlugs;
    no_of_output_plugs_t             m_noOfOutputPlugs;
};

typedef std::vector<ExtendedSubunitInfoPageData*> ExtendedSubunitInfoPageDataVector;

class ExtendedSubunitInfoCmd: public AVCCommand
{
public:
    enum EFunctionBlockType {
        eFBT_AllFunctinBlockType    = 0xff,
        eFBT_AudioSubunitSelector   = 0x80,
        eFBT_AudioSubunitFeature    = 0x81,
        eFBT_AudioSubunitProcessing = 0x82,
        eFBT_AudioSubunitCodec      = 0x83,
    };

    enum EProcessingType {
        ePT_Mixer                   = 0x01,
        ePT_Generic                 = 0x02,
        ePT_UpDown                  = 0x03,
        ePT_DolbyProLogic           = 0x04,
        ePT_3DStereoExtender        = 0x05,
        ePT_Reverberation           = 0x06,
        ePT_Chorus                  = 0x07,
        ePT_DynamicRangeCompression = 0x08,
        ePT_EnhancedMixer           = 0x82,
        ePT_Reserved                = 0xff,
    };

    enum ECodecType {
        eCT_AC3Decoder  = 0x01,
        eCT_MPEGDecoder = 0x02,
        eCT_DTSDecoder  = 0x03,
        eCT_Reserved    = 0xff,

    };

    ExtendedSubunitInfoCmd( Ieee1394Service* ieee1394service );
    ExtendedSubunitInfoCmd( const ExtendedSubunitInfoCmd& rhs );
    virtual ~ExtendedSubunitInfoCmd();

    virtual bool serialize( IOSSerialize& se );
    virtual bool deserialize( IISDeserialize& de );

    virtual const char* getCmdName() const
	{ return "ExtendedSubunitInfoCmd"; }

    page_t                m_page;
    function_block_type_t m_fbType;
    ExtendedSubunitInfoPageDataVector m_infoPageDatas;
};

#endif
