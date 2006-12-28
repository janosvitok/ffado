/* avdevicepool.cpp
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

#include <queue>

#include "avdevice.h"
#include "avdevicepool.h"

AvDevicePool* AvDevicePool::m_pInstance = 0;

AvDevicePool::AvDevicePool()
{
    setDebugLevel( DEBUG_LEVEL_ALL );
}

AvDevicePool::~AvDevicePool()
{
  AvDeviceVectorIterator it = m_avDevices.begin();

  for(; it < m_avDevices.end(); it++) {
	delete *it;
  }

}

AvDevicePool*
AvDevicePool::instance()
{
    if ( !m_pInstance ) {
        m_pInstance = new AvDevicePool;
    }
    return m_pInstance;
}

void
AvDevicePool::shutdown()
{
	delete this;
}

FBReturnCodes
AvDevicePool::registerAvDevice(AvDevice* pAvDevice)
{
    m_avDevices.push_back( pAvDevice );

    debugPrint( DEBUG_LEVEL_INFO, "AvDevice 0x%08x added to device pool.\n",
                pAvDevice );
    debugPrint( DEBUG_LEVEL_INFO, "Pool size = %d\n", m_avDevices.size() );

    return eFBRC_Success;
}


FBReturnCodes
AvDevicePool::unregisterAvDevice(AvDevice* pAvDevice)
{
    AvDeviceVector::iterator iter;
    for ( iter = m_avDevices.begin();
          iter != m_avDevices.end();
          ++iter )
    {
        if ( ( *iter ) == pAvDevice ) {
            debugPrint( DEBUG_LEVEL_INFO,
                        "AvDevice 0x%08x removing from device pool\n",
                        ( *iter ) );
            m_avDevices.erase( iter );
            debugPrint( DEBUG_LEVEL_INFO,
                        "Pool size = %d\n",
                        m_avDevices.size() );
            return eFBRC_Success;
        }
    }

    if ( iter == m_avDevices.end() ) {
        return eFBRC_AvDeviceNotFound;
    }

    return eFBRC_Success;
}

AvDevice*
AvDevicePool::getAvDevice(octlet_t oGuid)
{
    AvDevice* pAvDevice = 0;
    for ( AvDeviceVector::iterator iter = m_avDevices.begin();
          iter != m_avDevices.end();
          ++iter )
    {
        if ( ( *iter )->getGuid() == oGuid ) {
            pAvDevice = *iter;
            break;
        }
    }
    return pAvDevice;
}
