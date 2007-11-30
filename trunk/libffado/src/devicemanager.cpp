/*
 * Copyright (C) 2005-2007 by Daniel Wagner
 * Copyright (C) 2005-2007 by Pieter Palmers
 *
 * This file is part of FFADO
 * FFADO = Free Firewire (pro-)audio drivers for linux
 *
 * FFADO is based upon FreeBoB.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "fbtypes.h"

#include "devicemanager.h"
#include "ffadodevice.h"

#include "libieee1394/configrom.h"
#include "libieee1394/ieee1394service.h"

#include "libstreaming/generic/StreamProcessor.h"
#include "libstreaming/StreamProcessorManager.h"

#include "debugmodule/debugmodule.h"

#ifdef ENABLE_BEBOB
#include "bebob/bebob_avdevice.h"
#include "maudio/maudio_avdevice.h"
#endif

#ifdef ENABLE_GENERICAVC
    #include "genericavc/avc_avdevice.h"
#endif

#ifdef ENABLE_FIREWORKS
    #include "fireworks/fireworks_device.h"
#endif

#ifdef ENABLE_BOUNCE
#include "bounce/bounce_avdevice.h"
#include "bounce/bounce_slave_avdevice.h"
#endif

#ifdef ENABLE_MOTU
#include "motu/motu_avdevice.h"
#endif

#ifdef ENABLE_RME
#include "rme/rme_avdevice.h"
#endif

#ifdef ENABLE_DICE
#include "dice/dice_avdevice.h"
#endif

#ifdef ENABLE_METRIC_HALO
#include "metrichalo/mh_avdevice.h"
#endif

#include <iostream>
#include <sstream>

#include <algorithm>

using namespace std;

IMPL_DEBUG_MODULE( DeviceManager, DeviceManager, DEBUG_LEVEL_NORMAL );

DeviceManager::DeviceManager()
    : Control::Container("devicemanager")
{
    addOption(Util::OptionContainer::Option("slaveMode",false));
    addOption(Util::OptionContainer::Option("snoopMode",false));
}

DeviceManager::~DeviceManager()
{
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
          it != m_avDevices.end();
          ++it )
    {
        if (!deleteElement(*it)) {
            debugWarning("failed to remove AvDevice from Control::Container\n");
        }
        delete *it;
    }

    for ( Ieee1394ServiceVectorIterator it = m_1394Services.begin();
          it != m_1394Services.end();
          ++it )
    {
        delete *it;
    }
    for ( FunctorVectorIterator it = m_busreset_functors.begin();
          it != m_busreset_functors.end();
          ++it )
    {
        delete *it;
    }
}

bool
DeviceManager::setThreadParameters(bool rt, int priority) {
    if (!m_processorManager.setThreadParameters(rt, priority)) {
        debugError("Could not set processor manager thread parameters\n");
        return false;
    }
    for ( Ieee1394ServiceVectorIterator it = m_1394Services.begin();
          it != m_1394Services.end();
          ++it )
    {
        if (!(*it)->setThreadParameters(rt, priority)) {
            debugError("Could not set 1394 service thread parameters\n");
            return false;
        }
    }
    m_thread_realtime = rt;
    m_thread_priority = priority;
    return true;
}

bool
DeviceManager::initialize()
{
    assert(m_1394Services.size() == 0);
    assert(m_busreset_functors.size() == 0);

    unsigned int nb_detected_ports = Ieee1394Service::detectNbPorts();
    if (nb_detected_ports == 0) {
        debugFatal("No firewire ports found.\n");
        return false;
    }
    debugOutput( DEBUG_LEVEL_VERBOSE, "Found %d firewire adapters (ports)\n", nb_detected_ports);
    for (unsigned int port = 0; port < nb_detected_ports; port++) {
        Ieee1394Service* tmp1394Service = new Ieee1394Service();
        if ( !tmp1394Service ) {
            debugFatal( "Could not create Ieee1349Service object for port %d\n", port );
            return false;
        }
        tmp1394Service->setVerboseLevel( getDebugLevel() );
        m_1394Services.push_back(tmp1394Service);

        tmp1394Service->setThreadParameters(m_thread_realtime, m_thread_priority);
        if ( !tmp1394Service->initialize( port ) ) {
            debugFatal( "Could not initialize Ieee1349Service object for port %d\n", port );
            return false;
        }
        // add the bus reset handler
        Functor* tmp_busreset_functor = new MemberFunctor0< DeviceManager*,
                    void (DeviceManager::*)() >
                    ( this, &DeviceManager::busresetHandler, false );
        if ( !tmp_busreset_functor ) {
            debugFatal( "Could not create busreset handler for port %d\n", port );
            return false;
        }
        m_busreset_functors.push_back(tmp_busreset_functor);

        tmp1394Service->addBusResetHandler( tmp_busreset_functor );
    }

    return true;
}

bool
DeviceManager::addSpecString(char *s) {
    std::string spec = s;
    if(isSpecStringValid(spec)) {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Adding spec string %s\n", spec.c_str());
        m_SpecStrings.push_back(spec);
        return true;
    } else {
        debugError("Invalid spec string: %s\n", spec.c_str());
        return false;
    }
}

bool
DeviceManager::isSpecStringValid(std::string s) {
    return true;
}

void
DeviceManager::busresetHandler()
{
    debugOutput( DEBUG_LEVEL_VERBOSE, "Bus reset...\n" );
    // FIXME: what port was the bus reset on?
    // propagate the bus reset to all avDevices
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
          it != m_avDevices.end();
          ++it )
    {
        (*it)->handleBusReset();
    }
}

bool
DeviceManager::discover( )
{
    bool slaveMode=false;
    if(!getOption("slaveMode", slaveMode)) {
        debugWarning("Could not retrieve slaveMode parameter, defauling to false\n");
    }
    bool snoopMode=false;
    if(!getOption("snoopMode", snoopMode)) {
        debugWarning("Could not retrieve snoopMode parameter, defauling to false\n");
    }

    setVerboseLevel(getDebugLevel());

    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
          it != m_avDevices.end();
          ++it )
    {
        if (!deleteElement(*it)) {
            debugWarning("failed to remove AvDevice from Control::Container\n");
        }
        delete *it;
    }
    m_avDevices.clear();

    if (!slaveMode) {
        for ( Ieee1394ServiceVectorIterator it = m_1394Services.begin();
            it != m_1394Services.end();
            ++it )
        {
            Ieee1394Service *portService = *it;
            for ( fb_nodeid_t nodeId = 0;
                nodeId < portService->getNodeCount();
                ++nodeId )
            {
                debugOutput( DEBUG_LEVEL_VERBOSE, "Probing node %d...\n", nodeId );
    
                if (nodeId == portService->getLocalNodeId()) {
                    debugOutput( DEBUG_LEVEL_VERBOSE, "Skipping local node (%d)...\n", nodeId );
                    continue;
                }
    
                std::auto_ptr<ConfigRom> configRom =
                    std::auto_ptr<ConfigRom>( new ConfigRom( *portService, nodeId ) );
                if ( !configRom->initialize() ) {
                    // \todo If a PHY on the bus is in power safe mode then
                    // the config rom is missing. So this might be just
                    // such this case and we can safely skip it. But it might
                    // be there is a real software problem on our side.
                    // This should be handlede more carefuly.
                    debugOutput( DEBUG_LEVEL_NORMAL,
                                "Could not read config rom from device (node id %d). "
                                "Skip device discovering for this node\n",
                                nodeId );
                    continue;
                }

                bool already_in_vector = false;
                for ( FFADODeviceVectorIterator it = m_avDevices.begin();
                    it != m_avDevices.end();
                    ++it )
                {
                    if ((*it)->getConfigRom().getGuid() == configRom->getGuid()) {
                        already_in_vector = true;
                        break;
                    }
                }
                if (already_in_vector) {
                    debugWarning("Device with GUID %s already discovered on other port, skipping device...\n",
                                 configRom->getGuidString().c_str());
                    continue;
                }

                if( getDebugLevel() >= DEBUG_LEVEL_VERBOSE) {
                    configRom->printConfigRom();
                }

                FFADODevice* avDevice = getDriverForDevice( configRom,
                                                            nodeId );

                if ( avDevice ) {
                    debugOutput( DEBUG_LEVEL_NORMAL,
                                "driver found for device %d\n",
                                nodeId );

                    avDevice->setVerboseLevel( getDebugLevel() );
                    bool isFromCache = false;
                    if ( avDevice->loadFromCache() ) {
                        debugOutput( DEBUG_LEVEL_VERBOSE, "could load from cache\n" );
                        isFromCache = true;
                        // restore the debug level for everything that was loaded
                        avDevice->setVerboseLevel( getDebugLevel() );
                    } else if ( avDevice->discover() ) {
                        debugOutput( DEBUG_LEVEL_VERBOSE, "discovery successful\n" );
                    } else {
                        debugError( "could not discover device\n" );
                        delete avDevice;
                        continue;
                    }

                    if (snoopMode) {
                        debugOutput( DEBUG_LEVEL_VERBOSE,
                                    "Enabling snoop mode on node %d...\n", nodeId );

                        if(!avDevice->setOption("snoopMode", snoopMode)) {
                            debugWarning("Could not set snoop mode for device on node %d\n", nodeId);
                            delete avDevice;
                            continue;
                        }
                    }

                    if ( !isFromCache && !avDevice->saveCache() ) {
                        debugOutput( DEBUG_LEVEL_VERBOSE, "No cached version of AVC model created\n" );
                    }
                    m_avDevices.push_back( avDevice );

                    if (!addElement(avDevice)) {
                        debugWarning("failed to add AvDevice to Control::Container\n");
                    }

                    debugOutput( DEBUG_LEVEL_NORMAL, "discovery of node %d on port %d done...\n", nodeId, portService->getPort() );
                }
            }
        }

        debugOutput( DEBUG_LEVEL_NORMAL, "Discovery finished...\n" );
        // FIXME: do better sorting
        // sort the m_avDevices vector on their GUID
        // then assign reassign the id's to the devices
        // the side effect of this is that for the same set of attached devices,
        // a device id always corresponds to the same device
        sort(m_avDevices.begin(), m_avDevices.end(), FFADODevice::compareGUID);
        int i=0;
        for ( FFADODeviceVectorIterator it = m_avDevices.begin();
            it != m_avDevices.end();
            ++it )
        {
            if ( !(*it)->setId( i++ ) ) {
                debugError( "setting Id failed\n" );
            }
        }
        showDeviceInfo();
        return true;
    } else { // slave mode
        Ieee1394Service *portService = m_1394Services.at(0);
        fb_nodeid_t nodeId = portService->getLocalNodeId();
        debugOutput( DEBUG_LEVEL_VERBOSE, "Starting in slave mode on node %d...\n", nodeId );

        std::auto_ptr<ConfigRom> configRom =
            std::auto_ptr<ConfigRom>( new ConfigRom( *portService,
                                                     nodeId ) );
        if ( !configRom->initialize() ) {
            // \todo If a PHY on the bus is in power safe mode then
            // the config rom is missing. So this might be just
            // such this case and we can safely skip it. But it might
            // be there is a real software problem on our side.
            // This should be handled more carefuly.
            debugOutput( DEBUG_LEVEL_NORMAL,
                         "Could not read config rom from device (node id %d). "
                         "Skip device discovering for this node\n",
                         nodeId );
            return false;
        }

        FFADODevice* avDevice = getSlaveDriver( configRom );
        if ( avDevice ) {
            debugOutput( DEBUG_LEVEL_NORMAL,
                         "driver found for device %d\n",
                         nodeId );

            avDevice->setVerboseLevel( getDebugLevel() );

            if ( !avDevice->discover() ) {
                debugError( "could not discover device\n" );
                delete avDevice;
                return false;
            }

            if ( !avDevice->setId( m_avDevices.size() ) ) {
                debugError( "setting Id failed\n" );
            }
            if ( getDebugLevel() >= DEBUG_LEVEL_VERBOSE ) {
                avDevice->showDevice();
            }
            m_avDevices.push_back( avDevice );
            debugOutput( DEBUG_LEVEL_NORMAL, "discovery of node %d on port %d done...\n", nodeId, portService->getPort() );
        }

        debugOutput( DEBUG_LEVEL_NORMAL, "discovery finished...\n" );
        return true;
    }
}

bool
DeviceManager::initStreaming()
{
    // iterate over the found devices
    // add the stream processors of the devices to the managers
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
        it != m_avDevices.end();
        ++it )
    {
        FFADODevice *device = *it;
        assert(device);

        debugOutput(DEBUG_LEVEL_VERBOSE, "Locking device (%p)\n", device);

        if (!device->lock()) {
            debugWarning("Could not lock device, skipping device (%p)!\n", device);
            continue;
        }

        debugOutput(DEBUG_LEVEL_VERBOSE, "Setting samplerate to %d for (%p)\n",
                    m_processorManager.getNominalRate(), device);

        // Set the device's sampling rate to that requested
        // FIXME: does this really belong here?  If so we need to handle errors.
        if (!device->setSamplingFrequency(m_processorManager.getNominalRate())) {
            debugOutput(DEBUG_LEVEL_VERBOSE, " => Retry setting samplerate to %d for (%p)\n",
                        m_processorManager.getNominalRate(), device);

            // try again:
            if (!device->setSamplingFrequency(m_processorManager.getNominalRate())) {
                debugFatal("Could not set sampling frequency to %d\n",m_processorManager.getNominalRate());
                return false;
            }
        }
        // prepare the device
        device->prepare();
    }

    // set the sync source
    if (!m_processorManager.setSyncSource(getSyncSource())) {
        debugWarning("Could not set processorManager sync source (%p)\n",
            getSyncSource());
    }
    return true;
}

bool
DeviceManager::prepareStreaming()
{
    if (!m_processorManager.prepare()) {
        debugFatal("Could not prepare streaming...\n");
        return false;
    }
    return true;
}

bool
DeviceManager::finishStreaming() {
    bool result = true;
    // iterate over the found devices
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
        it != m_avDevices.end();
        ++it )
    {
        debugOutput(DEBUG_LEVEL_VERBOSE, "Unlocking device (%p)\n", *it);

        if (!(*it)->unlock()) {
            debugWarning("Could not unlock device (%p)!\n", *it);
            result = false;
        }
    }
    return result;
}

bool
DeviceManager::startStreaming() {
    // create the connections for all devices
    // iterate over the found devices
    // add the stream processors of the devices to the managers
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
        it != m_avDevices.end();
        ++it )
    {
        FFADODevice *device = *it;
        assert(device);

        int j=0;
        for(j=0; j < device->getStreamCount(); j++) {
        debugOutput(DEBUG_LEVEL_VERBOSE,"Starting stream %d of device %p\n", j, device);
            // start the stream
            if (!device->startStreamByIndex(j)) {
                debugWarning("Could not start stream %d of device %p\n", j, device);
                continue;
            }
        }

        if (!device->enableStreaming()) {
            debugWarning("Could not enable streaming on device %p!\n", device);
        }
    }

    if(m_processorManager.start()) {
        return true;
    } else {
        stopStreaming();
        return false;
    }
}

bool
DeviceManager::resetStreaming() {
    return true;
}

bool
DeviceManager::stopStreaming()
{
    bool result = true;
    m_processorManager.stop();

    // create the connections for all devices
    // iterate over the found devices
    // add the stream processors of the devices to the managers
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
        it != m_avDevices.end();
        ++it )
    {
        FFADODevice *device = *it;
        assert(device);

        if (!device->disableStreaming()) {
            debugWarning("Could not disable streaming on device %p!\n", device);
        }

        int j=0;
        for(j=0; j < device->getStreamCount(); j++) {
            debugOutput(DEBUG_LEVEL_VERBOSE,"Stopping stream %d of device %p\n", j, device);
            // stop the stream
            // start the stream
            if (!device->stopStreamByIndex(j)) {
                debugWarning("Could not stop stream %d of device %p\n", j, device);
                result = false;
                continue;
            }
        }
    }
    return result;
}

bool
DeviceManager::waitForPeriod() {
    if(m_processorManager.waitForPeriod()) {
        return true;
    } else {
        debugWarning("XRUN detected\n");
        // do xrun recovery
        m_processorManager.handleXrun();
        return false;
    }
}

bool
DeviceManager::setStreamingParams(unsigned int period, unsigned int rate, unsigned int nb_buffers) {
    m_processorManager.setPeriodSize(period);
    m_processorManager.setNominalRate(rate);
    m_processorManager.setNbBuffers(nb_buffers);
    return true;
}

FFADODevice*
DeviceManager::getDriverForDevice( std::auto_ptr<ConfigRom>( configRom ),
                                   int id )
{
#ifdef ENABLE_BEBOB
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying BeBoB...\n" );
    if ( BeBoB::AvDevice::probe( *configRom.get() ) ) {
        return BeBoB::AvDevice::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_GENERICAVC
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying Generic AV/C...\n" );
    if ( GenericAVC::AvDevice::probe( *configRom.get() ) ) {
        return GenericAVC::AvDevice::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_FIREWORKS
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying ECHO Audio FireWorks...\n" );
    if ( FireWorks::Device::probe( *configRom.get() ) ) {
        return FireWorks::Device::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_BEBOB
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying M-Audio...\n" );
    if ( MAudio::AvDevice::probe( *configRom.get() ) ) {
        return MAudio::AvDevice::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_MOTU
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying Motu...\n" );
    if ( Motu::MotuDevice::probe( *configRom.get() ) ) {
        return Motu::MotuDevice::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_DICE
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying Dice...\n" );
    if ( Dice::DiceAvDevice::probe( *configRom.get() ) ) {
        return Dice::DiceAvDevice::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_METRIC_HALO
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying Metric Halo...\n" );
    if ( MetricHalo::MHAvDevice::probe( *configRom.get() ) ) {
        return MetricHalo::MHAvDevice::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_RME
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying RME...\n" );
    if ( Rme::RmeDevice::probe( *configRom.get() ) ) {
        return Rme::RmeDevice::createDevice( *this, configRom );
    }
#endif

#ifdef ENABLE_BOUNCE
    debugOutput( DEBUG_LEVEL_VERBOSE, "Trying Bounce...\n" );
    if ( Bounce::BounceDevice::probe( *configRom.get() ) ) {
        return Bounce::BounceDevice::createDevice( *this, configRom );
    }
#endif

    return 0;
}

FFADODevice*
DeviceManager::getSlaveDriver( std::auto_ptr<ConfigRom>( configRom ) )
{

#ifdef ENABLE_BOUNCE
    if ( Bounce::BounceSlaveDevice::probe( *configRom.get() ) ) {
        return Bounce::BounceSlaveDevice::createDevice( configRom );
    }
#endif

    return 0;
}

bool
DeviceManager::isValidNode(int node)
{
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
          it != m_avDevices.end();
          ++it )
    {
        FFADODevice* avDevice = *it;

        if (avDevice->getConfigRom().getNodeId() == node) {
            return true;
    }
    }
    return false;
}

int
DeviceManager::getNbDevices()
{
    return m_avDevices.size();
}

int
DeviceManager::getDeviceNodeId( int deviceNr )
{
    if ( ! ( deviceNr < getNbDevices() ) ) {
        debugError( "Device number out of range (%d)\n", deviceNr );
        return -1;
    }

    FFADODevice* avDevice = m_avDevices.at( deviceNr );

    if ( !avDevice ) {
        debugError( "Could not get device at position (%d)\n",  deviceNr );
    }

    return avDevice->getConfigRom().getNodeId();
}

FFADODevice*
DeviceManager::getAvDevice( int nodeId )
{
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
          it != m_avDevices.end();
          ++it )
    {
        FFADODevice* avDevice = *it;
        if ( avDevice->getConfigRom().getNodeId() == nodeId ) {
            return avDevice;
        }
    }

    return 0;
}

FFADODevice*
DeviceManager::getAvDeviceByIndex( int idx )
{
    return m_avDevices.at(idx);
}

unsigned int
DeviceManager::getAvDeviceCount( )
{
    return m_avDevices.size();
}

/**
 * Return the streamprocessor that is to be used as
 * the sync source.
 *
 * Algorithm still to be determined
 *
 * @return StreamProcessor that is sync source
 */
