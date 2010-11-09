/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005, 2006, 2007 Roman Schindlauer
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Thomas Krennwallner
 * Copyright (C) 2009, 2010 Peter Schüller
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/**
 * @file Interpretation.cpp
 * @author Peter Schüller
 *
 * @brief Implementation of the (bitset-)interpretation.
 */

#include "dlvhex/Interpretation.hpp"
#include "dlvhex/Logger.hpp"

DLVHEX_NAMESPACE_BEGIN

Interpretation::Interpretation()
{
}

Interpretation::~Interpretation()
{
}

std::ostream& Interpretation::print(std::ostream& o) const
{
#warning TODO
  throw std::runtime_error("TODO");
}

void Interpretation::add(const Interpretation& other)
{
  throw std::runtime_error("TODO");
}

void Interpretation::setFact(IDAddress id)
{
  throw std::runtime_error("TODO");
}

DLVHEX_NAMESPACE_END