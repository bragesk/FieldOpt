#ifndef XYZCOORDINATE_H
#define XYZCOORDINATE_H

#include <QVector3D>
#include <Eigen/Dense>

namespace Reservoir {
    namespace Grid {

        /*!
         * \brief The XYZCoordinate class represents a float-based coordinate (x, y, z).
         * XYZ Coordinates are immutable.
         */
        class XYZCoordinate
        {
        public:
            XYZCoordinate(){}
            XYZCoordinate(double x, double y, double z);
            XYZCoordinate(QVector3D point);

            double x() const { return x_; }
            double y() const { return y_; }
            double z() const { return z_; }

            /*!
             * \brief Equals Checks whether two points are (approximately) equal.
             * \param other Coordinate to be compared with this.
             * \param epsilon Allowed error.
             * \return True if xyz is within epsilon of this along all dimensions.
             */
            bool Equals(const XYZCoordinate *other, double epsilon=0.0) const;
            bool Equals(const XYZCoordinate &other, double epsilon=0.0) const;

            /*!
             * \brief Add Returns a new coordinate which is the sum of this coordinate and another coordinate.
             */
            XYZCoordinate *Add(const XYZCoordinate *other) const;

            /*!
             * \brief Get a QVector3D object representing the (x, y, z) coordinates of this point.
             */
            QVector3D toQvec() const;

            Eigen::Vector3d toEigenVec() const;

            /*!
             * \brief Find vector going from this point to the other point.
             * \param other The point to create a vector to.
             * \return The vector between this point and the other point.
            */
            QVector3D vectorTo(const XYZCoordinate other) const;

            Eigen::Vector3d vectorTo_eigen(const XYZCoordinate other) const;


        private:
            double x_;
            double y_;
            double z_;
        };

    }
}

#endif // XYZCOORDINATE_H