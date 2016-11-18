#ifndef __UTILS_H__
#define __UTILS_H__

#include <vector>

/* type definition */
template <class T> using vector_2d = std::vector< std::vector<T> >; 
template <class T> using vector_3d = std::vector< vector_2d<T> >;

#endif /* end of include guard: __UTILS_H__ */

