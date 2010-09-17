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
 * @file ASPSolverManager.cpp
 * @author Peter Schüller
 * @date Tue Jul 13 2010
 *
 * @brief ASP Solver Manager
 *
 *
 */

#include "dlvhex/ASPSolverManager.h"

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#  ifdef DLVHEX_DEBUG
#    define DLVHEX_BENCHMARK
#  endif
#endif

#include "dlvhex/Benchmarking.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/Program.h"
#include "dlvhex/globals.h"
#include "dlvhex/AtomSet.h"

#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp> // seems to be required for scope_exit
#include <boost/foreach.hpp>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN


ASPSolverManager::GenericOptions::GenericOptions():
  includeFacts(false)
{
}

ASPSolverManager::GenericOptions::~GenericOptions()
{
}


ASPSolverManager::ASPSolverManager()
{
}

namespace
{
  ASPSolverManager* instance = 0;
}

//static
ASPSolverManager& ASPSolverManager::Instance()
{
  if( instance == 0 )
    instance = new ASPSolverManager;
  return *instance;
}

//! solve idb/edb and add to result
void ASPSolverManager::solve(
    const SoftwareConfigurationBase& solver,
    const Program& idb, const AtomSet& edb,
    std::vector<AtomSet>& result) throw (FatalError)
{
  DelegatePtr delegate = solver.createDelegate();
  delegate->useASTInput(idb, edb);
  delegate->getOutput(result);
}

// solve string program and add to result
void ASPSolverManager::solveString(
    const SoftwareConfigurationBase& solver,
    const std::string& program,
    std::vector<AtomSet>& result) throw (FatalError)
{
  DelegatePtr delegate = solver.createDelegate();
  delegate->useStringInput(program);
  delegate->getOutput(result);
}

// solve program in file and add to result
void ASPSolverManager::solveFile(
    const SoftwareConfigurationBase& solver,
    const std::string& filename,
    std::vector<AtomSet>& result) throw (FatalError)
{
  DelegatePtr delegate = solver.createDelegate();
  delegate->useFileInput(filename);
  delegate->getOutput(result);
}


DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End: