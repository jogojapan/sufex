//#include "./S2SParser.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE S2SParserTest
#include <boost/test/included/unit_test.hpp>

#include <iostream>
#include <tuple>

#include "../tupletools.hpp"

int fret(char c1, float farg, char &c2)
{
  c2 = c1;
  return static_cast<int>(farg);
}

void fvoid(char c1, char &c2)
{
  c2 = c1;
}

BOOST_AUTO_TEST_CASE(tupletools_test_call_on_tuple_lvalue)
{
  using std::tuple;

  char c
  { 'a' };

  tuple<int,float,char&> args
  { 'z',4.2,c };

  int result = rlxutil::call_on_tuple(fret,args);
  BOOST_CHECK(c == 'z');
  BOOST_CHECK(result == 4);
}

BOOST_AUTO_TEST_CASE(tupletools_test_call_on_tuple_void_lvalue)
{
  using std::tuple;

  char c
  { 'a' };

  tuple<int,char&> args
  { 'z',c };

  rlxutil::call_on_tuple(fvoid,args);
  BOOST_CHECK(c == 'z');
}

BOOST_AUTO_TEST_CASE(tupletools_test_call_on_tuple_rvalue)
{
  using std::forward_as_tuple;
  using std::ref;

  char c
  { 'a' };

  /* Testing using std::reference_wrapper for the reference. */
  int result = rlxutil::call_on_tuple(fret,forward_as_tuple('c',4.2,ref(c)));
  BOOST_CHECK(c == 'c');
  BOOST_CHECK(result == 4);

  char d
  { 'a' };

  /* Testing using a plain reference for the reference. */
  result = rlxutil::call_on_tuple(fret,forward_as_tuple('d',7.2,d));
  BOOST_CHECK(d == 'd');
  BOOST_CHECK(result == 7);
}

BOOST_AUTO_TEST_CASE(tupletools_test_call_on_tuple_void_rvalue)
{
  using std::forward_as_tuple;
  using std::ref;

  char c
  { 'a' };

  /* Testing using std::reference_wrapper for the reference. */
  rlxutil::call_on_tuple(fvoid,forward_as_tuple('c',ref(c)));
  BOOST_CHECK(c == 'c');

  char d
  { 'a' };

  /* Testing using a plain reference for the reference. */
  rlxutil::call_on_tuple(fvoid,forward_as_tuple('d',d));
  BOOST_CHECK(d == 'd');
}
