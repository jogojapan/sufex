/*
 * random.hpp
 *
 *  Created on: 2012/08/26
 *      Author: Johannes Goller
 */

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>

namespace rlxutil {

template <typename Char, typename RandomNumberGenerator = std::ranlux24>
class RandomSequenceGeneratorBase
{
public:
  /** Random number generator. */
  RandomNumberGenerator _random_number_generator;

  /**
   * Constructs a random sequence generator using a hardware-generated
   * random integer as seed.
   */
  RandomSequenceGeneratorBase()
  :_random_number_generator()
  {
    std::random_device rd;
    _random_number_generator.seed(rd());
  }

  /**
   * Construts a random sequence generator using the given value as seed.
   */
  RandomSequenceGeneratorBase(const typename RandomNumberGenerator::result_type seed)
  :_random_number_generator(seed)
  {}
};


template <typename Char, typename RandomNumberGenerator = std::ranlux24>
class RandomSequenceGeneratorUniform : public RandomSequenceGeneratorBase<Char,RandomNumberGenerator>
{
public:
  /** Uniform integer distribution used to generate the
   * elements of the sequence. */
  std::uniform_int_distribution<Char> _dist;

  /**
   * Constructs a random sequence generator using a hardware-generated
   * random integer as seed.
   */
  RandomSequenceGeneratorUniform(const Char min, const Char max)
  :RandomSequenceGeneratorBase<Char,RandomNumberGenerator>(),_dist(min,max)
  {}

  /**
   * Construts a random sequence generator using the given value as seed.
   */
  RandomSequenceGeneratorUniform(
      const typename RandomNumberGenerator::result_type seed,
      const Char min, const Char max)
  :RandomSequenceGeneratorBase<Char,RandomNumberGenerator>(seed),_dist(min,max)
  {}

  /** Generate the next element of the sequence. */
  Char operator()() {
    return _dist(RandomSequenceGeneratorBase<Char,RandomNumberGenerator>::_random_number_generator);
  }

};

}

#endif /* RANDOM_HPP_ */
