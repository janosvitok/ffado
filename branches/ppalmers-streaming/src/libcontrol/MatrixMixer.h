/*
 * Copyright (C) 2007 by Pieter Palmers
 *
 * This file is part of FFADO
 * FFADO = Free Firewire (pro-)audio drivers for linux
 *
 * FFADO is based upon FreeBoB
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software Foundation;
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifndef CONTROL_MATRIX_MIXER_H
#define CONTROL_MATRIX_MIXER_H

#include "debugmodule/debugmodule.h"

#include "Element.h"

#include <vector>
#include <string>

namespace Control {

/*!
@brief Abstract Base class for Matrix Mixer elements

*/
class MatrixMixer : public Element
{
public:
    MatrixMixer();
    MatrixMixer(std::string n);
    virtual ~MatrixMixer() {};

    virtual void show() = 0;

    virtual std::string getRowName( const int ) = 0;
    virtual std::string getColName( const int ) = 0;
    virtual int canWrite( const int, const int ) = 0;
    virtual double setValue( const int, const int, const double ) = 0;
    virtual double getValue( const int, const int ) = 0;
    virtual int getRowCount( ) = 0;
    virtual int getColCount( ) = 0;

protected:

};


}; // namespace Control

#endif // CONTROL_MATRIX_MIXER_H
