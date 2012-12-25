//#include "./S2SParser.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE S2SParserTest
#include <boost/test/included/unit_test.hpp>

#include <iostream>
#include <tuple>

#include "../tupletools.hpp"

int f(char c1, float, char &c2)
{
  c2 = c1;
  return 0;
}

BOOST_AUTO_TEST_CASE(tupletools_test_call_on_tuple_lvalue)
{
  using std::tuple;

  char c
  { 'a' };

  tuple<int,float,char&> args
  { 'z',4.2,c };

  rlxutil::call_on_tuple(f,args);
  BOOST_CHECK(c == 'z');
}

BOOST_AUTO_TEST_CASE(tupletools_test_call_on_tuple_rvalue)
{
  using std::forward_as_tuple;
  using std::ref;

  char c
  { 'a' };

  /* Testing using std::reference_wrapper for the reference. */
  rlxutil::call_on_tuple(f,forward_as_tuple('c',4.2,ref(c)));
  BOOST_CHECK(c == 'c');

  char d
  { 'a' };

  /* Testing using a plain reference for the reference. */
  rlxutil::call_on_tuple(f,forward_as_tuple('d',4.2,d));
  BOOST_CHECK(d == 'd');
}
