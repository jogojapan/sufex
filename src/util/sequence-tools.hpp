/*
 * sequence-tools.hpp
 *
 *  Created on: 2013/01/05
 *      Author: jogojapan
 */

#ifndef SEQUENCE_TOOLS_HPP_
#define SEQUENCE_TOOLS_HPP_

namespace rlx {

  namespace seqtools {

    /**
     * For functional-style size look-ups. E.g.
     *
     *     std::vector<int> vec { 1 , 3 , 5 };
     *     std::cout << "Size of the vector: " << size(vec) << std::endl;
     *
     */
    template <typename T> std::size_t size(const T& obj) { return obj.size(); }

  }

}


#endif /* SEQUENCE_TOOLS_HPP_ */
