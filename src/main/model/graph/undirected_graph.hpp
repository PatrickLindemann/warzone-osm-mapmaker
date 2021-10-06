#pragma once

#include <algorithm>
#include <set>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <exception>

#include "model/graph/graph.hpp"

namespace model
{

    namespace graph
    {

        /**
         * An undirected graph structure with vertices and edges.
         */
        class UndirectedGraph : public Graph
        {
        public:

            /* Constructor */

            UndirectedGraph() {};

            /* Methods */

            /**
             * Returns the size of the graph, which is a pair of the number
             * of vertices and number of edges. 
             */
            std::pair<size_t, size_t> size() const override
            {
                return std::make_pair(m_vertices, m_edges.size() / 2);
            }

            protected:

            /**
             * Creates a reversed edge by swapping the source and
             * target vertices.
             */
            edge_type reverse(const edge_type& edge) const
            {
                return edge_type{ edge.second, edge.first };
            }

            public:

            /* Edge methods */

            /**
             * Retrieves the vertex count in the graph.
             * 
             * Time complexity: Constant
             */
            size_t edge_count() const override
            {
                return m_edges.size() / 2;
            }

            /**
             * Inserts an edge into the graph. 
             * 
             * @param edge The edge as <vertex, vertex> pair.
             * 
             * Time complexity: Logarithmic
             */
            void insert_edge(edge_type edge) override
            {
                m_edges.insert(edge);
                m_edges.insert(reverse(edge));
                m_vertices = std::max(m_vertices, edge.first + 1);
                m_vertices = std::max(m_vertices, edge.second + 1);
            };

            /**
             * Checks if an edge exists in the graph.
             * 
             * @param edge The edge as <vertex, vertex> pair.
             * @returns true If the edge exists
             * 
             * Time complexity: Logarithmic
             */
            bool contains_edge(const edge_type& edge) const override
            {
                return m_edges.find(edge) != m_edges.end();
            }

        protected:

            /**
             * Retrieves the lower edge offset for a specified vertex.
             * 
             * @param vertex The vertex.
             * @returns An iterator to the lower edge bound.
             * 
             * Time complexity: Logarithmic
             */
            edge_const_iterator lower_edge_bound(const vertex_type& vertex) const
            {
                return m_edges.lower_bound(
                    std::make_pair(vertex, std::numeric_limits<vertex_type>::lowest())
                );
            }

            /**
             * Retrieves upper edge offset for a specified vertex.
             * 
             * @param vertex The vertex.
             * @returns An iterator to the upper edge bound.
             * 
             * Time complexity: Logarithmic
             */
            edge_const_iterator upper_edge_bound(const vertex_type& vertex) const
            {
                return m_edges.upper_bound(
                    std::make_pair(vertex, std::numeric_limits<vertex_type>::max())
                );
            }

        public:

            /**
             * Returns the degree (number of outgoing edges) for a specified
             * vertex in the graph.
             * 
             * @param Vertex The vertex.
             * @throws std::out_of_range If the vertex does not exist.
             * 
             * Time complexity: Logarithmic
             */
            size_t degree(const vertex_type& vertex) const override
            {
                // Determine the offset of the edges that contain the
                // vertex as source 
                edge_const_iterator it_l, it_u;
                it_l = lower_edge_bound(vertex);
                it_u = upper_edge_bound(vertex);
                if (it_l == m_edges.end())
                {
                    // No edge with this source vertex exists
                    return 0;
                }
                return std::distance(it_l, it_u);
            }

            /**
             * Retrieves the adjacending vertices for a specified vertex
             * in the graph.
             * 
             * @param Vertex The vertex.
             * @throws std::out_of_range If the vertex does not exist.
             * 
             * Time complexity: Logarithmic
             */
            const std::vector<vertex_type> adjacents(const vertex_type& vertex) const override
            {
                std::vector<vertex_type> vertices;
                // Determine the offset of the edges that contain the
                // vertex as source 
                edge_const_iterator it_l, it_u;
                it_l = lower_edge_bound(vertex);
                it_u = upper_edge_bound(vertex);
                // Add adjacents to result list
                while (it_l != it_u)
                {
                    vertices.push_back(it_l->second);
                    it_l++;
                }
                return vertices;
            }

        };

    }

}