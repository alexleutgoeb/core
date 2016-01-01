/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) 2009-2016 Peter Schüller
 * Copyright (C) 2011-2016 Christoph Redl
 * Copyright (C) 2015-2016 Tobias Kaminski
 * Copyright (C) 2015-2016 Antonius Weinzierl
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
 * @file   OrdinaryASPSolver.hpp
 * @author Peter Schueller
 *
 * @brief  Abstract interface to nonground disjunctive ASP Grounder and Solver.
 */

#if !defined(_DLVHEX_ORDINARYASPSOLVER_HPP)
#define _DLVHEX_ORDINARYASPSOLVER_HPP

#include "dlvhex2/Interpretation.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/OrdinaryASPProgram.h"

#include <boost/shared_ptr.hpp>

DLVHEX_NAMESPACE_BEGIN

/**
 * \brief Abstract base class to capture ASP solvers.
 */
class OrdinaryASPSolver
{
    public:
        // instantiate (implement for each derived class)
        // static OrdinaryASPSolverPtr getInstance(ProgramCtx& ctx, OrdinaryASPProgram& program);

        /**
         * \brief Returns the next model.
         *
         * This will also trigger callbacks to the propagators, see addPropagator.
         * @return The next model or a NULL-pointer of no more models exist.
         */
        virtual InterpretationPtr getNextModel() = 0;
};
typedef boost::shared_ptr<OrdinaryASPSolver> OrdinaryASPSolverPtr;

DLVHEX_NAMESPACE_END
#endif                           // _DLVHEX_ORDINARYASPSOLVER_HPP


// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
