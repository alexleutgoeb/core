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
 * @file   HexGrammar.tcc
 * @author Peter Schüller
 * @date   Wed Jul  8 14:00:48 CEST 2009
 * 
 * @brief  Implementation of HexGrammar.h
 */

/**
 * @file   HexGrammar.tcc
 * @author Peter Schüller
 * 
 * @brief  Grammar for parsing HEX using boost::spirit
 */

#ifndef DLVHEX_HEX_GRAMMAR_TCC_INCLUDED
#define DLVHEX_HEX_GRAMMAR_TCC_INCLUDED

#include "dlvhex/PlatformDefinitions.h"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_stl.hpp>

DLVHEX_NAMESPACE_BEGIN

template<typename Iterator>
HexParserSkipperGrammar<Iterator>::HexParserSkipperGrammar():
  HexParserSkipperGrammar::base_type(start)
{
  using namespace boost::spirit;
  start
    = ascii::space
    | qi::lexeme[ qi::char_('%') > *(qi::char_ - qi::eol) ];

  #ifdef BOOST_SPIRIT_DEBUG
  BOOST_SPIRIT_DEBUG_NODE(start);
  #endif
}

template<typename Iterator, typename Skipper>
HexGrammarBase<Iterator, Skipper>::
HexGrammarBase(HexGrammarSemantics& sem):
  sem(sem)
{
  namespace qi = boost::spirit::qi;
  namespace ascii = boost::spirit::ascii;
  typedef HexGrammarSemantics Sem;

  cident
    = qi::lexeme[ ascii::lower >> *(ascii::alnum | qi::char_('_')) ];
  string
    = qi::lexeme[ qi::char_('"') >> *(qi::char_ - qi::char_('"')) >> qi::char_('"') ];
  variable
    = qi::char_('_')
    | qi::lexeme[ ascii::upper >> *(ascii::alnum | qi::char_('_')) ];
  posinteger
    = qi::ulong_;
  term
    = termExt
    | cident     [ Sem::termFromCIdent(sem) ]
    | string     [ Sem::termFromString(sem) ]
    | variable   [ Sem::termFromVariable(sem) ]
    | posinteger [ Sem::termFromInteger(sem) ];
  terms
    = term % qi::lit(',');
  externalatom
    = (
        qi::lit('&') > cident >
        (
          (qi::lit('[') > -terms >> qi::lit(']')) ||
          (qi::lit('(') > -terms >> qi::lit(')'))
        )
      ) [ Sem::handler(sem) ];
  /*
     boost::fusion::vector2<
      std::basic_string<char>,
      boost::fusion::vector2<
        boost::optional<boost::optional<std::vector<dlvhex::ID, std::allocator<dlvhex::ID> > > >,
        boost::optional<boost::optional<std::vector<dlvhex::ID, std::allocator<dlvhex::ID> > > >
      >
     >
   */

  toplevelExt
    = qi::eps(false);
  bodyPredicateExt
    = qi::eps(false);
  headPredicateExt
    = qi::eps(false);
  termExt
    = qi::eps(false);
}

//! register module for parsing top level elements of input file
//! (use this to parse queries or other meta or control flow information)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerToplevelModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
}

//! register module for parsing body elements of rules and constraints
//! (use this to parse predicates in rule bodies)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerBodyPredicateModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
}

//! register module for parsing head elements of rules
//! (use this to parse predicates in rule heads)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerHeadPredicateModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
}

//! register module for parsing terms
//! (use this to parse terms in any predicates)
template<typename Iterator, typename Skipper>
void 
HexGrammarBase<Iterator, Skipper>::
registerTermModule(
    HexParserModuleGrammarPtr module)
{
  // TODO
}

DLVHEX_NAMESPACE_END

