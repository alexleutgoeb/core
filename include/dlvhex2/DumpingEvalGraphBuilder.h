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
 * @file   DumpingEvalGraphBuilder.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Evaluation Graph builder that dumps its evaluation plan.
 */

#ifndef DUMPING_EVAL_GRAPH_BUILDER_HPP_INCLUDED__16112011
#define DUMPING_EVAL_GRAPH_BUILDER_HPP_INCLUDED__16112011

#include "dlvhex2/EvalGraphBuilder.h"
#include <fstream>

DLVHEX_NAMESPACE_BEGIN

/** \brief Evaluation Graph builder that dumps its evaluation plan. */
class DumpingEvalGraphBuilder:
public EvalGraphBuilder
{
    protected:
        /** \brief Stream where the EvaluationGraph is dumped to. */
        std::ofstream output;
        /** \brief Assignment of unique indexes to components. */
        std::map<ComponentGraph::Component, unsigned> componentidx;

        //////////////////////////////////////////////////////////////////////////////
        // methods
        //////////////////////////////////////////////////////////////////////////////
    public:
        /** \brief Constructor.
         * @param ctx See EvalGraphBuilder::ctx.
         * @param cg See EvalGraphBuilder::cg.
         * @param eg Evaluation graph to write the result to.
         * @param externalEvalConfig See ASPSolverManager::SoftwareConfiguration.
         * @param outputfilename Name of the file where the evaluation graph is written to. */
        DumpingEvalGraphBuilder(
            ProgramCtx& ctx, ComponentGraph& cg, EvalGraphT& eg,
            ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig,
            const std::string& outputfilename);
        /** \brief Destructor. */
        virtual ~DumpingEvalGraphBuilder();

        // write to file how eval units were created
        virtual EvalUnit createEvalUnit(
            const std::list<Component>& comps, const std::list<Component>& ccomps);
};

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
