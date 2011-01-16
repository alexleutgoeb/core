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
 * @file   fixturesMCS.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Interface for testing fixtures related to MCS-IE.
 *
 * Here we provide two slightly modified encodings from the medical example:
 * calculating equilibria using KR2010 encoding, and calculating diagnoses
 * using KR2010 encoding. The modification is the addition of two extra rules
 * without external atoms that form an SCC.
 */

#ifndef FIXTURES_MCS_HPP_INCLUDED__08112010
#define FIXTURES_MCS_HPP_INCLUDED__08112010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/ComponentGraph.hpp"
#include "dlvhex/DependencyGraph.hpp"
#include "dlvhex/HexParser.hpp"
#include "dlvhex/ProgramCtx.h"
#include "dlvhex/Registry.hpp"
#include "dlvhex/PluginInterface.h"
#include "fixturesDepgraphCompgraphGeneric.hpp"

class TestPluginAspCtxAcc:
	public dlvhex::PluginAtom
{
public:
	TestPluginAspCtxAcc():
    dlvhex::PluginAtom("dlv_asp_context_acc", false)
	{
		inputSize = 5;
		outputSize = 0;
		inputType.push_back(CONSTANT);
		inputType.push_back(PREDICATE);
		inputType.push_back(PREDICATE);
		inputType.push_back(PREDICATE);
		inputType.push_back(CONSTANT);
	}

	// won't be used
	virtual void retrieve(const Query&, Answer&) throw (dlvhex::PluginError)
		{ assert(false); }
};

// provide program for equilibrium calculation
// parse into ProgramCtx
// register dummy atoms
struct ProgramMCSMedEQProgramCtxFixture
{
  std::string program;
  dlvhex::ProgramCtx ctx;
	dlvhex::PluginAtomPtr papAspCtxAcc;

  ProgramMCSMedEQProgramCtxFixture();
  ~ProgramMCSMedEQProgramCtxFixture() {}
};

typedef GenericDepGraphFixture<ProgramMCSMedEQProgramCtxFixture>
  ProgramMCSMedEQProgramCtxDependencyGraphFixture;
typedef GenericDepGraphCompGraphFixture<ProgramMCSMedEQProgramCtxFixture>
  ProgramMCSMedEQProgramCtxDependencyGraphComponentGraphFixture;

// provide program for diagnosis calculation
// parse into ProgramCtx
// register dummy atoms
struct ProgramMCSMedDProgramCtxFixture
{
  std::string program;
  dlvhex::ProgramCtx ctx;
	dlvhex::PluginAtomPtr papAspCtxAcc;

  ProgramMCSMedDProgramCtxFixture();
  ~ProgramMCSMedDProgramCtxFixture() {}
};

typedef GenericDepGraphFixture<ProgramMCSMedDProgramCtxFixture>
  ProgramMCSMedDProgramCtxDependencyGraphFixture;
typedef GenericDepGraphCompGraphFixture<ProgramMCSMedDProgramCtxFixture>
  ProgramMCSMedDProgramCtxDependencyGraphComponentGraphFixture;

//
// implementation
//

ProgramMCSMedEQProgramCtxFixture::ProgramMCSMedEQProgramCtxFixture():
  papAspCtxAcc(new TestPluginAspCtxAcc)
{
  using namespace dlvhex;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  papAspCtxAcc->setRegistry(ctx.registry());
  ID idAspCtxAcc = papAspCtxAcc->getPredicateID();
  BOOST_REQUIRE(idAspCtxAcc != ID_FAIL);
  LOG(INFO,"got ID: dlv_asp_context_acc = " << idAspCtxAcc);

  // program was obtained from trunk of mcs-ie via 'dlvhex --verbose=15 --plugindir=`pwd`/../build/src medExample/master.hex --ieenable --ieuseKR2010rewriting'
  std::stringstream ss;
  ss <<
    "foo(X,c) :- bar. foo(c,Y) :- baz." << std::endl << // this is not from MCS, but required to test scc dependencies!
    "o2(xray_pneumonia)." << std::endl <<
    "b3(pneumonia) :- a2(xray_pneumonia)." << std::endl <<
    "o2(blood_marker)." << std::endl <<
    "b3(marker) :- a2(blood_marker)." << std::endl <<
    "o3(pneumonia)." << std::endl <<
    "b4(need_ab) :- a3(pneumonia)." << std::endl <<
    "o3(atyppneumonia)." << std::endl <<
    "b4(need_strong) :- a3(atyppneumonia)." << std::endl <<
    "o1(allergy_strong_ab)." << std::endl <<
    "b4(allow_strong_ab) :- na1(allergy_strong_ab)." << std::endl <<
    "a1(X) v na1(X) :- o1(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[1,a1,b1,o1,\"./medExample/kb1.dlv\"]()." << std::endl <<
    "ctx(1)." << std::endl <<
    "a2(X) v na2(X) :- o2(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[2,a2,b2,o2,\"./medExample/kb2.dlv\"]()." << std::endl <<
    "ctx(2)." << std::endl <<
    "a3(X) v na3(X) :- o3(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[3,a3,b3,o3,\"./medExample/kb3.dlv\"]()." << std::endl <<
    "ctx(3)." << std::endl <<
    "a4(X) v na4(X) :- o4(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[4,a4,b4,o4,\"./medExample/kb4.dlv\"]()." << std::endl <<
    "ctx(4)." << std::endl;
  program = ss.str();
  HexParser parser(ctx);
  parser.parse(ss);

  //TODO this should become a common functionality using some pluginAtom registry
	{
		ExternalAtomTable::PredicateIterator it, it_end;
		for(boost::tie(it, it_end) = ctx.registry()->eatoms.getRangeByPredicateID(idAspCtxAcc);
				it != it_end; ++it)
		{
			ExternalAtom ea(*it);
			ea.pluginAtom = papAspCtxAcc;
			ctx.registry()->eatoms.update(*it, ea);
		}
	}
}

