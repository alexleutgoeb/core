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
 * @file   CAUAlgorithms.hpp
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * 
 * @brief  Function templates related to Common Ancestor Units (CAUs).
 */

#ifndef CAUALGORITHMS_HPP_INCLUDED__28092010
#define CAUALGORITHMS_HPP_INCLUDED__28092010

#include "Logger.hpp"
#include "EvalGraph.hpp"
#include "ModelGraph.hpp"
#include "ModelGenerator.hpp"

#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/graph/reverse_graph.hpp>

namespace CAUAlgorithms
{
  typedef std::set<int> Ancestry;

  // store for each eval unit the ancestry starting from some join
  // ancestry is stored in terms of join order integers
  // (if a unit is reachable from multiple join orders, the set contains multiple values)
  typedef boost::vector_property_map<Ancestry> AncestryPropertyMap;

  // the first parameter is the unit for which we want to find the CAUs
  // the second parameter is the ancestry map used for finding the CAUs
  // (this is required for a subsequent call to markJoinRelevance and for debugging)
  template<typename EvalGraphT>
  void findCAUs(
      std::set<typename EvalGraphT::EvalUnit>& caus,
      const EvalGraphT& eg,
      typename EvalGraphT::EvalUnit u,
      AncestryPropertyMap& apm);

  // version with internal apm
  template<typename EvalGraphT>
  void findCAUs(
      std::set<typename EvalGraphT::EvalUnit>& caus,
      const EvalGraphT& eg,
      typename EvalGraphT::EvalUnit u)
    { AncestryPropertyMap apm; return findCAUs(caus, eg, u, apm); }

  void logAPM(const AncestryPropertyMap& apm);

  // store for each unit whether it is relevant for joining
  // if it is relevant, offline model building ensures to use a common omodel
  // otherwise, offline model building just iterates over all omodels at that unit
  typedef boost::vector_property_map<bool> JoinRelevancePropertyMap;

  // given the results of findCAUs(caus, eg, u),
  // mark all units between u and elements of caus
  // as relevant (true), others as irrelevant (false)
  // (do this by going from caus along a DFS through the reversed graph,
  // marking everything that has an ancestry as relevant)
  template<typename EvalGraphT>
  void markJoinRelevance(
      JoinRelevancePropertyMap& jr,
      const EvalGraphT& eg,
      typename EvalGraphT::EvalUnit u,
      const std::set<typename EvalGraphT::EvalUnit>& caus,
      const AncestryPropertyMap& apm);

  void logJRPM(const JoinRelevancePropertyMap& jr);
} // namespace CAUAlgorithms


// implementation

namespace CAUAlgorithms
{

template<typename Graph>
class AncestryMarkingVisitor:
  public boost::default_dfs_visitor
{
public:
  typedef typename Graph::vertex_descriptor Vertex;
  typedef typename Graph::edge_descriptor Edge;

public:
  AncestryMarkingVisitor(AncestryPropertyMap& apm, std::set<Vertex>& caus):
    apm(apm), caus(caus) {}

  void examine_edge(Edge e, const Graph& g) const
  {
    Vertex from = boost::source(e, g);
    Vertex to = boost::target(e, g);

    // join order is stored in an internal property
    unsigned joinOrder = g[e].joinOrder;
    LOG("examine edge " << from << " -> " << to << " joinOrder " << joinOrder);

    Ancestry& afrom = apm[from];
    Ancestry& ato = apm[to];
    Ancestry propagate;
    if( afrom.empty() )
    {
      // directly above first vertex -> initialize
      propagate.insert(joinOrder);
    }
    else
    {
      // propagate from previous vertex
      propagate = afrom;
    }

    if( ato.empty() )
    {
      // just copy (fast way out)
      ato = propagate;
    }
    else
    {
      // check if we hit something new
      Ancestry newancestry;
      // newancestry = ato - afrom
      std::set_difference(
          ato.begin(), ato.end(),
          afrom.begin(), afrom.end(),
          std::insert_iterator<Ancestry>(newancestry, newancestry.begin()));
      if( !newancestry.empty() )
      {
        LOG("found new ancestry: " << printset(newancestry));
        caus.insert(to);
      }
      ato.insert(afrom.begin(), afrom.end());
    }
  }

protected:
  AncestryPropertyMap& apm;
  std::set<Vertex>& caus;
};

// the first parameter is the unit for which we want to find the CAUs (common ancestor units)
// the second parameter is the ancestry map used for finding the CAUs
// (this is required for a subsequent call to markJoinRelevance and for debugging)
template<typename EvalGraphT>
void
findCAUs(
    std::set<typename EvalGraphT::EvalUnit>& caus,
    const EvalGraphT& eg,
    typename EvalGraphT::EvalUnit u,
    AncestryPropertyMap& apm)
{
  std::stringstream dbgstr;
  dbgstr << "findCAUs[" << u << "]";
  LOG_FUNCTION(dbgstr.str());

  typedef EvalGraphT EvalGraph;
  typedef typename EvalGraphT::EvalUnit EvalUnit;

  // for each predecessor p of u,
  // do a DFS in the eval graph, marking ancestry with the join order of p
  // if we find an ancestry different from the one of p,
  // and we did not find it before in that DFS run, we have found a CAU

  AncestryMarkingVisitor<typename EvalGraphT::EvalGraphInt>
    visitor(apm, caus);

  boost::two_bit_color_map<boost::identity_property_map>
    cmap(eg.countEvalUnits());

  boost::depth_first_visit(
      eg.getInt(), u, visitor, cmap);
}

template<typename Graph>
class RelevanceMarkingVisitor:
  public boost::default_dfs_visitor
{
public:
  typedef typename Graph::vertex_descriptor Vertex;
  typedef typename Graph::edge_descriptor Edge;

public:
  RelevanceMarkingVisitor(const AncestryPropertyMap& apm, JoinRelevancePropertyMap& jr):
    apm(apm), jr(jr) {}

  template<typename G>
  void discover_vertex(Vertex v, const G& g) const
  {
    if( !apm[v].empty() )
      jr[v] = true;
  }

protected:
  const AncestryPropertyMap& apm;
  JoinRelevancePropertyMap& jr;
};

// given the results of findCAUs(caus, eg, u),
// mark all units between u and elements of caus
// as relevant (true), others as irrelevant (false)
// (do this by going from caus along a DFS through the reversed graph,
// marking everything that has an ancestry as relevant)
template<typename EvalGraphT>
void markJoinRelevance(
    JoinRelevancePropertyMap& jr,
    const EvalGraphT& eg,
    typename EvalGraphT::EvalUnit u,
    const std::set<typename EvalGraphT::EvalUnit>& caus,
    const AncestryPropertyMap& apm)
{
  typedef typename EvalGraphT::EvalGraphInt GraphInt;
  typedef boost::reverse_graph<GraphInt, const GraphInt&> RGraphInt;
  typedef typename EvalGraphT::EvalUnit EvalUnit;

  // reverse eval graph
  RGraphInt reg(eg.getInt());

  // mark all irrelevant
  typename RGraphInt::vertex_iterator itv, endv;
  boost::tie(itv, endv) = boost::vertices(reg);
  for(; itv != endv; ++itv)
  {
    jr[*itv] = false;
  }

  // mark unit u relevant (it causes the join)
  jr[u] = true;

  // do DFS starting from each CAU
  typename std::set<EvalUnit>::const_iterator cau;
  for(cau = caus.begin(); cau != caus.end(); ++cau)
  {
    LOG("marking relevance from cau " << *cau);
    RelevanceMarkingVisitor<typename EvalGraphT::EvalGraphInt>
      visitor(apm, jr);
    boost::two_bit_color_map<boost::identity_property_map>
      cmap(eg.countEvalUnits());
    boost::depth_first_visit(
        reg, *cau, visitor, cmap);
    logJRPM(jr);
  }
}

} // namespace CAUAlgorithms

#endif // CAUALGORITHMS_HPP_INCLUDED__28092010
