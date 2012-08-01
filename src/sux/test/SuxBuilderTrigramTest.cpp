//#include "./S2SParser.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE S2SParserTest
#include <boost/test/included/unit_test.hpp>

#include "../sux.hpp"
#include <iterator>
#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;

typedef unsigned char             Char;
typedef unsigned short            Pos;
typedef sux::SuxBuilder<Char,Pos> Builder;
typedef Builder::Trigram          Trigram;
typedef Builder::Trigrams         Trigrams;
typedef Builder::CharFrequency    CharFrequency;
typedef Builder::CharDistribution CharDistribution;

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_3)
{
  const std::basic_string<Char> input { (const Char *)"abc" };
  Builder::Trigrams expected {
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_4)
{
  const std::basic_string<Char> input { (const Char *)"abcd" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_5)
{
  const std::basic_string<Char> input { (const Char *)"abcde" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_6)
{
  const std::basic_string<Char> input { (const Char *)"abcdef" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_7)
{
  const std::basic_string<Char> input { (const Char *)"abcdefg" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' },
    Trigram { 4,'e','f','g' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_8)
{
  const std::basic_string<Char> input { (const Char *)"abcdefgh" };
  Builder::Trigrams expected {
    Trigram { 1,'b','c','d' },
    Trigram { 2,'c','d','e' },
    Trigram { 4,'e','f','g' },
    Trigram { 5,'f','g','h' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_chardistribution_test)
{
  const std::basic_string<Char> input { (const Char *)"abcabbbbcc" };
  vector<CharFrequency> expected {
    CharFrequency { 'b',0 },
    CharFrequency { 'c',5 },
    CharFrequency { 'a',8 }
  };

  CharDistribution actual { Builder::accumulated_charcounts(begin(input),end(input)) };
  BOOST_CHECK(actual.size() == 3);
  Pos next_count {};
  auto it(begin(actual));
  cout << it->first << ',' << it->second << '\n';
  BOOST_CHECK(it->second == next_count);
  switch (it->first) {
  case 'a': next_count += 2; break;
  case 'b': next_count += 5; break;
  case 'c': next_count += 3; break;
  }
  ++it;
  cout << it->first << ',' << it->second << '\n';
  BOOST_CHECK(it->second == next_count);
  switch (it->first) {
  case 'a': next_count += 2; break;
  case 'b': next_count += 5; break;
  case 'c': next_count += 3; break;
  }
  ++it;
  cout << it->first << ',' << it->second << '\n';
  BOOST_CHECK(it->second == next_count);
  cout.flush();
}
