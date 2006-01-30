/* -*- C++ -*- */

/**
 * @file   dlvhex.cpp
 * @author Roman Schindlauer
 * @date   Tue Nov 15 17:29:45 CET 2005
 * 
 * @brief  Definition of ASP solver class.
 * 
 */

#include "dlvhex/ASPsolver.h"
#include "dlvhex/errorHandling.h"
#include "dlvhex/helper.h"

#include "../config.h"


namespace solverResult
{
    std::vector<GAtomSet> answersets;

    unsigned returncode;

    std::string message;

    GAtomSet*
    createNewAnswerset()
    {
        solverResult::answersets.push_back(GAtomSet());

        return &(solverResult::answersets.back());
    }

    void
    addMessage(std::string msg)
    {
        message += msg + '\n';
    }
}



ASPsolver::ASPsolver()
    : lpcommand(DLVPATH)
{
    solverResult::answersets.clear();
    
    answerSetIndex = solverResult::answersets.end();
}


GAtomSet*
ASPsolver::getNextAnswerSet()
{
    if (answerSetIndex != solverResult::answersets.end())
        return &(*(answerSetIndex++));

    return NULL;
}


unsigned
ASPsolver::numAnswerSets()
{
    return solverResult::answersets.size();
}



//
// Where LEX reads its input from:
//
extern "C" FILE * dlvresultin;

extern int dlvresultparse();

extern bool dlvresultdebug;


void
ASPsolver::callSolver(std::string prg)
{
    solverResult::answersets.clear();
    solverResult::message = "";
    
    
    //std::cout << prg << std::endl << std::endl;
    

    //char tempfile[] = "/tmp/dlvXXXXXX";
    char tempfile[L_tmpnam];

    std::string execdlv;

    //
    // if-branch: using a temporary file for the asp-program
    // else-branch: passing the program to dlv via echo and pipe
    //
    if (1)
    {
        tmpnam(tempfile);

        FILE* dlvinput = fopen(tempfile, "w");
//        FILE* dlvinput = mkstemp(tempfile);

        if (dlvinput == NULL)
            throw FatalError("LP solver temp-file " + (std::string)tempfile + " could not be created!");

        fputs(prg.c_str(), dlvinput);
        fflush(dlvinput);
        fclose(dlvinput);

        execdlv = lpcommand + " -nofacts " + (std::string)tempfile + " 2>&1; echo $?";
    }
    else
    {
        //
        // escape quotes for shell command execution with echo!
        //
        helper::escapeQuotes(prg);
        
        execdlv = "echo \"" + prg + "\" | " + lpcommand + " -nofacts -- 2>&1; echo $?";
    }


    dlvresultin = popen(execdlv.c_str(), "r");

    dlvresultparse ();

    fclose(dlvresultin);

    //unlink(tempfile);

    //std::cout << solverResult::returncode << std::endl;
    
    if (solverResult::returncode == 127)
    {
        throw FatalError("LP solver command not found!");
    }
    
    //
    // other problem:
    //
    if (solverResult::returncode != 0)
    {
        throw FatalError("LP solver aborted due to program errors!");
    }
    
    // TODO: what to do with solverResult::message?

    //
    // reset retrieval pointer:
    //

    answerSetIndex = solverResult::answersets.begin();

    /*
    for (std::vector<GAtomSet>::iterator o = solverResult::answersets.begin();
            o != solverResult::answersets.end();
            o++)
        cout << "as: " << *o << endl;
        */
}

