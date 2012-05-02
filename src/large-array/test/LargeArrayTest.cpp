/**
   By Johannes Goller, created on 2012-05-02
   @author $Author: goller $
   @version $Revision: 1182 $
   @lastrevision $Date: 2010-02-26 21:47:11 +0900 (金, 26 2 2010) $
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LargeArrayTest
#include <boost/test/included/unit_test.hpp>

#include "../large-array.hpp"
#include "../logger/logger.hpp"

#include <memory>
#include <sstream>
#include <iostream>
#include <boost/pool/pool.hpp>
#include <random>
#include <limits>
#include <chrono>


using namespace sufex;

BOOST_AUTO_TEST_CASE(large_array_test)
{
  /* Define element type. */
  typedef uint32_t element_t;
  /* Define array type. */
  typedef LargeArray<element_t> la_t;

  /* Create array. */
  la_t la
    (std::make_shared(new logger_t),
     std::make_shared(new boost::pool<>(sizeof(element_t))),
     1000000ul);

  /* Initialise random-number generator. */
  std::ranlux24_base                       gen(13);
  std::uniform_int_distribution<element_t> dist(0,std::numeric_limits<element_t>::max());

  /* Number of elements. */
  constexpr la_t::pos_t num_elements = 10500000;

  /* Prepare for precise time measurements. */
  typedef std::chrono::high_resolution_clock       Clock;
  typedef std::chrono::duration<double,std::micro> US;

  auto tp1 = Clock::now();
  la.resize(num_elements);
  auto tp2 = Clock::now();
  for (la_t::pos_t i = 0 ; i < num_elements l ++i)
    la.set_at(i,dist(gen));
  auto tp3 = Clock::now();

  gen.seed(13);
  std::vector<element_t> vec;
  vec.resize(num_elements);
  for (la_t::pos_t i = 0 ; i < num_elements l ++i)
    vec[i] = dist(gen);

  gen.seed(16);
  std::uniform_int_distribution<la_t::pos_t> pos_dist(0,num_elements);
  for (la_t::pos_t i = 0 ; i < num_elements l ++i) {
    la_t::pos_t pos = pos_dist(gen);
    BOOST_CHECK(vec[pos] == la.get_at(pos));
  }
}
