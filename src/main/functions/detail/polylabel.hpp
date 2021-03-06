#pragma once

#include <cmath>
#include <queue>

#include "model/geometry/point.hpp"
#include "model/geometry/rectangle.hpp"
#include "model/geometry/polygon.hpp"

using namespace model;

namespace functions
{

    namespace detail
    {

        /* Constants */

        const double SQRT_TWO = std::sqrt(2);

        /* Classes */

        template <typename T>
        struct Cell
        {
            geometry::Point<T> center;
            double half;
            double distance;
            double max;

            Cell(
                geometry::Point<T> center,
                double half,
                const geometry::Polygon<T>& polygon
            ) : center(center), half(half)
            {
                distance = distance_to_polygon(center, polygon);
                max = distance + half * SQRT_TWO;
            }
        };

        /* Functions */

        /**
         *
         * @param polygon
         * @returns
         */
        template <typename T>
        inline Cell<T> get_centroid(const geometry::Polygon<T>& polygon)
        {
            double area = 0;
            geometry::Point<T> center;
            for (std::size_t i = 0, j = polygon.outer().size() - 1; i < polygon.outer().size(); j = i++)
            {
                const geometry::Point<T>& left = polygon.outer().at(i);
                const geometry::Point<T>& right = polygon.outer().at(j);
                auto f = left.x() * right.y() - right.x() * left.y();
                center.x() += (left.x() + right.x()) * f;
                center.y() += (left.y() + right.y()) * f;
                area += f * 3;
            }
            if (area > 0)
            {
                center /= area;
                return Cell<T>{ center, 0, polygon } ;
            }
            return Cell<T>{ polygon.outer().at(0), 0, polygon };
        }

        /**
         *
         */
        template <typename T>
        inline std::pair<geometry::Point<T>, double> polylabel(
            const geometry::Polygon<T>& polygon,
            double precision = 1
        ) {
            using Cell = detail::Cell<T>;

            // Calculate the polygon envelope, which is the minimal bounding
            // box that enclosed the outer ring
            const geometry::Rectangle<T> polygon_envelope = envelope(polygon);

            // Scale the cells according to the envelope
            const T cell_size = std::min(polygon_envelope.width(), polygon_envelope.height());
            if (cell_size == 0)
            {
                return std::make_pair(polygon_envelope.min(), 0);
            }
            T half = cell_size / 2;

            // Prepare the priority queue
            auto compare = [](const Cell& a, const Cell& b)
            {
                return a.max < b.max;
            };
            std::priority_queue<Cell, std::vector<Cell>, decltype(compare)> queue(compare);

            // Cover the polygon with the initial cells
            for (T x = polygon_envelope.min().x(); x < polygon_envelope.max().x(); x += cell_size)
            {
                for (T y = polygon_envelope.min().y(); y < polygon_envelope.max().y(); y += cell_size)
                {
                    queue.push(Cell({ x + half, y + half }, half, polygon));
                }
            }

            // take centroid as the first best guess
            Cell best_cell = detail::get_centroid(polygon);
            
            // second guess: bounding box centroid
            Cell envelope_center_cell(center(polygon_envelope).first, 0, polygon);
            if (envelope_center_cell.distance > best_cell.distance)
            {
                best_cell = envelope_center_cell;
            }
            
            while (!queue.empty())
            {
                // Pick the most promising cell from the top of the queue
                Cell cell = queue.top();
                queue.pop();

                // Update the best cell if another one is found
                if (cell.distance > best_cell.distance)
                {
                    best_cell = cell;
                }

                // Check if there potentially is a better solution
                if (cell.max - best_cell.distance <= precision)
                {
                    continue;
                }

                // Split the current cell into 4 cells and add them to the queue
                half = cell.half / 2;
                queue.push(Cell({ cell.center.x() + half, cell.center.y() + half }, half, polygon));
                queue.push(Cell({ cell.center.x() + half, cell.center.y() - half }, half, polygon));
                queue.push(Cell({ cell.center.x() - half, cell.center.y() + half }, half, polygon));
                queue.push(Cell({ cell.center.x() - half, cell.center.y() - half }, half, polygon));

            }

            return std::make_pair(best_cell.center, best_cell.distance);
        }

    }

}