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
 * @file GenuineGuessAndCheckModelGeneratorAsync.cpp
 * @author Christoph Redl
 *
 * @brief Implementation of the model generator for "GuessAndCheck" components.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/GenuineGuessAndCheckModelGeneratorAsync.h"
#include "dlvhex2/Logger.h"
#include "dlvhex2/Registry.h"
#include "dlvhex2/Printer.h"
#include "dlvhex2/ASPSolver.h"
#include "dlvhex2/ASPSolverManager.h"
#include "dlvhex2/ProgramCtx.h"
#include "dlvhex2/PluginInterface.h"
#include "dlvhex2/Benchmarking.h"
#include "dlvhex2/InternalGroundDASPSolver.h"
#include "dlvhex2/UnfoundedSetChecker.h"

#include <bm/bmalgo.h>

#include <boost/foreach.hpp>

DLVHEX_NAMESPACE_BEGIN

GenuineGuessAndCheckModelGeneratorAsyncFactory::GenuineGuessAndCheckModelGeneratorAsyncFactory(
    ProgramCtx& ctx,
    const ComponentInfo& ci,
    ASPSolverManager::SoftwareConfigurationPtr externalEvalConfig):
  FLPModelGeneratorFactoryBase(ctx.registry()),
  externalEvalConfig(externalEvalConfig),
  ctx(ctx),
  ci(ci),
  outerEatoms(ci.outerEatoms)
{
  // this model generator can handle any components
  // (and there is quite some room for more optimization)

  // create program for domain exploration
  if (ctx.config.getOption("AutoStrongSafety")){
    std::vector<ID> deidb;
    deidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    deidb.insert(deidb.end(), ci.innerRules.begin(), ci.innerRules.end());
    deidb.insert(deidb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());
    createDomainExplorationProgram(ci, reg, deidb);

    // add domain predicates
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    std::back_insert_iterator<std::vector<ID> > dinserter(idb);
    std::transform(ci.innerRules.begin(), ci.innerRules.end(),
        dinserter, boost::bind(&GenuineGuessAndCheckModelGeneratorAsyncFactory::addDomainPredicatesWhereNecessary, this, ci, reg, _1));
    std::transform(ci.innerConstraints.begin(), ci.innerConstraints.end(),
        dinserter, boost::bind(&GenuineGuessAndCheckModelGeneratorAsyncFactory::addDomainPredicatesWhereNecessary, this, ci, reg, _1));
  }else{
    // copy rules and constraints to idb
    // TODO we do not really need this except for debugging (tiny optimization possibility)
    idb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
    idb.insert(idb.end(), ci.innerRules.begin(), ci.innerRules.end());
    idb.insert(idb.end(), ci.innerConstraints.begin(), ci.innerConstraints.end());
  }

  innerEatoms = ci.innerEatoms;
  // create guessing rules "gidb" for innerEatoms in all inner rules and constraints
  createEatomGuessingRules();

  // transform original innerRules and innerConstraints to xidb with only auxiliaries
  xidb.reserve(ci.innerRules.size() + ci.innerConstraints.size());
  std::back_insert_iterator<std::vector<ID> > inserter(xidb);
  std::transform(idb.begin(), idb.end(),
      inserter, boost::bind(&GenuineGuessAndCheckModelGeneratorAsyncFactory::convertRule, this, reg, _1));

  // transform xidb for flp calculation
  if (ctx.config.getOption("FLPCheck")) createFLPRules();

  // output rules
  {
    std::ostringstream s;
    print(s, true);
    LOG(DBG,"GenuineGuessAndCheckModelGeneratorAsyncFactory(): " << s.str());
  }
}

GenuineGuessAndCheckModelGeneratorAsyncFactory::ModelGeneratorPtr
GenuineGuessAndCheckModelGeneratorAsyncFactory::createModelGenerator(
    InterpretationConstPtr input)
{ 
  return ModelGeneratorPtr(new GenuineGuessAndCheckModelGeneratorAsync(*this, input));
}

std::ostream& GenuineGuessAndCheckModelGeneratorAsyncFactory::print(
    std::ostream& o) const
{
  return print(o, false);
}

std::ostream& GenuineGuessAndCheckModelGeneratorAsyncFactory::print(
    std::ostream& o, bool verbose) const
{
  // item separator
  std::string isep(" ");
  // group separator
  std::string gsep(" ");
  if( verbose )
  {
    isep = "\n";
    gsep = "\n";
  }
  RawPrinter printer(o, ctx.registry());
  if( !outerEatoms.empty() )
  {
    o << "outer Eatoms={" << gsep;
    printer.printmany(outerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !innerEatoms.empty() )
  {
    o << "inner Eatoms={" << gsep;
    printer.printmany(innerEatoms,isep);
    o << gsep << "}" << gsep;
  }
  if( !gidb.empty() )
  {
    o << "gidb={" << gsep;
    printer.printmany(gidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !idb.empty() )
  {
    o << "idb={" << gsep;
    printer.printmany(idb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidb.empty() )
  {
    o << "xidb={" << gsep;
    printer.printmany(xidb,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflphead.empty() )
  {
    o << "xidbflphead={" << gsep;
    printer.printmany(xidbflphead,isep);
    o << gsep << "}" << gsep;
  }
  if( !xidbflpbody.empty() )
  {
    o << "xidbflpbody={" << gsep;
    printer.printmany(xidbflpbody,isep);
    o << gsep << "}" << gsep;
  }
  return o;
}

//
// the model generator
//

GenuineGuessAndCheckModelGeneratorAsync::GenuineGuessAndCheckModelGeneratorAsync(
    Factory& factory,
    InterpretationConstPtr input):
  FLPModelGeneratorBase(factory, input),
  factory(factory),
  reg(factory.reg)
{
    DBGLOG(DBG, "Genuine GnC-ModelGenerator is instantiated for a " << (factory.ci.disjunctiveHeads ? "" : "non-") << "disjunctive component");

    RegistryPtr reg = factory.reg;

    // create new interpretation as copy
    InterpretationPtr postprocInput;
    if( input == 0 )
    {
      // empty construction
      postprocInput.reset(new Interpretation(reg));
    }
    else
    {
      // copy construction
      postprocInput.reset(new Interpretation(*input));
    }

    // augment input with edb
    #warning perhaps we can pass multiple partially preprocessed input edb's to the external solver and save a lot of processing here
    postprocInput->add(*factory.ctx.edb);

    // remember which facts we must remove
    mask.reset(new Interpretation(*postprocInput));

    // manage outer external atoms
    if( !factory.outerEatoms.empty() )
    {
      // augment input with result of external atom evaluation
      // use newint as input and as output interpretation
      IntegrateExternalAnswerIntoInterpretationCB cb(postprocInput);
      evaluateExternalAtoms(factory.ctx,
          factory.outerEatoms, postprocInput, cb);
      DLVHEX_BENCHMARK_REGISTER(sidcountexternalatomcomps,
          "outer external atom computations");
      DLVHEX_BENCHMARK_COUNT(sidcountexternalatomcomps,1);

      assert(!factory.xidb.empty() &&
          "the guess and check model generator is not required for "
          "non-idb components! (use plain)");
    }

    // compute extensions of domain predicates and add it to the input
    if (factory.ctx.config.getOption("AutoStrongSafety")){
      InterpretationConstPtr domPredictaesExtension = computeExtensionOfDomainPredicates<GenuineSolver>(factory.ctx, postprocInput);
      postprocInput->add(*domPredictaesExtension);
    }

    // assign to const member -> this value must stay the same from here on!
    postprocessedInput = postprocInput;

    // evaluate edb+xidb+gidb
    {
	DBGLOG(DBG,"evaluating guessing program");
	// no mask
	OrdinaryASPProgram program(reg, factory.xidb, postprocessedInput, factory.ctx.maxint);
	// append gidb to xidb
	program.idb.insert(program.idb.end(), factory.gidb.begin(), factory.gidb.end());

	grounder = GenuineGrounder::getInstance(factory.ctx, program);
        annotatedGroundProgram = AnnotatedGroundProgram(reg, grounder->getGroundProgram(), factory.innerEatoms);
	solver = GenuineGroundSolver::getInstance(
						factory.ctx, annotatedGroundProgram,
						false,
						!factory.ctx.config.getOption("FLPCheck") && !factory.ctx.config.getOption("UFSCheck")	// do the UFS check for disjunctions only if we don't do
																	// a minimality check in this class;
																	// this will not find unfounded sets due to external sources,
																	// but at least unfounded sets due to disjunctions
						);
	learnedEANogoods = SimpleNogoodContainerPtr(new SimpleNogoodContainer());
	learnedEANogoodsTransferredIndex = 0;
	nogoodGrounder = NogoodGrounderPtr(new ImmediateNogoodGrounder(factory.ctx.registry(), learnedEANogoods, learnedEANogoods, annotatedGroundProgram));

	solver->addPropagator(this);
    }

    setHeuristics();

    // initialize UFS checker
    //   Concerning the last parameter, note that clasp backend uses choice rules for implementing disjunctions:
    //   this must be regarded in UFS checking (see examples/trickyufs.hex)
    ufscm = UnfoundedSetCheckerManagerPtr(new UnfoundedSetCheckerManager(*this, factory.ctx, annotatedGroundProgram, factory.ctx.config.getOption("GenuineSolver") >= 3));

    // start producing models
    modelProducer = new boost::thread(boost::bind(&GenuineGuessAndCheckModelGeneratorAsync::produceOrdinaryModels, this));
    modelVerifier = new boost::thread(boost::bind(&GenuineGuessAndCheckModelGeneratorAsync::verifyModels, this));
    destruct = false;
}

GenuineGuessAndCheckModelGeneratorAsync::~GenuineGuessAndCheckModelGeneratorAsync(){

	// stop producing models
	destruct = true;
	modelProducer->join();
	modelVerifier->join();

	solver->removePropagator(this);
	DBGLOG(DBG, "Final Statistics:" << std::endl << solver->getStatistics());
}

void GenuineGuessAndCheckModelGeneratorAsync::setHeuristics(){

    // set external atom evaluation strategy according to selected heuristics
    for (int i = 0; i < factory.innerEatoms.size(); ++i){
      eaEvaluated.push_back(false);
      eaVerified.push_back(false);

      // watch all atoms in the scope of the external atom for unverification
      bm::bvector<>::enumerator en = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().first();
      bm::bvector<>::enumerator en_end = annotatedGroundProgram.getEAMask(i)->mask()->getStorage().end();
      bool first = true;
      while (en < en_end){
        unverifyWatchList[*en].push_back(i);

        // watch one input atom for verification
        if (first) verifyWatchList[*en].push_back(i);
        first = false;

        en++;
      }
    }
    externalAtomEvalHeuristics = factory.ctx.externalAtomEvaluationHeuristicsFactory->createHeuristics(this, reg);

    // create ufs check heuristics as selected
    ufsCheckHeuristics = factory.ctx.unfoundedSetCheckHeuristicsFactory->createHeuristics(this, reg);
}

InterpretationPtr GenuineGuessAndCheckModelGeneratorAsync::generateNextModel()
{
	DLVHEX_BENCHMARK_REGISTER_AND_SCOPE(sidgcsolve, "genuine guess and check loop");

	InterpretationPtr modelCandidate;
	do
	{
		boost::mutex::scoped_lock lock(verifiedModelsMutex);
		// is there a verified model prepared?
		while(verifiedModels.size() == 0){
			// no: wait
			DBGLOG(DBG, "verifiedModels queue is empty");
			waitForVerifiedModelsCondition.wait(lock);
		}

		assert (verifiedModels.size() > 0);

		// return the next model
		modelCandidate = verifiedModels.front();
		if (modelCandidate) DBGLOG(DBG, "Got a HEX model: " << *modelCandidate);
		verifiedModels.pop();

		return modelCandidate;
	}while(true);
}

void GenuineGuessAndCheckModelGeneratorAsync::produceOrdinaryModels(){

	InterpretationPtr modelCandidate;
	do{
		// let the ordinary ASP solver produce another model
		modelCandidate = solver->getNextModel();

		// more models or end of models?
		if (modelCandidate){
			DBGLOG(DBG, "Got a model of the ordinary ASP program: " << *modelCandidate);
			boost::mutex::scoped_lock omlock(ordinaryModelsMutex);

			// if the model queue is full, wait for more space
			while (ordinaryModels.size() >= factory.ctx.config.getOption("ModelQueueSize")){
				waitForOrdinaryModelsQueueSpaceCondition.wait(omlock);
			}

			// store the new model and notify the verification thread that there is a new model
			ordinaryModels.push(std::pair<InterpretationPtr, std::pair<std::vector<bool>, std::vector<bool> > >(modelCandidate, std::pair<std::vector<bool>, std::vector<bool> >(eaEvaluated, eaVerified)));
			waitForOrdinaryModelsCondition.notify_all();
		}else{
			// end of models: add a "dummy model" (InterpretationPtr()) to indicate end of models and notify the verification thread
			boost::mutex::scoped_lock omlock(ordinaryModelsMutex);
			ordinaryModels.push(std::pair<InterpretationPtr, std::pair<std::vector<bool>, std::vector<bool> > >(InterpretationPtr(), std::pair<std::vector<bool>, std::vector<bool> >(std::vector<bool>(), std::vector<bool>())));
			waitForOrdinaryModelsCondition.notify_all();
			break;
		}
	}while(!destruct);
}

void GenuineGuessAndCheckModelGeneratorAsync::verifyModels(){

	do{
		std::pair<InterpretationPtr, std::pair<std::vector<bool>, std::vector<bool> > > modelCandidate;
		{
			// is there a model waiting for verification?
			boost::mutex::scoped_lock lock(ordinaryModelsMutex);
			while (ordinaryModels.size() == 0){
				// no: wait
				waitForOrdinaryModelsCondition.wait(lock);
			}

			assert (ordinaryModels.size() > 0);
			modelCandidate = ordinaryModels.front();
			ordinaryModels.pop();
		}
		// model was retrieved: notify the model producer thread that there is now space in the queue
		waitForOrdinaryModelsQueueSpaceCondition.notify_all();

		// breakout on end of models
		if (!modelCandidate.first){
			{
				boost::mutex::scoped_lock lock(verifiedModelsMutex);
				verifiedModels.push(modelCandidate.first);	// indicate end of models
			}
			waitForVerifiedModelsCondition.notify_all();	// notify the main thread that there is now a (dummy) model waiting for retrieval
			break;
		}

		DBGLOG_SCOPE(DBG,"gM", false);
		DLVHEX_BENCHMARK_REGISTER_AND_COUNT(ssidmodelcandidates, "Candidate compatible sets", 1);
		DBGLOG(DBG, "Doing final compatibility check for model candidate " << *modelCandidate.first);
		if (!finalCompatibilityCheck(modelCandidate.first, modelCandidate.second.first, modelCandidate.second.second)) continue;

		DBGLOG(DBG, "Doing minimality check for model candidate " << *modelCandidate.first);
		if (!isModel(modelCandidate.first)) continue;

		// remove edb and the guess (from here we don't need the guess anymore)
		modelCandidate.first->getStorage() -= factory.gpMask.mask()->getStorage();
		modelCandidate.first->getStorage() -= factory.gnMask.mask()->getStorage();
		modelCandidate.first->getStorage() -= mask->getStorage();

		DBGLOG(DBG,"= final model " << *modelCandidate.first);

		{
			// add the verified model to the queue and notify the main thread about it
			boost::mutex::scoped_lock lock(verifiedModelsMutex);
			verifiedModels.push(modelCandidate.first);
		}
		waitForVerifiedModelsCondition.notify_all();
	}while(!destruct);
}

void GenuineGuessAndCheckModelGeneratorAsync::generalizeNogood(Nogood ng){

	if (!ng.isGround()) return;

	DBGLOG(DBG, "Generalizing " << ng.getStringRepresentation(reg));

	// find the external atom related to this nogood
	ID eaid = ID_FAIL;
	BOOST_FOREACH (ID l, ng){
		if (reg->ogatoms.getIDByAddress(l.address).isExternalAuxiliary() && annotatedGroundProgram.mapsAux(l.address)){
			eaid = l;
			break;
		}
	}
	if (eaid == ID_FAIL) return;

	assert(annotatedGroundProgram.getAuxToEA(eaid.address).size() > 0);
	DBGLOG(DBG, "External atom is " << annotatedGroundProgram.getAuxToEA(eaid.address)[0]);
	const ExternalAtom& ea = reg->eatoms.getByID(annotatedGroundProgram.getAuxToEA(eaid.address)[0]);

	// learn related nonground nogoods
	int oldCount = learnedEANogoods->getNogoodCount();
	ea.pluginAtom->generalizeNogood(ng, &factory.ctx, learnedEANogoods);
}

void GenuineGuessAndCheckModelGeneratorAsync::generalizeNogoods(){
	int max = learnedEANogoods->getNogoodCount();
	for (int i = learnedEANogoodsTransferredIndex; i < max; ++i){
		generalizeNogood(learnedEANogoods->getNogood(i));
	}
}

void GenuineGuessAndCheckModelGeneratorAsync::transferLearnedEANogoods(){

	for (int i = learnedEANogoodsTransferredIndex; i < learnedEANogoods->getNogoodCount(); ++i){
		DLVHEX_BENCHMARK_REGISTER_AND_COUNT(sidcompatiblesets, "Learned IO-Nogoods", 1);
		if (factory.ctx.config.getOption("PrintLearnedNogoods")){
			if (factory.ctx.config.getOption("GenuineSolver") >= 3){
				if (i == 0) std::cerr << "( NOTE: With clasp backend, learned nogoods become effective with a delay because of multithreading! )" << std::endl << std::endl;
			}else{
				if (i == 0) std::cerr << "( NOTE: With i-backend, learned nogoods become effective AFTER the next model was printed ! )" << std::endl << std::endl;
			}
		}
		if (learnedEANogoods->getNogood(i).isGround()){
			solver->addNogood(learnedEANogoods->getNogood(i));
		}
	}
	// for encoding-based UFS checkers, we need to keep learned nogoods (otherwise future UFS searches will not be able to use them)
	// for assumption-based UFS checkers we can delete them as soon as nogoods were added both to the main search and to the UFS search
	if (factory.ctx.config.getOption("UFSCheckAssumptionBased")){
		// assumption-based
		ufscm->learnNogoodsFromMainSearch();
		learnedEANogoods->clear();
	}else{
		// encoding-based
		learnedEANogoods->forgetLeastFrequentlyAdded();
	}
	learnedEANogoodsTransferredIndex = learnedEANogoods->getNogoodCount();
}

bool GenuineGuessAndCheckModelGeneratorAsync::finalCompatibilityCheck(InterpretationConstPtr modelCandidate, std::vector<bool> eaEvaluated, std::vector<bool> eaVerified){

	// did we already verify during model construction or do we have to do the verification now?
	bool compatible;
	int ngCount;

	// list of started EA evaluation threads together with the index of the inner external atom they evaluate
	std::vector<std::pair<int, boost::thread*> > eaEvalThreads;

	compatible = true;
	for (int eaIndex = 0; eaIndex < factory.innerEatoms.size(); ++eaIndex){
		if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == true){
		}
		if (eaEvaluated[eaIndex] == true && eaVerified[eaIndex] == false){
			DBGLOG(DBG, "External atom " << factory.innerEatoms[eaIndex] << " was evaluated but falsified");
			compatible = false;
			break;
		}
		if (eaEvaluated[eaIndex] == false){
			// try to verify
			DBGLOG(DBG, "External atom " << factory.innerEatoms[eaIndex] << " is not verified, trying to do this now");

			// evaluate the EA in a separate thread
			boost::thread* thread = new boost::thread(boost::bind(&GenuineGuessAndCheckModelGeneratorAsync::finalExternalAtomEvaluation, this, eaIndex, modelCandidate, &eaVerified));
			eaEvalThreads.push_back(std::pair<int, boost::thread*>(eaIndex, thread));
//eaVerified[eaIndex] = !verifyExternalAtom(eaIndex, modelCandidate);

			eaEvaluated[eaIndex] = true;
/*
			if (eaVerified[eaIndex] == false){
				compatible = false;
				break;
			}
*/
		}
	}

	// wait for all threads to termimnate and read their verification result
	for (int i = 0; i < eaEvalThreads.size(); ++i){
		eaEvalThreads[i].second->join();
		if (eaVerified[eaEvalThreads[i].first] == false){
			compatible = false;
		}
	}

	DBGLOG(DBG, "Compatible: " << compatible);

	return compatible;
}

void GenuineGuessAndCheckModelGeneratorAsync::finalExternalAtomEvaluation(int eaIndex, InterpretationConstPtr modelCandidate, std::vector<bool>* eaVerified){
	(*eaVerified)[eaIndex] = !verifyExternalAtom(eaIndex, modelCandidate);
}

bool GenuineGuessAndCheckModelGeneratorAsync::isModel(InterpretationConstPtr compatibleSet){

	// which semantics?
	if (factory.ctx.config.getOption("WellJustified")){
		// well-justified FLP: fixpoint iteration
		InterpretationPtr fixpoint = getFixpoint(factory.ctx, compatibleSet, grounder->getGroundProgram());
		InterpretationPtr reference = InterpretationPtr(new Interpretation(*compatibleSet));
		reference->getStorage() -= factory.gpMask.mask()->getStorage();
		reference->getStorage() -= factory.gnMask.mask()->getStorage();

		DBGLOG(DBG, "Comparing fixpoint " << *fixpoint << " to reference " << *reference);
		if ((fixpoint->getStorage() & reference->getStorage()).count() == reference->getStorage().count()){
			DBGLOG(DBG, "Well-Justified FLP Semantics: Pass fixpoint test");
			return true;
		}else{
			DBGLOG(DBG, "Well-Justified FLP Semantics: Fail fixpoint test");
			return false;
		}
	}else{
		// FLP: ensure minimality of the compatible set wrt. the reduct (if necessary)
		if (annotatedGroundProgram.hasHeadCycles() == 0 && annotatedGroundProgram.hasECycles() == 0){
			DBGLOG(DBG, "No head- or e-cycles --> No FLP/UFS check necessary");
			return true;
		}else{
			DBGLOG(DBG, "Head- or e-cycles --> FLP/UFS check necessary");

			// Explicit FLP check
			if (factory.ctx.config.getOption("FLPCheck")){
				DBGLOG(DBG, "FLP Check");
				// do FLP check (possibly with nogood learning) and add the learned nogoods to the main search
				bool result = isSubsetMinimalFLPModel<GenuineSolver>(compatibleSet, postprocessedInput, factory.ctx, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
				if (factory.ctx.config.getOption("ExternalLearningGeneralize")) generalizeNogoods();
				if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->update(compatibleSet);
				transferLearnedEANogoods();
				return result;
			}

			// UFS check
			if (factory.ctx.config.getOption("UFSCheck")){
				DBGLOG(DBG, "UFS Check");
				std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(compatibleSet, std::set<ID>(), factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
				if (factory.ctx.config.getOption("ExternalLearningGeneralize")) generalizeNogoods();
				if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->update(compatibleSet);
				transferLearnedEANogoods();
				if (ufs.size() > 0){
					DBGLOG(DBG, "Got a UFS");
					if (factory.ctx.config.getOption("UFSLearning")){
						DBGLOG(DBG, "Learn from UFS");
						Nogood ufsng = ufscm->getLastUFSNogood();
						solver->addNogood(ufsng);
					}
					return false;
				}else{
					return true;
				}
			}

			// no check
			return true;
		}
	}
}

bool GenuineGuessAndCheckModelGeneratorAsync::partialUFSCheck(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	if (!factory.ctx.config.getOption("UFSCheck")) return false;

	// ufs check without nogood learning makes no sense if the interpretation is not complete
	if (factory.ctx.config.getOption("UFSLearning")){

		std::pair<bool, std::set<ID> > decision = ufsCheckHeuristics->doUFSCheck(partialInterpretation, factWasSet, changed);

		if (decision.first){

			DBGLOG(DBG, "Heuristic decides to do an UFS check");
			std::vector<IDAddress> ufs = ufscm->getUnfoundedSet(partialInterpretation, decision.second, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : SimpleNogoodContainerPtr());
			DBGLOG(DBG, "UFS result: " << (ufs.size() == 0 ? "no" : "") << " UFS found (interpretation: " << *partialInterpretation << ", assigned: " << *factWasSet << ")");

			if (ufs.size() > 0){
				Nogood ng = ufscm->getLastUFSNogood();
				DBGLOG(DBG, "Adding UFS nogood: " << ng);
				solver->addNogood(ng);

				// check if nogood is violated
				BOOST_FOREACH (ID l, ng){
					if (!factWasSet->getFact(l.address) || l.isNaf() == partialInterpretation->getFact(l.address)) return false;
				}

				return true;
			}
		}else{
			DBGLOG(DBG, "Heuristic decides not to do an UFS check");
		}
	}

	return false;
}

bool GenuineGuessAndCheckModelGeneratorAsync::isVerified(ID eaAux, InterpretationConstPtr factWasSet){

	assert(annotatedGroundProgram.getAuxToEA(eaAux.address).size() > 0);

	// check if at least one of the external atoms which can derive this auxiliary were verified
	BOOST_FOREACH (ID ea, annotatedGroundProgram.getAuxToEA(eaAux.address)){
		int eaIndex = 0;
		while (factory.innerEatoms[eaIndex] != ea) eaIndex++;

		if (eaEvaluated[eaIndex] && eaVerified[eaIndex]){
			DBGLOG(DBG, "Auxiliary " << eaAux.address << " is verified by " << ea);
			return true;
		}
	}
	DBGLOG(DBG, "Auxiliary " << eaAux.address << " is not verified");
	return false;
}

IDAddress GenuineGuessAndCheckModelGeneratorAsync::getWatchedLiteral(int eaIndex, InterpretationConstPtr search, bool truthValue){

	bm::bvector<>::enumerator eaDepAtoms = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().first();
	bm::bvector<>::enumerator eaDepAtoms_end = annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage().end();

	while (eaDepAtoms < eaDepAtoms_end){
		if (search->getFact(*eaDepAtoms) == truthValue){
			DBGLOG(DBG, "Found watch " << *eaDepAtoms << " for atom " << factory.innerEatoms[eaIndex]);
			return *eaDepAtoms;
		}
		eaDepAtoms++;
	}
	assert(false);
}

bool GenuineGuessAndCheckModelGeneratorAsync::verifyExternalAtoms(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	DBGLOG(DBG, "Evaluating External Atoms");

	// for all changed atoms
	bm::bvector<>::enumerator en = changed->getStorage().first();
	bm::bvector<>::enumerator en_end = changed->getStorage().end();

	bool conflict = false;
	while (en < en_end){
		DBGLOG(DBG, "Processing watches for atom " << *en);

		// unverify/unfalsify external atoms which watch this atom
		BOOST_FOREACH (int eaIndex, unverifyWatchList[*en]){
			DBGLOG(DBG, "Checking if " << eaIndex << " is marked as evaluated");
			if (eaEvaluated[eaIndex]){
				DBGLOG(DBG, "Unverifying/Unfalsifying atom " << factory.innerEatoms[eaIndex]);
				// unverify
				eaVerified[eaIndex] = false;
				eaEvaluated[eaIndex] = false;
				if (!factWasSet->getFact(*en)){
					// watch a yet unassigned atom such that the external atom depends on it
					verifyWatchList[getWatchedLiteral(eaIndex, factWasSet, false)].push_back(eaIndex);
				}else{
					// watch a changed atom
					verifyWatchList[getWatchedLiteral(eaIndex, changed, true)].push_back(eaIndex);
				}
			}
		}
		en++;
	}

	en = changed->getStorage().first();
	en_end = changed->getStorage().end();
	while (en < en_end){
		// for all external atoms which watch this atom
		if (factWasSet->getFact(*en)){
			BOOST_FOREACH (int eaIndex, verifyWatchList[*en]){
				if (!eaEvaluated[eaIndex]){
					// check if all atoms relevant to this external atom are assigned
					if (!factWasSet ||
					    ((annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage() & factWasSet->getStorage()).count() == (annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage()).count())){

						// evaluate external atom if the heuristics decides so
						const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
						if (externalAtomEvalHeuristics->doEvaluate(eatom, partialInterpretation, factWasSet, changed)){
							// evaluate it
							eaEvaluated[eaIndex] = true;
							eaVerified[eaIndex] = !verifyExternalAtom(eaIndex, partialInterpretation, factWasSet, changed);
							conflict |= !eaVerified[eaIndex];
						}
					}else{
						// find a new yet unassigned atom to watch
						verifyWatchList[getWatchedLiteral(eaIndex, factWasSet, false)].push_back(eaIndex);
					}
				}

			}
			// current atom was set, so remove all watches
			verifyWatchList[*en].clear();
		}

		en++;
	}

	return conflict;
}

bool GenuineGuessAndCheckModelGeneratorAsync::verifyExternalAtom(int eaIndex, InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	// we need all relevant atoms to be assigned before we can do the verification
	assert(!factWasSet || ((annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage() & factWasSet->getStorage()).count() == (annotatedGroundProgram.getEAMask(eaIndex)->mask()->getStorage() & annotatedGroundProgram.getProgramMask()->getStorage()).count()));

	// prepare EA evaluation
	const ExternalAtom& eatom = reg->eatoms.getByID(factory.innerEatoms[eaIndex]);
	VerifyExternalAtomCB vcb(partialInterpretation, eatom, *(annotatedGroundProgram.getEAMask(eaIndex)));

	// make sure that ALL input auxiliary atoms are true, otherwise we might miss some output atoms and consider true output atoms wrongly as unfounded
	InterpretationPtr evalIntr = InterpretationPtr(new Interpretation(*partialInterpretation));
	BOOST_FOREACH (Tuple t, annotatedGroundProgram.getEAMask(eaIndex)->getAuxInputTuples()){
		OrdinaryAtom oa(ID::MAINKIND_ATOM | ID::SUBKIND_ATOM_ORDINARYG);
		oa.tuple.push_back(eatom.auxInputPredicate);
		oa.tuple.insert(oa.tuple.end(), t.begin(), t.end());
		ID oaid = reg->storeOrdinaryGAtom(oa);
#ifndef NDEBUG
		if (!evalIntr->getFact(oaid.address)){
			DBGLOG(DBG, "Setting aux input " << oaid.address);
		}
#endif
		evalIntr->setFact(oaid.address);
	}

	// evaluate the external atom (and learn nogoods if external learning is used)
	{
		DBGLOG(DBG, "Verifying external Atom " << factory.innerEatoms[eaIndex] << " under " << *evalIntr);
		evaluateExternalAtom(factory.ctx, eatom, evalIntr, vcb, factory.ctx.config.getOption("ExternalLearning") ? learnedEANogoods : NogoodContainerPtr());
	}

	// transfer learned nogoods to solver
	if (factory.ctx.config.getOption("ExternalLearningGeneralize")) generalizeNogoods();
	if (factory.ctx.config.getOption("NongroundNogoodInstantiation")) nogoodGrounder->update(partialInterpretation, factWasSet, changed);
	transferLearnedEANogoods();

	// remember the verification result
	bool verify = vcb.verify();
	DBGLOG(DBG, "Verifying " << factory.innerEatoms[eaIndex] << " (Result: " << verify << ")");

	return !verify;
}

const OrdinaryASPProgram& GenuineGuessAndCheckModelGeneratorAsync::getGroundProgram(){
	return grounder->getGroundProgram();
}

void GenuineGuessAndCheckModelGeneratorAsync::propagate(InterpretationConstPtr partialInterpretation, InterpretationConstPtr factWasSet, InterpretationConstPtr changed){

	bool conflict = verifyExternalAtoms(partialInterpretation, factWasSet, changed);

	// UFS check requires a conflict-free interpretation
	if (conflict) return;

	partialUFSCheck(partialInterpretation, factWasSet, changed);
}

DLVHEX_NAMESPACE_END
