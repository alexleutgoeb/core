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
 * @file   Atoms.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Storage classes for atoms: Atom, OrdinaryAtom, BuiltinAtom, AggregateAtom, ExternalAtom.
 */

#ifndef ATOMS_HPP_INCLUDED__14102010
#define ATOMS_HPP_INCLUDED__14102010

#include "dlvhex/PlatformDefinitions.h"
#include "dlvhex/fwd.hpp"
#include "dlvhex/Logger.hpp"
#include "dlvhex/ID.hpp"
#include "dlvhex/PredicateMask.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>
#include <list>

DLVHEX_NAMESPACE_BEGIN

struct Atom
{
  // the kind part of the ID of this atom
  IDKind kind;

  // the ID representation of the main tuple of this atom
  // (for builtin and ordinary atoms, the main tuple is the only content)
  // (aggregate atoms add an "inner tuple" for the aggregate conditions)
  // (external atoms add an "input tuple" for the inputs)
  Tuple tuple;

  // used for indices
  ID front() const { return tuple.front(); }

protected:
  // atom should not be used directly, so no public constructor
  Atom(IDKind kind):
    kind(kind), tuple()
    { assert(ID(kind,0).isAtom()); }
  Atom(IDKind kind, const Tuple& tuple):
    kind(kind), tuple(tuple)
    { assert(ID(kind,0).isAtom()); assert(!tuple.empty()); }
};

// regarding strong negation:
// during the parse process we do the following:
// we convert strong negation -<foo> into <foo'> (careful with variables in <foo>!)
// we add constraint :- <foo>, <foo'>.
// we somehow mark the <foo'> as strongly negated helper s.t. output can correctly print results
//
// for the first implementation, we leave out strong negation alltogether (not parseable)
struct OrdinaryAtom:
  public Atom,
  private ostream_printable<OrdinaryAtom>
{
  // the textual representation of the whole thing
  // this is stored for efficient parsing and printing
  // @todo make this a template parameter of OrdinaryAtom, so that we can store various "efficient" representations here (depending on the solver dlvhex should work with; e.g., we could store clasp- or dlv-library internal atom representations here and index them) if we don't need it, we can replace it by an empty struct and conserve space
  // TODO if we get answer sets in a structured way we do not need to parse them anymore
  // TODO if we only need this for printing, we should generate it on-demand and save a lot of effort
  std::string text;

  bool unifiesWith(const OrdinaryAtom& a) const;

  OrdinaryAtom(IDKind kind):
    Atom(kind), text()
    { assert(ID(kind,0).isOrdinaryAtom()); }
  OrdinaryAtom(IDKind kind, const std::string& text):
    Atom(kind), text(text)
    { assert(ID(kind,0).isOrdinaryAtom()); assert(!text.empty()); }
  OrdinaryAtom(IDKind kind, const std::string& text, const Tuple& tuple):
    Atom(kind, tuple), text(text)
    { assert(ID(kind,0).isOrdinaryAtom());
      assert(!text.empty()); }
  std::ostream& print(std::ostream& o) const
    { return o << "OrdinaryAtom(" << std::hex << kind << std::dec << ",'" << text << "'," << printvector(tuple) << ")"; }
};

struct BuiltinAtom:
  public Atom,
  private ostream_printable<BuiltinAtom>
{
  // for ternary builtins of the form (A = B * C) tuple contains
  // in this order: <*, B, C, A>
  BuiltinAtom(IDKind kind):
    Atom(kind)
    { assert(ID(kind,0).isBuiltinAtom()); }
  BuiltinAtom(IDKind kind, const Tuple& tuple):
    Atom(kind, tuple)
    { assert(ID(kind,0).isBuiltinAtom()); }
  std::ostream& print(std::ostream& o) const
    { return o << "BuiltinAtom(" << printvector(tuple) << ")"; }
};

