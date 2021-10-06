#pragma once

#include <cmath>

#include "model/geometry/point.hpp"

namespace model
{

    namespace geometry
    {
        
        /**
         * 
         */
        template <typename T>
        class Circle
        {
        public:

            /* Types */

            using point_type = Point<T>;

            /* Members */

            point_type center;
            double radius;

            /* Constructors */

            Circle(point_type center) : center(center) {};
            Circle(point_type center, double radius) : center(center), radius(radius) {};

            /* Misc */

            const bool valid() const
            {
                return radius >= 0;
            }

            const double diameter() const
            {
                return this->radius * 2;
            }

            const double area() const
            {
                return M_PI * std::pow(this->radius, 2);
            }

            const double circumference() const
            {
                return 2 * this->radius * M_PI;
            }

        };

    }

}