Streaming::StreamProcessor *
DeviceManager::getSyncSource() {
    FFADODevice* device = getAvDeviceByIndex(0);

    bool slaveMode=false;
    if(!getOption("slaveMode", slaveMode)) {
        debugWarning("Could not retrieve slaveMode parameter, defauling to false\n");
    }

    #warning TEST CODE FOR BOUNCE DEVICE !!
    // this makes the bounce slave use the xmit SP as sync source
    if (slaveMode) {
        return device->getStreamProcessorByIndex(1);
    } else {
        return device->getStreamProcessorByIndex(0);
    }
}

bool
DeviceManager::deinitialize()
{
    return true;
}


void
DeviceManager::setVerboseLevel(int l)
{
    setDebugLevel(l);
    Control::Element::setVerboseLevel(l);
    m_processorManager.setVerboseLevel(l);
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
          it != m_avDevices.end();
          ++it )
    {
        (*it)->setVerboseLevel(l);
    }
    for ( Ieee1394ServiceVectorIterator it = m_1394Services.begin();
          it != m_1394Services.end();
          ++it )
    {
        (*it)->setVerboseLevel(l);
    }
    debugOutput( DEBUG_LEVEL_VERBOSE, "Setting verbose level to %d...\n", l );
}

void
DeviceManager::showDeviceInfo() {
    debugOutput(DEBUG_LEVEL_NORMAL, "===== Device Manager =====\n");
    Control::Element::show();

    int i=0;
    for ( Ieee1394ServiceVectorIterator it = m_1394Services.begin();
          it != m_1394Services.end();
          ++it )
    {
        debugOutput(DEBUG_LEVEL_NORMAL, "--- IEEE1394 Service %2d ---\n", i++);
        (*it)->show();
    }

    i=0;
    for ( FFADODeviceVectorIterator it = m_avDevices.begin();
        it != m_avDevices.end();
        ++it )
    {
        FFADODevice* avDevice = *it;
        debugOutput(DEBUG_LEVEL_NORMAL, "--- Device %2d ---\n", i++);
        avDevice->showDevice();

        debugOutput(DEBUG_LEVEL_NORMAL, "Clock sync sources:\n");
        FFADODevice::ClockSourceVector sources=avDevice->getSupportedClockSources();
        for ( FFADODevice::ClockSourceVector::const_iterator it
                = sources.begin();
            it != sources.end();
            ++it )
        {
            FFADODevice::ClockSource c=*it;
            debugOutput(DEBUG_LEVEL_NORMAL, " Type: %s, Id: %2d, Valid: %1d, Active: %1d, Locked %1d, Slipping: %1d, Description: %s\n",
                FFADODevice::ClockSourceTypeToString(c.type), c.id, c.valid, c.active, c.locked, c.slipping, c.description.c_str());
        }
    }
}
void
DeviceManager::showStreamingInfo() {
    m_processorManager.dumpInfo();
}
