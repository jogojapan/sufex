/*
 * alphabet.hpp
 *
 *  Created on: 2012/12/22
 *      Author: gollerjo
 */

#ifndef ALPHABET_HPP_
#define ALPHABET_HPP_

namespace sux {

  /** Alphabet types:
   * sparse, a limited number of integers from a wide set of possible integers)
   * zero_range, a range of integers [0,K)
   * range, a range of integers [K1,K2) */
  enum class AlphabetClass { sparse, zero_range, range };

  template <AlphabetClass alphabetclass, typename Char = char>
  struct Alphabet;

  template <typename Char>
  struct Alphabet<AlphabetClass::sparse,Char>
  {

  };

  /**
   * A continuous integer alphabet from (Char)0 to _highest.
   */
  template <typename Char>
  struct Alphabet<AlphabetClass::range,Char>
  {
    Char _highest;

    constexpr Alphabet(const Char highest)
    : _highest(highest)
    { }
  };

  /**
   * A continuous integer alphabet from _lowest to _highest. Use the
   * from_zero function to transform characters to the range
   * `[0,_highest-_lowest)`.
   */
  template <typename Char>
  struct Alphabet<AlphabetClass::zero_range,Char>
  {
    Char _lowest;
    Char _highest;

    constexpr Alphabet(const Char lowest, const Char highest)
    : _lowest(lowest), _highest(highest)
    { }

    constexpr Char from_zero(const Char c)
    { return (c - _lowest); }
  };

}


#endif /* ALPHABET_HPP_ */