# if 0

  sp::chset<> alnum_("a-zA-Z0-9_");
  // nonnegative integer
  number
    = sp::token_node_d[+sp::digit_p];
  ident_or_var
    = ident | var;
  ident_or_var_or_number
    = ident | var | number;
  aggregate_leq_binop
    = str_p("<=") | '<';
  aggregate_geq_binop
    = str_p(">=") | '>';
  aggregate_binop
    = aggregate_leq_binop | aggregate_geq_binop | "==" | '=';
  binop
    = str_p("<>") | "!=" | aggregate_binop;
  cons
    = str_p(":-") | "<-";
  neg
    = ch_p('-')|'~';
  user_pred_classical
    = !neg >> ident_or_var >> '(' >> terms >> ')';
  user_pred_tuple
    = '(' >> terms >> ')';
  user_pred_atom
    = !neg >> ident_or_var;
  user_pred
    = user_pred_classical | user_pred_tuple | user_pred_atom;
  external_inputs
    = '[' >> !terms >> ']';
  external_outputs
    = '(' >> !terms >> ')';
  external_atom
    = '&' >> ident >> !external_inputs >> !external_outputs;
  aggregate_pred
    = (str_p("#any")|"#avg"|"#count"|"#max"|"#min"|"#sum"|"#times")
    >> '{' >> terms >> ':' >> body >> '}';
  aggregate_rel
    = (term >> aggregate_binop >> aggregate_pred)
    | (aggregate_pred >> aggregate_binop >> term);
  aggregate_range
    = (term >> aggregate_leq_binop >> aggregate_pred >> aggregate_leq_binop >> term)
    | (term >> aggregate_geq_binop >> aggregate_pred >> aggregate_geq_binop >> term);
  aggregate = aggregate_rel | aggregate_range;
  builtin_tertop_infix =
    term >> '=' >> term >> (ch_p('*') | '+' | '-' | '/') >> term;
  builtin_tertop_prefix =
    (ch_p('*') | '+' | '-' | '/' | str_p("#mod")) >> '(' >> term >> ',' >> term >> ',' >> term >> ')';
  builtin_binop_prefix = binop >> '(' >> term >> ',' >> term >> ')';
  builtin_binop_infix = term >> binop >> term;
  builtin_other
    = (str_p("#int") >> '(' >> term >> ')')
    | (str_p("#succ") >> '(' >> term >> ',' >> term >> ')');
  builtin_pred =
    builtin_tertop_infix | builtin_tertop_prefix |
    builtin_binop_infix | builtin_binop_prefix | builtin_other;
  naf = sp::lexeme_d[(str_p("not") | "non") >> sp::space_p];
  literal
    = builtin_pred
    | ( !naf >> (user_pred | external_atom | aggregate) );
  disj = user_pred >> *(rm[ch_p('v')] >> user_pred);
  body = literal >> *(rm[ch_p(',')] >> literal);
  maxint = str_p("#maxint") >> '=' >> number >> '.';
  namespace_ = str_p("#namespace") >> '(' >> ident >> ',' >> ident >> ')' >> '.';
  // rule (optional body/condition)
  rule_ = disj >> !(cons >> !body) >> '.';
  // constraint
  constraint = (cons >> body >> '.');
  // weak constraint
  wconstraint =
    ":~" >> body >> '.' >>
    // optional weight
    !( '[' >> !ident_or_var_or_number >> ':' >> !ident_or_var_or_number >> ']');
  clause = maxint | namespace_ | rule_ | constraint | wconstraint;
  ///@todo: namespace, maxint before other things
  root
    = *( // comment
         rm[sp::comment_p("%")]
       | clause
       )
       // end_p enforces a "full" match (in case of success)
       // even with trailing newlines
       >> !sp::end_p;

#   ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_NODE(ident);
    BOOST_SPIRIT_DEBUG_NODE(var);
    BOOST_SPIRIT_DEBUG_NODE(number);
    BOOST_SPIRIT_DEBUG_NODE(ident_or_var);
    BOOST_SPIRIT_DEBUG_NODE(ident_or_var_or_number);
    BOOST_SPIRIT_DEBUG_NODE(cons);
    BOOST_SPIRIT_DEBUG_NODE(term);
    BOOST_SPIRIT_DEBUG_NODE(terms);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_leq_binop);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_geq_binop);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_binop);
    BOOST_SPIRIT_DEBUG_NODE(binop);
    BOOST_SPIRIT_DEBUG_NODE(external_inputs);
    BOOST_SPIRIT_DEBUG_NODE(external_outputs);
    BOOST_SPIRIT_DEBUG_NODE(external_atom);
    BOOST_SPIRIT_DEBUG_NODE(aggregate);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_pred);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_rel);
    BOOST_SPIRIT_DEBUG_NODE(aggregate_range);
    BOOST_SPIRIT_DEBUG_NODE(naf);
    BOOST_SPIRIT_DEBUG_NODE(builtin_tertop_infix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_tertop_prefix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_binop_infix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_binop_prefix);
    BOOST_SPIRIT_DEBUG_NODE(builtin_other);
    BOOST_SPIRIT_DEBUG_NODE(builtin_pred);
    BOOST_SPIRIT_DEBUG_NODE(literal);
    BOOST_SPIRIT_DEBUG_NODE(disj);
    BOOST_SPIRIT_DEBUG_NODE(neg);
    BOOST_SPIRIT_DEBUG_NODE(user_pred_classical);
    BOOST_SPIRIT_DEBUG_NODE(user_pred_tuple);
    BOOST_SPIRIT_DEBUG_NODE(user_pred_atom);
    BOOST_SPIRIT_DEBUG_NODE(user_pred);
    BOOST_SPIRIT_DEBUG_NODE(body);
    BOOST_SPIRIT_DEBUG_NODE(maxint);
    BOOST_SPIRIT_DEBUG_NODE(namespace_);
    BOOST_SPIRIT_DEBUG_NODE(rule_);
    BOOST_SPIRIT_DEBUG_NODE(constraint);
    BOOST_SPIRIT_DEBUG_NODE(wconstraint);
    BOOST_SPIRIT_DEBUG_NODE(clause);
    BOOST_SPIRIT_DEBUG_NODE(root);
#   endif
}
#endif

#endif // DLVHEX_HEX_GRAMMAR_TCC_INCLUDED

// Local Variables:
// mode: C++
// End:
