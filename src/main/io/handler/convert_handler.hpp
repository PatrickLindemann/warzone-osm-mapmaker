#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <osmium/tags/tags_filter.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/lexical_cast.hpp>

#include <osmium/osm/item_type.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/relations/relations_manager.hpp>

#include "model/memory/buffer.hpp"
#include "model/memory/node.hpp"
#include "model/memory/relation.hpp"
#include "model/memory/way.hpp"
#include "model/type.hpp"

using namespace model;

namespace io
{

    namespace handler
    {

        /**
         * A handler that converts a stream of osmium objects to
         * internal memory objects.
         * 
         * This conversion is needed becausd osmium buffers are immutable,
         * which means that actions such as way compression and custom node
         * projections can only be applied by copying the buffer elements,
         * which is inefficient.
         */
        class ConvertHandler : public osmium::relations::RelationsManager<ConvertHandler, false, true, false>
        {

            /* Types */

            using osm_id_type = osmium::object_id_type;

            /**
             * The admin_level filter. OpenStreetMap defines 9 usable levels
             * from 2 to 11. Yet, it is also possible to use the levels 0, 1
             * and 12, which are not rendered by default, but need to be
             * considered to.
             * 
             * The index of this filter refers to the level. If the value is
             * set to true, boundaries with this level will be persisted,
             * if set to false, they will be skipped.
             * 
             * For more information, refer to
             * https://wiki.openstreetmap.org/wiki/Key:admin_level
             */
            osmium::TagsFilter m_filter;
            
            memory::Buffer<memory::Node> m_nodes;
            memory::Buffer<memory::Way> m_ways;
            memory::Buffer<memory::Relation> m_relations;

            /**
             * The identifier maps that map osmium object ids to ids in the
             * continous, natural number interval [0, N].
             * This bijective mapping ensures that all internal node ids are consecutive,
             * which is not guaranteed for osmium object ids.
             */
            std::unordered_map<osm_id_type, object_id_type> m_nids;
            std::unordered_map<osm_id_type, object_id_type> m_wids;
            std::unordered_map<osm_id_type, object_id_type> m_rids;

        public:

            /* Constructors */

            ConvertHandler(
                const level_type territory_level,
                const std::vector<level_type>& bonus_levels
            ) {
                // Initialize the level filter for relations
                m_filter = osmium::TagsFilter{false};
                m_filter.add_rule(true, "admin_level", territory_level);
                for (const auto& level : bonus_levels)
                {
                    m_filter.add_rule(true, "admin_level", level);
                }
            }

            /* Accessors */

            memory::Buffer<memory::Node>& nodes()
            {
                return m_nodes;
            }

            memory::Buffer<memory::Way>& ways()
            {
                return m_ways;
            }

            memory::Buffer<memory::Relation>& relations()
            {
                return m_relations;
            }

        protected:

            /* Helper methods */

            /**
             * Create a new node from an osmium node in the internal
             * node buffer and return a reference to it.
             * If the node already exists, it will not be re-added
             * again.
             * 
             * @param osm_node The osmium node object
             * @returns        A reference to the node in the
             *                 internal buffer
             */
            memory::Node& create_node(const osmium::NodeRef& osm_node)
            {   
                // Check if node was inserted already
                auto it = m_nids.find(osm_node.ref());
                if (it != m_nids.end())
                {
                    // memory::Node already exists, return it
                    return m_nodes.at(it->second); 
                }
                // Convert the osmium node and add the result to the node buffer
                object_id_type mapped_id = m_nids.size();
                m_nids[osm_node.ref()] = mapped_id;
                m_nodes.push_back({ mapped_id, osm_node.lon(), osm_node.lat() });
                // Return a reference to the newly created node in the container
                return m_nodes.at(mapped_id); 
            }

            /**
             * Create a new way from an osmium way in the internal
             * way buffer and return a reference to it.
             * If the way already exists, it will not be re-added
             * again.
             * 
             * @param osm_way  The osmium way object
             * @returns        A reference to the way in the
             *                 internal buffer
             */
            memory::Way& create_way(const osmium::Way& osm_way)
            {
                // Check if way was inserted already
                auto it = m_wids.find(osm_way.id());
                if (it != m_wids.end())
                {
                    // memory::Way exists, return it
                    return m_ways.at(it->second); 
                }
                // Convert the osmium way and add the result to the way buffer
                object_id_type mapped_id = m_wids.size();
                m_wids[osm_way.id()] = mapped_id;
                m_ways.push_back({ mapped_id });
                // Add the osmium nodes to the created way and to the node buffer
                memory::Way& way = m_ways.at(mapped_id);
                way.reserve(osm_way.nodes().size());
                for (const auto& osm_node : osm_way.nodes())
                {
                    memory::Node& node = create_node(osm_node);
                    way.push_back(node.id());
                }
                // Return a reference to the newly created way in the container
                return way; 
            }

        public:

            /**
             * We are interested in all relations tagged with type = multipolygon
             * or type = boundary with at least one way member.
             */
            bool new_relation(const osmium::Relation& osm_relation) const
            {
                const char* type = osm_relation.tags().get_value_by_key("type");

                // Ignore relations without "type" tag
                if (type == nullptr)
                {
                    return false;
                }

                if (((!std::strcmp(type, "multipolygon")) || (!std::strcmp(type, "boundary"))) && osmium::tags::match_any_of(osm_relation.tags(), m_filter)) {
                    return std::any_of(osm_relation.members().cbegin(), osm_relation.members().cend(), [](const osmium::RelationMember& member) {
                        return member.type() == osmium::item_type::way;
                    });
                }

                return false;
            }

            /**
             * This is called when a relation is complete, ie. all members
             * were found in the input.
             */
            void complete_relation(const osmium::Relation& osm_relation)
            {
                // Convert the osmium relation to an internal relation and add the
                // result to the relation buffer
                object_id_type mapped_id = m_rids.size();
                m_rids[osm_relation.id()] = mapped_id;
                // Create and add the new area
                memory::Relation relation = { mapped_id };
                // Add the tag values
                relation.add_tag("name", osm_relation.get_value_by_key("name", ""));
                relation.add_tag("type", osm_relation.get_value_by_key("type", ""));
                relation.add_tag("admin_level", osm_relation.get_value_by_key("admin_level", ""));
                // Add the relation's way members
                for (const auto& osm_member : osm_relation.members())
                {
                    // Ignore members with invalid references or roles
                    if (osm_member.ref() != 0
                    && osm_member.type() == osmium::item_type::way
                    && osm_member.role() != nullptr
                    ) {
                        // Retrieve way from osmium object buffer
                        const osmium::Way* osm_way = this->get_member_way(osm_member.ref());
                        // Create the way if it doesn't already exists
                        memory::Way& way = create_way(*osm_way);
                        // Add a reference to the current area depending
                        // on the member role
                        std::string role{ osm_member.role() };
                        if (role == "outer" || role == "inner")
                        {
                            relation.add_member({ way.id(), memory::Member::Type::WAY, role });
                        }
                        // throw std::invalid_argument("Invalid relation member role");
                    }
                }
                m_relations.push_back(relation);
            }

            /**
             * This is called when the member handling is done.
             */
            void after_way(const osmium::Way& osm_way)
            {
                // yCheck if the way qualifies as an area
                if (osm_way.nodes().size() <= 3
                || !osm_way.nodes().front().location()
                || !osm_way.nodes().back().location()
                || !osm_way.ends_have_same_location()
                || !osm_way.tags().has_tag("area", "no")
                ) {
                    return;
                }

                // Check way's admin level
                if (!osmium::tags::match_any_of(osm_way.tags(), m_filter))
                {
                    return;
                }

                // TODO parse way as an area

            }

        };
    }

}