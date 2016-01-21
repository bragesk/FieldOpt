#ifndef IJKCOORDINATE_H
#define IJKCOORDINATE_H

namespace Model {
namespace Grid {

/*!
 * \brief The IJKCoordinate class represents an integer-based (I,J,K) coordinate or index.
 * All IJK coordinates must be positive.
 */
class IJKCoordinate
{
private:
    int i_;
    int j_;
    int k_;

public:
    IJKCoordinate(int i, int j, int k);

    int i() const { return i_; }
    int j() const { return j_; }
    int k() const { return k_;}

    bool operator==(const IJKCoordinate &other);
    bool operator!=(const IJKCoordinate &other);
};

}
}

#endif // IJKCOORDINATE_H