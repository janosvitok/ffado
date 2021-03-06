/*
 * Copyright (C) 2005-2008 by Daniel Wagner
 *
 * This file is part of FFADO
 * FFADO = Free Firewire (pro-)audio drivers for linux
 *
 * FFADO is based upon FreeBoB.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) version 3 of the License.
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

#ifndef GENERICAVC_VENDORMODEL_H
#define GENERICAVC_VENDORMODEL_H

#include <string>
#include <vector>

namespace GenericAVC {

// struct to define the supported devices
struct VendorModelEntry {
    VendorModelEntry();
    VendorModelEntry(const VendorModelEntry& rhs);
    VendorModelEntry& operator = (const VendorModelEntry& rhs);
    bool operator == (const VendorModelEntry& rhs) const;
    virtual ~VendorModelEntry();

    unsigned int vendor_id;
    unsigned int model_id;
    std::string vendor_name;
    std::string model_name;
};

typedef std::vector<VendorModelEntry> VendorModelEntryVector;

class VendorModel {
public:
    VendorModel( const char* filename );
    virtual ~VendorModel();

    virtual bool parse();
    virtual bool printTable() const;
    virtual bool handleAdditionalEntries(VendorModelEntry& vme,
                                         std::vector<std::string>& v,
                                         std::vector<std::string>::const_iterator& b,
                                         std::vector<std::string>::const_iterator& e );
    VendorModelEntry find( unsigned int vendor_id,  unsigned model_id );
    bool isPresent( unsigned int vendor_id,  unsigned model_id );
    static bool isValid( const VendorModelEntry& vme );

    const VendorModelEntryVector& getVendorModelEntries() const;
private:
    std::string m_filename;
    VendorModelEntryVector m_vendorModelEntries;

};

}

#endif
