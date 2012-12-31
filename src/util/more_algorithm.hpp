/*
 * more_algorithm.hpp
 *
 *  Created on: 2013/01/01
 *      Author: jogojapan
 */

#ifndef MORE_ALGORITHM_HPP_
#define MORE_ALGORITHM_HPP_

#include "more_type_traits.hpp"

namespace rlxutil {
  namespace algorithm {

    template <typename It>
    typename deref<It>::type make_cumulative(It from, It to)
    {
      using std::swap;
      typedef typename deref<It>::type elem_type;

      elem_type total
      { };

      while (from != to)
        {
          total += *from;
          *from = total;
          ++from;
        }

      return total;
    }

  }
}


#endif /* MORE_ALGORITHM_HPP_ */
