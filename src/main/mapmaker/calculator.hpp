#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <unordered_map>
#include <vector>

#include "functions/area.hpp"
#include "functions/center.hpp"
#include "functions/envelope.hpp"
#include "model/geometry/rectangle.hpp"
#include "model/graph/undirected_graph.hpp"
#include "model/map/bonus.hpp"
#include "model/map/territory.hpp"
#include "model/memory/area.hpp"
#include "model/memory/buffer.hpp"
#include "model/memory/node.hpp"
#include "model/memory/ring.hpp"
#include "model/type.hpp"

using namespace model;

namespace mapmaker
{

    namespace calculator
    {

        /**
         * A calculator for territory centerpoints.
         */
        class CenterCalculator
        {
        
            /* Members */

            std::vector<map::Territory>& m_territories;

        public:

            /* Constructors */

            CenterCalculator(
                std::vector<map::Territory>& territories
            ) : m_territories(territories) {}

        public:

            /* Methods */

            /**
             * Calculate the centerpoints for each territory.
             * 
             * Time complexity: Linear
             */
            void create_centerpoints()
            {
                // Calculate the center point for each territory
                for (map::Territory& territory : m_territories)
                {
                    // Calculate the centerpoint and save it directly in
                    // the territory
                    territory.center() = functions::center(territory.geometry());
                }
            }

        };

        class ArmyCalculator
        {
        
            /* Constants */

            const float TERRITORY_WEIGHT = 0.5;
            const float OUTER_WEIGHT = 0.5;

            /* Members */

            std::vector<map::Territory>& m_territories;
            std::vector<map::Bonus>& m_bonuses;
            std::vector<map::SuperBonus>& m_super_bonuses;
            const graph::UndirectedGraph& m_neighbors;

        public:

            /* Constructors */

            ArmyCalculator(
                std::vector<map::Territory>& territories,
                std::vector<map::Bonus>& bonuses,
                std::vector<map::SuperBonus>& super_bonuses,
                graph::UndirectedGraph& neighbors
            ) : m_territories(territories), m_bonuses(bonuses), m_super_bonuses(super_bonuses), m_neighbors(neighbors) {}

        protected:

            double get_score(size_t territories, size_t connections)
            {
                return TERRITORY_WEIGHT * (territories / m_territories.size())
                     + OUTER_WEIGHT * std::min(0.5 * connections / territories, 1.0);
            }

        public:

            /**
             * 
             */
            void calculate_armies(army_type min_armies, army_type max_armies)
            {

                for (map::Bonus& bonus : m_bonuses)
                {
                    // Determine all neighbors of the territories of this
                    // bonus
                    std::set<object_id_type> children, adjacents;
                    for (const map::BoundaryRef& child : bonus.children())
                    {
                        children.insert(child.ref());
                        auto neighbors = m_neighbors.adjacents(child.ref());
                        for (const object_id_type neighbor : neighbors)
                        {
                            adjacents.insert(neighbor);
                        }
                    }
                    // Determine the outer adjacents, which are territory neighbors
                    // that do not belong to this bonus
                    std::vector<object_id_type> outer_adjacents;
                    std::set_difference(
                        adjacents.begin(),
                        adjacents.end(),
                        children.begin(),
                        children.end(),
                        std::back_inserter(outer_adjacents)
                    );
                    // Calculate the score for the current bonus
                    double score = get_score(children.size(), outer_adjacents.size());
                    // Assign the number of armies according to the maximum and minimum
                    // army count
                    bonus.armies() = std::max((army_type) std::round<int>(score * max_armies), min_armies);
                }

                // TODO do the same for super bonuses

            }

        };

        class ColorCalculator
        {
        
            /* Members */

        public:

            ColorCalculator() {}

        };

    }

}