struct AggregateAtom:
  public Atom,
  private ostream_printable<AggregateAtom>
{
  // Atom::tuple is used for outer conditions (always contains 5 elements):
  // tuple[0] = left term or ID_FAIL
  // tuple[1] = left comparator or ID_FAIL
  // tuple[2] = aggregation function
  // tuple[3] = right comparator or ID_FAIL
  // tuple[4] = right term or ID_FAIL

  // variables of the symbolic set
  Tuple variables;
  // atoms in conjunction of the symbolic set
  Tuple atoms;

  AggregateAtom(IDKind kind):
    Atom(kind, Tuple(5, ID_FAIL)), 
    variables(), atoms()
    { assert(ID(kind,0).isAggregateAtom()); }
  AggregateAtom(IDKind kind,
      const Tuple& tuple, const Tuple& variables, const Tuple& atoms):
    Atom(kind, tuple), variables(variables), atoms(atoms)
    { assert(ID(kind,0).isAggregateAtom()); assert(tuple.size() == 5);
      assert(!variables.empty()); assert(!atoms.empty()); }
  std::ostream& print(std::ostream& o) const
    { return o << "AggregateAtom(" << printvector(tuple) << " with vars " <<
        printvector(variables) << " and atoms " << printvector(atoms) << ")"; }
};

// this is one concrete atom in one rule
// the general external atom functionality provided by the user is "PluginAtom"
struct ExternalAtom:
  public Atom,
  private ostream_printable<ExternalAtom>
{
  // &<predicate>[<inputs>](<outputs>)

  // external atom name (constant term)
  ID predicate;

  // input terms
  Tuple inputs;

  // Atom::tuple is used for output terms

	// POD-style pointer to plugin atom
  // (cannot be indexed in multi_index_container as it is mutable)
  // this is a POD-style pointer as the target object is dynamically loaded
  // shared library code, which cannot be weak_ptr- or shared_ptr-managed.
	mutable PluginAtom* pluginAtom;

  // auxiliary input predicate for this occurance in this rule, ID_FAIL if no input here
  ID auxInputPredicate;
  // this mapping stores for each argument of auxInputPredicate
  // a list of positions in the input tuple where this argument applies
  // e.g., for &foo[a,C,d,X,C]() we have aux(C,X) and inputs <a,C,d,X,C>
  // then we have mapping < [1,4], [3] >:
  // for index 0 = argument C we have to set index 1 and 4 in inputs
  // for index 1 = argument X we have to set index 3 in inputs
  typedef std::vector<std::list<unsigned> > AuxInputMapping;
  AuxInputMapping auxInputMapping;

  // auxiliary replacement predicate name is stored in pluginAtom!

  // inputMask stores a bitmask to project interpretations to relevant predicate inputs
  //
  // kind of a cache: interpretation with all ground atoms set that must be passed to
  // the pluginAtom for subsequent calls this must be extended (new values may have
  // been invented), but this extension need only look to the bits not yet covered by
  // predicateInputMask
  //
  // updatePredicateInputMask may update this while this object is stored in an
  // ExternalAtomTable (where only const refs can be retrieved) we should be fine "as
  // long as we don't use predicateInputMask in an index of the
  // multi_index_container"
  //
  mutable PredicateMask inputMask;

public:
  ExternalAtom(IDKind kind, ID predicate, const Tuple& inputs, const Tuple& outputs):
    Atom(kind, outputs),
    predicate(predicate),
    inputs(inputs),
    pluginAtom(),
    auxInputPredicate(ID_FAIL),
    inputMask()
    { assert(ID(kind,0).isExternalAtom()); assert(predicate.isConstantTerm()); }
  ExternalAtom(IDKind kind):
    Atom(kind),
    predicate(ID_FAIL),
    inputs(),
    pluginAtom(),
    auxInputPredicate(ID_FAIL),
    inputMask()
    { assert(ID(kind,0).isExternalAtom()); }
  ~ExternalAtom();

  std::ostream& print(std::ostream& o) const;

  // updates inputMask (creates mask with registry if it does not exist)
  // needs a non-expired pluginAtom pointer (this is only asserted)
  // uses pluginAtom pointer to get the registry
  // we make this const so that we can call it on eatoms in ExternalAtomTable
  void updatePredicateInputMask() const;
  InterpretationConstPtr getPredicateInputMask() const
    { return inputMask.mask(); }
};

DLVHEX_NAMESPACE_END

#endif // ATOMS_HPP_INCLUDED__14102010
