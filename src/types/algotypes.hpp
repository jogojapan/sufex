/*
 * types.hpp
 *
 *  Created on: 2013/01/02
 *      Author: jogojapan
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#include "../util/more_type_traits.hpp"

namespace rlx {

  /**
   * This namespace defined standard types and auxiliary templates
   * to derived standard data types used for string algorithms.
   */
  namespace algotypes {

    template <typename NGram>
    struct ngram
    {
      typedef typename NGram::char_type c;
      typedef typename NGram::pos_type  pos;
    };

    template <typename Container>
    struct char_vec
    { typedef rlxtype::elemtype<Container> c; };

    template <typename It>
    struct char_it
    { typedef rlxtype::deref<It> c; };

    template <typename Container>
    struct ngram_vec
    {
      typedef rlxtype::elemtype<Container> elem;

      typedef ngram<elem> etype;
      typedef typename etype::c   c;
      typedef typename etype::pos pos;
    };

    template <typename It>
    struct ngram_it
    {
      typedef rlxtype::deref<It> elem;

      typedef ngram<elem> etype;
      typedef typename etype::c   c;
      typedef typename etype::pos pos;
    };

    template <typename Frame> using elemtype = typename Frame::elem;
    template <typename Frame> using chtype   = typename Frame::c;
    template <typename Frame> using postype  = typename Frame::pos;

  } // algotypes

} // rlx

#endif /* TYPES_HPP_ */
