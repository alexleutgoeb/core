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
 * @file ManualEvalHeuristicsPlugin.h
 * @author Peter Schueller
 *
 * @brief Plugin for specifying evaluation units in HEX input.
 */

#ifndef MANUALEVALHEURISTICS_PLUGIN__HPP_INCLUDED_1518
#define MANUALEVALHEURISTICS_PLUGIN__HPP_INCLUDED_1518

#include "dlvhex2/PlatformDefinitions.h"
#include "dlvhex2/PluginInterface.h"

DLVHEX_NAMESPACE_BEGIN

/** \brief Allows for implementing evaluation heuristics by manually specifying the units. */
class ManualEvalHeuristicsPlugin:
public PluginInterface
{
    public:
        // stored in ProgramCtx, accessed using getPluginData<ManualEvalHeuristicsPlugin>()
        class CtxData:
    public PluginData
    {
        public:
            /** \brief Stores if plugin is enabled. */
            bool enabled;

            /** \brief ID of last rule in input (other rules are auxiliary, created by hex or plugins). */
            ID lastUserRuleID;

            /** \brief Running index used during parsing (rules of which unit are we currently parsing).
             *
             * Assumption: parser processes rules in input and plugin understandable instructions sequentially.
             */
            unsigned currentUnit;

            typedef std::pair<ID,unsigned> UnitInstruction;
            typedef std::list<UnitInstruction> InstructionList;
            /** \brief Maximum rule id parsed is stored for each unit instruction.
             *
             * The ID might be ID_FAIL which means that no rule comes before that, i.e., the first instruction in a file was #evalunit(...).
             */
            InstructionList instructions;

            CtxData();
            virtual ~CtxData() {};
    };

    /** \brief Constructor. */
    ManualEvalHeuristicsPlugin();
    /** \brief Destructor. */
    virtual ~ManualEvalHeuristicsPlugin();

    // output help message for this plugin
    virtual void printUsage(std::ostream& o) const;

    // accepted options: --manualevalheuristics-enable
    //
    // processes options for this plugin, and removes recognized options from pluginOptions
    // (do not free the pointers, the const char* directly come from argv)
    //
    // configures custom evaluation heuristics
    virtual void processOptions(std::list<const char*>& pluginOptions, ProgramCtx&);

    // create parser modules that extend and the basic hex grammar
    // this parser also stores the query information into the plugin
    virtual std::vector<HexParserModulePtr> createParserModules(ProgramCtx&);

    // create program rewriter (we use it just to gather information from the parsed program)
    virtual PluginRewriterPtr createRewriter(ProgramCtx& ctx);
};

DLVHEX_NAMESPACE_END
#endif

// vim:expandtab:ts=4:sw=4:
// mode: C++
// End:
