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
 * @file ASPSolver.cpp
 * @author Thomas Krennwallner
 * @date Tue Jun 16 14:34:00 CEST 2009
 *
 * @brief ASP Solvers
 *
 *
 */

// @todo: this is required in dlv.h from libdlv s.t. UINT8_MAX is defined, this is ugly, perhaps UINT8_MAX can be replaced by something more portable?
#define __STDC_LIMIT_MACROS
#include "dlvhex/ASPSolver.h"

// activate benchmarking if activated by configure option --enable-debug
#ifdef HAVE_CONFIG_H
#  include "config.h"
#  ifdef DLVHEX_DEBUG
#    define DLVHEX_BENCHMARK
#  endif
#endif

#if HAVE_LIBDLV
# include <dlv.h>
#endif

#include "dlvhex/DLVresultParserDriver.h"
#include "dlvhex/Benchmarking.h"
#include "dlvhex/DLVProcess.h"
#include "dlvhex/PrintVisitor.h"
#include "dlvhex/Program.h"
#include "dlvhex/globals.h"
#include "dlvhex/AtomSet.h"

#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp> // seems to be required for scope_exit
#include <boost/foreach.hpp>
#include <cassert>

DLVHEX_NAMESPACE_BEGIN

namespace ASPSolver
{

DLVOptions::DLVOptions():
  ASPSolverManager::GenericOptions(),
  rewriteHigherOrder(false),
  dropPredicates(false),
  arguments()
{
  arguments.push_back("-silent");
}

DLVOptions::~DLVOptions()
{
}

DLVSoftware::Delegate::Delegate(const Options& options):
  options(options),
  proc()
{
}

DLVSoftware::Delegate::~Delegate()
{
  int retcode = proc.close();

  // check for errors
  if (retcode == 127)
  {
    throw FatalError("LP solver command `" + proc.path() + "´ not found!");
  }
  else if (retcode != 0) // other problem
  {
    std::stringstream errstr;

    errstr << "LP solver `" << proc.path() << "´ bailed out with exitcode " << retcode << ": "
	   << "re-run dlvhex with `strace -f´.";

    throw FatalError(errstr.str());
  }
}

void DLVSoftware::Delegate::setupProcess()
{
  proc.setPath(DLVPATH);
  if( options.includeFacts )
    proc.addOption("-facts");
  else
    proc.addOption("-nofacts");
  BOOST_FOREACH(const std::string& arg, options.arguments)
  {
    proc.addOption(arg);
  }
}

#define CATCH_RETHROW_DLVDELEGATE \
  catch(const GeneralError& e) { \
    std::stringstream errstr; \
    int retcode = proc.close(); \
    errstr << proc.path() << " (exitcode = " << retcode << "): " + e.getErrorMsg(); \
    throw FatalError(errstr.str()); \
  } \
  catch(const std::exception& e) \
  { \
    throw FatalError(proc.path() + ": " + e.what()); \
  }

void
DLVSoftware::Delegate::useASTInput(const Program& idb, const AtomSet& edb)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVSoftware::Delegate::useASTInput");

  if( idb.isHigherOrder() && !options.rewriteHigherOrder )
    throw SyntaxError("Higher Order Constructions cannot be solved with DLVSoftware without rewriting");

  // in higher-order mode we cannot have aggregates, because then they would
  // almost certainly be recursive, because of our atom-rewriting!
  if( idb.isHigherOrder() && idb.hasAggregateAtoms() )
    throw SyntaxError("Aggregates and Higher Order Constructions must not be used together");

  try
  {
    setupProcess();
    // request stdin as last parameter
    proc.addOption("--");
    // fork dlv process
    proc.spawn();

    typedef boost::shared_ptr<DLVPrintVisitor> PrinterPtr;
    std::ostream& programStream = proc.getOutput();

    ///@todo: this is marked as "temporary hack" in globals.h -> move this info into ProgramCtx and allow ProgramCtx to contribute to the solving process
    if( !Globals::Instance()->maxint.empty() )
      programStream << Globals::Instance()->maxint << std::endl;

    // output program
    PrinterPtr printer;
    if( options.rewriteHigherOrder )
      printer = PrinterPtr(new HOPrintVisitor(programStream));
    else
      printer = PrinterPtr(new DLVPrintVisitor(programStream));
    idb.accept(*printer);
    edb.accept(*printer);

    proc.endoffile();
  }
  CATCH_RETHROW_DLVDELEGATE
}

void
DLVSoftware::Delegate::useStringInput(const std::string& program)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVSoftware::Delegate::useStringInput");

  try
  {
    setupProcess();
    // request stdin as last parameter
    proc.addOption("--");
    // fork dlv process
    proc.spawn();
    proc.getOutput() << program << std::endl;
    proc.endoffile();
  }
  CATCH_RETHROW_DLVDELEGATE
}

void
DLVSoftware::Delegate::useFileInput(const std::string& fileName)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVSoftware::Delegate::useFileInput");

  try
  {
    setupProcess();
    proc.addOption(fileName);
    // fork dlv process
    proc.spawn();
    proc.endoffile();
  }
  CATCH_RETHROW_DLVDELEGATE
}

void
DLVSoftware::Delegate::getOutput(std::vector<AtomSet>& result)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVSoftware::Delegate::getOutput");

  try
  {
    // parse result
    DLVresultParserDriver parser(
      options.dropPredicates?(DLVresultParserDriver::HO):(DLVresultParserDriver::FirstOrder));
    parser.parse(proc.getInput(), result);
  }
  CATCH_RETHROW_DLVDELEGATE
}


#if defined(HAVE_LIBDLV)
// pimpl idiom
struct DLVLibSoftware::Delegate::DelegateImpl
{
  /*
  class MObserver: public MODEL_Observer
  {
  public:
    void handleResult( const MODEL &m )
    {
      std::cerr << "TODO MObserver::handleResult(M): " << m << std::endl;
    }
    
    void handleResult( const bool b )
    {
      std::cerr << "TODO MObserver::handleResult(b): " << b << std::endl;
    }
  };

  class MAObserver : public MODEL_ATOM_Observer
  {
  public:
    void handleResult( const MODEL_ATOM &m )
    {
      std::cerr << "TODO MAObserver::handleResult(MA): " << m << std::endl;
    }
    
    void handleResult( const bool b )
    {
      std::cerr << "TODO MObserver::handleResult(b): " << b << std::endl;
    }
  };
  */

  const Options& options;
  PROGRAM_HANDLER *ph;
  //MObserver mo;
  //MAObserver mao;

  DelegateImpl(const Options& options):
    options(options),
    ph(create_program_handler()) //,
    //mo(),
    //mao()
  {
    //ph->subscribe(mo);
    //ph->subscribe(mao);
    if( options.includeFacts )
      ph->setOption(INCLUDE_FACTS, 1);
    else
      ph->setOption(INCLUDE_FACTS, 0);
    BOOST_FOREACH(const std::string& arg, options.arguments)
    {
      if( arg == "-silent" )
      {
	// do nothing?
      }
      else
	throw std::runtime_error("dlv-lib commandline option not implemented: " + arg);
    }
  }

  ~DelegateImpl()
  {
    destroy_program_handler(ph);
  }
};

DLVLibSoftware::Delegate::Delegate(const Options& opt):
  options(opt),
  pimpl(new DelegateImpl(options))
{
}

DLVLibSoftware::Delegate::~Delegate()
{
  delete pimpl;
}

void
DLVLibSoftware::Delegate::useASTInput(const Program& idb, const AtomSet& edb)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVLibSoftware::Delegate::useASTInput");

  if( idb.isHigherOrder() && !options.rewriteHigherOrder )
    throw SyntaxError("Higher Order Constructions cannot be solved with DLVSoftware without rewriting");

  // in higher-order mode we cannot have aggregates, because then they would
  // almost certainly be recursive, because of our atom-rewriting!
  if( idb.isHigherOrder() && idb.hasAggregateAtoms() )
    throw SyntaxError("Aggregates and Higher Order Constructions must not be used together");

  std::stringstream programStream;

  ///@todo: this is marked as "temporary hack" in globals.h -> move this info into ProgramCtx and allow ProgramCtx to contribute to the solving process
  if( !Globals::Instance()->maxint.empty() )
  {
    programStream << Globals::Instance()->maxint << std::endl;
    /*
    std::istringstream iss(Globals::Instance()->maxint);
    int mi;
    iss >> mi;
    std::cerr << "setting maxint '" << Globals::Instance()->maxint << "' to " << mi << std::endl;
    pimpl->ph->setMaxInt(mi);
    */
  }
  else
  {
    pimpl->ph->setMaxInt(10);
  }

  // output program
  typedef boost::shared_ptr<DLVPrintVisitor> PrinterPtr;
  PrinterPtr printer;
  if( options.rewriteHigherOrder )
    printer = PrinterPtr(new HOPrintVisitor(programStream));
  else
    printer = PrinterPtr(new DLVPrintVisitor(programStream));
  idb.accept(*printer);
  edb.accept(*printer);

  pimpl->ph->clearProgram();
  std::cerr <<
    "sending program to dlv-lib:===" << std::endl <<
    programStream.str() << std::endl <<
    "==============================" << std::endl;
  pimpl->ph->Parse(programStream);
  pimpl->ph->ResolveProgram(SYNCRONOUSLY);
}

void
DLVLibSoftware::Delegate::useStringInput(const std::string& program)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVLibSoftware::Delegate::useStringInput");

  std::stringstream prog(program);

  pimpl->ph->clearProgram();
  pimpl->ph->Parse(prog);
  pimpl->ph->ResolveProgram(SYNCRONOUSLY);
}

void
DLVLibSoftware::Delegate::useFileInput(const std::string& fileName)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVLibSoftware::Delegate::useFileInput");

  pimpl->ph->clearProgram();
  pimpl->ph->ParseFile(fileName.c_str());
  pimpl->ph->ResolveProgram(SYNCRONOUSLY);
}

void
DLVLibSoftware::Delegate::getOutput(std::vector<AtomSet>& result)
{
  DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sid,"DLVLibSoftware::Delegate::getOutput");

  const std::vector<MODEL> *models = pimpl->ph->getAllModels();

  assert(result.empty()); // at the moment we implement this only for adding to empty vector (easily possible to extend)

  // create empty atom set for each model
  result.resize(models->size());

  std::vector<MODEL>::const_iterator itm;
  std::vector<AtomSet>::iterator itr;
  // iterate over models
  for(itm = models->begin(), itr = result.begin();
      itm != models->end() && itr != result.end(); ++itm, ++itr)
  {
    // iterate over atoms
    for(MODEL_ATOMS::const_iterator ita = itm->begin();
	ita != itm->end(); ++ita)
    {
      const char* pname = ita->getName();
      assert(pname);

      // i have a hunch this might be the encoding
      assert(pname[0] != '-');

      //PTuple = std::vector<dlvhex::Term*>
      dlvhex::PTuple ptuple;
      if( options.rewriteHigherOrder == false )
      {
	//std::cerr << "creating predicate with first term '" << pname << "'" << std::endl;
	ptuple.push_back(new dlvhex::Term(pname));
      }
      for(MODEL_TERMS::const_iterator itt = ita->getParams().begin();
	  itt != ita->getParams().end(); ++itt)
      {
	switch(itt->type)
	{
	case 1:
	  // string terms
	  //std::cerr << "creating string term '" << itt->data.item << "'" << std::endl;
	  ptuple.push_back(new dlvhex::Term(itt->data.item));
	  break;
	case 2:
	  // int terms
	  //std::cerr << "creating int term '" << itt->data.number << "'" << std::endl;
	  ptuple.push_back(new dlvhex::Term(itt->data.number));
	  break;
	default:
	  throw std::runtime_error("unknown term type!");
	}
      }
      itr->insert(dlvhex::AtomPtr(new dlvhex::Atom(ptuple)));
    }
  }
  assert(itm == models->end() && itr == result.end());

  // TODO: is this necessary?
  // delete models;
 
}
#endif

#if defined(HAVE_DLVDB)
DLVDBSoftware::Options::Options():
  DLVSoftware::Options(),
  typFile()
{
}

DLVDBSoftware::Options::~Options()
{
}

DLVDBSoftware::Delegate::Delegate(const Options& opt):
  DLVSoftware::Delegate(opt),
  options(opt)
{
}

DLVDBSoftware::Delegate::~Delegate()
{
}

void DLVDBSoftware::Delegate::setupProcess()
{
  DLVSoftware::Delegate::setupProcess();

  proc.setPath(DLVDBPATH);
  proc.addOption("-DBSupport"); // turn on database support
  proc.addOption("-ORdr-"); // turn on rewriting of false body rules
  if( !options.typFile.empty() )
    proc.addOption(options.typFile);

}

#endif // defined(HAVE_DLVDB)

} // namespace ASPSolver

DLVHEX_NAMESPACE_END

/* vim: set noet sw=2 ts=8 tw=80: */
// Local Variables:
// mode: C++
// End:
