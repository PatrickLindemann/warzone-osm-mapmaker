#pragma once

#include <set>
#include <vector>

#include "model/geometry/point.hpp"
#include "model/geometry/segment.hpp"

#include "functions/detail/compare.hpp"

using namespace model::geometry;

namespace functions
{

    namespace detail
    {

        /* Functions */

        /**
         * Returns the distance of specified point p to a segment s, defined
         * by the two points s1 and s2.
         *
         * @param p  The point
         * @param s1 The first point of the segment
         * @param s2 The second point of the segment
         * @returns  The distance of the point to the segment
         *           If the distance is > 0, the point is left of the segment
         *           If the distance is < 0, the point is right of the segment
         *           If the distance is = 0, the point is on the segment
         */
        template <typename T>
        double distance(const Point<T>& p, const Point<T>& s1, const Point<T>& s2)
        {
            return (s1.x() - p.x()) * (s2.y() - p.y()) - (s2.x() - p.x()) * (s1.x() - p.y());
        }

        /**
         * Returns the distance of specified point p to a segment s.
         *
         * @param p  The point
         * @param s The first point of the segment
         * @returns  The distance of the point to the segment
         *           If the distance is > 0, the point is left of the segment
         *           If the distance is < 0, the point is right of the segment
         *           If the distance is = 0, the point is on the segment
         */
        template <typename T>
        double distance(const Point<T>& p, const Segment<T>& s)
        {
            return distance(p, s.first(), s.last());
        }

        /* Events */

       /**
        * The event type. An event can either reference the left or right point of
        * a segment.
        */
        enum Type
        {
            LEFT,
            RIGHT
        };

        /**
         * A struct that contains a point and its associated segment index and its
         * type (left or right).
         */
        template <typename T>
        struct Event
        {
            std::size_t edge;
            Point<T> point;
            Type type;
        };

        /**
         * The event comparator that is used to order events within STL containers.
         * It compares events by the xy-order of their points.
         */
        template <typename T>
        class EventComparator
        {
        public:

            bool operator() (const Event<T>& e1, const Event<T>& e2) const
            {
                // Use the greater-than comparator because the priority queue is
                // ordered by highest priority first
                return compare_gt(e1.point, e2.point);
            }

        };

        /**
         * The event queue is a priority queue that stores events with their points
         * ordered by their xy-coordinates.
         */
        template <typename T>
        class EventQueue : public std::priority_queue<Event<T>, std::vector<Event<T>>, EventComparator<T>>
        {
        public:

            /* Constructors */

           /**
            * Create an EventQueue for an (unordered) list of segments. This will
            * automatically create and order the events for each point of the
            * segments.
            *
            * @param
            */
            EventQueue(const std::vector<Segment<T>>& segments)
            {
                // Convert segments to events and add them to the internal queue                
                for (std::size_t i = 0; i < segments.size(); i++)
                {
                    // Retrieve the current segment and its points
                    const Segment<T>& segment = segments.at(i);
                    const Point<T>& p1 = segment.first();
                    const Point<T>& p2 = segment.last();
                    // Create the events for each point
                    Event<T> e1{ i, p1 };
                    Event<T> e2{ i, p2 };
                    // Determine the event types
                    if (compare_lt(p1, p2))
                    {
                        e1.type = LEFT;
                        e2.type = RIGHT;
                    }
                    else
                    {
                        e1.type = RIGHT;
                        e2.type = LEFT;
                    }
                    // Add the events to the queue
                    this->push(e1);
                    this->push(e2);
                }
            }

        };

        /* Sweep Line */

        template <typename T>
        struct SLSegment
        {
            std::size_t edge;
            Point<T> left;
            Point<T> right;
        };

        /**
         *
         */
        template <typename T>
        bool intersect(const SLSegment<T>& s1, const SLSegment<T>& s2)
        {
            // Check if segments share a node
            if (s1.left == s2.left || s1.right == s2.right || s1.left == s2.right || s1.right == s2.left)
            {
                // Segments share a common point
                return false;
            }

            // Test for existence of an intersection point
            float l_sign = distance(s1.left, s1.right, s2.left);
            float r_sign = distance(s1.left, s1.right, s2.right);
            if (l_sign * r_sign > 0)
            {
                // Endpoints of s1 have the same sign relative
                // to s2
                return false;
            }
            l_sign = distance(s2.left, s2.right, s1.left);
            r_sign = distance(s2.left, s2.right, s1.right);
            if (l_sign * r_sign > 0)
            {
                // Endpoints of s2 have the same sign relative
                // to s1
                return false;
            }

            // Found intersection
            return true;
        }

        /**
         * The event comparator that is used to order events within STL containers.
         * It compares events by the xy-order of their points.
         */
        template <typename T>
        class SLSegmentComparator
        {
        public:

            bool operator()(const SLSegment<T>& s1, const SLSegment<T>& s2) const
            {
                // Compare the left points first
                if (compare_lt(s1.left, s2.left)) return true;
                if (compare_gt(s1.left, s2.left)) return false;
                // Left points are the same, compare the right points
                if (compare_lt(s1.right, s2.right)) return true;
                // if (compare_gt(s1.right, s2.right)) return false;
                // Segments are the same
                return false;
            }

        };

        template <typename T>
        class SweepLine
        {
        public:

            /* Types */

            using tree_type = std::set<SLSegment<T>, SLSegmentComparator<T>>;
            using const_iterator = typename tree_type::const_iterator;

        protected:

            /* Members */

            std::map<std::size_t, Segment<T>> m_segments;

            tree_type m_tree;

        public:

            /* Constructors */

            SweepLine() {}

        protected:

            /* Helper Methods */

            SLSegment<T> convert(std::size_t index, const Segment<T>& segment) const
            {
                if (compare_lt(segment.first(), segment.last()))
                {
                    return SLSegment<T>{ index, segment.first(), segment.last() };
                }
                return SLSegment<T>{ index, segment.last(), segment.first() };
            }

            SLSegment<T> get(std::size_t index) const
            {
                Segment<T> s = m_segments.at(index);
                return convert(index, s);
            }

        public:

            /* Methods */

            std::pair<const_iterator, bool> insert(std::size_t index, const Segment<T>& segment)
            {
                SLSegment<T> s = convert(index, segment);
                m_segments[index] = segment;
                return m_tree.insert(s);
            }

            const_iterator find(std::size_t index) const
            {
                SLSegment<T> s = get(index);
                return m_tree.find(s);
            }

            std::size_t erase(std::size_t index)
            {
                SLSegment<T> s = get(index);
                m_segments.erase(index);
                return m_tree.erase(s);
            }

            /* Derived Methods */

            const_iterator cbegin() const noexcept
            {
                return m_tree.cbegin();
            }

            const_iterator cend() const noexcept
            {
                return m_tree.cend();
            }

            const_iterator crbegin() const noexcept
            {
                return m_tree.crbegin();
            }

            const_iterator crend() const noexcept
            {
                return m_tree.crend();
            }

            bool empty() const noexcept
            {
                return m_tree.empty();
            }

            std::size_t size() const noexcept
            {
                return m_tree.size();
            }

        };

        template <typename T>
        inline bool shamos_hoey(const std::vector<Segment<T>>& segments)
        {
            SweepLine<T> sl{};
            EventQueue<T> eq{ segments };

            while (!eq.empty())
            {
                // Retrieve the next event from the queue
                Event<T> e = eq.top();
                eq.pop();
                if (e.type == LEFT)
                {
                    // The event is a left point
                    // Retrieve the index and segment of the event and insert them
                    // into the sweep line
                    auto [it_s, b] = sl.insert(e.edge, segments.at(e.edge));
                    // Check for intersections with the segment above
                    if (it_s != sl.cbegin())
                    {
                        auto it_a = std::prev(it_s, 1);
                        if (intersect(*it_s, *it_a))
                        {
                            return true;
                        }
                    }
                    // Check for intersections with the segment below
                    if (std::next(it_s) != sl.cend())
                    {
                        auto it_b = std::next(it_s, 1);
                        if (intersect(*it_s, *it_b))
                        {
                            return true;
                        }
                    }
                }
                else if (e.type == RIGHT)
                {
                    // The event is a right point
                    // Retrieve the index of the event and find the segment in the
                    // sweep line
                    auto it_s = sl.find(e.edge);
                    // Check for intersections of the segment above and below
                    if (it_s != sl.cbegin() && std::next(it_s) != sl.cend())
                    {
                        auto it_a = std::prev(it_s, 1);
                        auto it_b = std::next(it_s, 1);
                        if (intersect(*it_a, *it_b))
                        {
                            return true;
                        }
                    }
                    // No other intersections are possible, remove the segment
                    // completely from the sweep line
                    sl.erase(e.edge);
                }
            }

            // No intersection was found
            return false;
        }

    }

}