ProgramMCSMedDProgramCtxFixture::ProgramMCSMedDProgramCtxFixture():
  papAspCtxAcc(new TestPluginAspCtxAcc)
{
  using namespace dlvhex;
  ctx.setupRegistryPluginContainer(RegistryPtr(new Registry));

  papAspCtxAcc->setRegistry(ctx.registry());
  ID idAspCtxAcc = papAspCtxAcc->getPredicateID();
  BOOST_REQUIRE(idAspCtxAcc != ID_FAIL);
  LOG(INFO,"got ID: dlv_asp_context_acc = " << idAspCtxAcc);

  // program was obtained from trunk of mcs-ie via 'dlvhex --verbose=15 --plugindir=`pwd`/../build/src medExample/master.hex --ieenable --ieuseKR2010rewriting --ieexplain=D'
  std::stringstream ss;
  ss <<
    "o2(xray_pneumonia)." << std::endl <<
    "normal(r1) v d1(r1) v d2(r1)." << std::endl <<
    "b3(pneumonia) :- d2(r1)." << std::endl <<
    "b3(pneumonia) :- not d1(r1), a2(xray_pneumonia)." << std::endl <<
    "o2(blood_marker)." << std::endl <<
    "normal(r2) v d1(r2) v d2(r2)." << std::endl <<
    "b3(marker) :- d2(r2)." << std::endl <<
    "b3(marker) :- not d1(r2), a2(blood_marker)." << std::endl <<
    "o3(pneumonia)." << std::endl <<
    "normal(r3) v d1(r3) v d2(r3)." << std::endl <<
    "b4(need_ab) :- d2(r3)." << std::endl <<
    "b4(need_ab) :- not d1(r3), a3(pneumonia)." << std::endl <<
    "o3(atyppneumonia)." << std::endl <<
    "normal(r4) v d1(r4) v d2(r4)." << std::endl <<
    "b4(need_strong) :- d2(r4)." << std::endl <<
    "b4(need_strong) :- not d1(r4), a3(atyppneumonia)." << std::endl <<
    "o1(allergy_strong_ab)." << std::endl <<
    "normal(r5) v d1(r5) v d2(r5)." << std::endl <<
    "b4(allow_strong_ab) :- d2(r5)." << std::endl <<
    "b4(allow_strong_ab) :- not d1(r5), na1(allergy_strong_ab)." << std::endl <<
    "a1(X) v na1(X) :- o1(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[1,a1,b1,o1,\"./medExample/kb1.dlv\"]()." << std::endl <<
    "ctx(1)." << std::endl <<
    "a2(X) v na2(X) :- o2(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[2,a2,b2,o2,\"./medExample/kb2.dlv\"]()." << std::endl <<
    "ctx(2)." << std::endl <<
    "a3(X) v na3(X) :- o3(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[3,a3,b3,o3,\"./medExample/kb3.dlv\"]()." << std::endl <<
    "ctx(3)." << std::endl <<
    "a4(X) v na4(X) :- o4(X)." << std::endl <<
    ":- not &dlv_asp_context_acc[4,a4,b4,o4,\"./medExample/kb4.dlv\"]()." << std::endl <<
    "ctx(4)." << std::endl;
  program = ss.str();
  HexParser parser(ctx);
  parser.parse(ss);

  //TODO this should become a common functionality using some pluginAtom registry
	{
		ExternalAtomTable::PredicateIterator it, it_end;
		for(boost::tie(it, it_end) = ctx.registry()->eatoms.getRangeByPredicateID(idAspCtxAcc);
				it != it_end; ++it)
		{
			ExternalAtom ea(*it);
			ea.pluginAtom = papAspCtxAcc;
			ctx.registry()->eatoms.update(*it, ea);
		}
	}
}

#endif // FIXTURES_MCS_HPP_INCLUDED__08112010
