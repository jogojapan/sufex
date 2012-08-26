//#include "./S2SParser.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE S2SParserTest
#include <boost/test/included/unit_test.hpp>

#include "../sux.hpp"
#include "../../util/random.hpp"
#include <iterator>
#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;

typedef unsigned char             Char;
typedef unsigned short            Pos;
typedef unsigned long             LPos;

typedef sux::SuxBuilder<Char,Pos> Builder;
typedef Builder::Trigram          Trigram;
typedef Builder::Trigrams         Trigrams;
typedef Builder::CharFrequency    CharFrequency;
typedef Builder::CharDistribution CharDistribution;

typedef sux::SuxBuilder<Char,LPos> LBuilder;
typedef Builder::Trigram           LTrigram;
typedef Builder::Trigrams          LTrigrams;
typedef Builder::CharFrequency     LCharFrequency;
typedef Builder::CharDistribution  LCharDistribution;

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
    CharFrequency { 'a',0 },
    CharFrequency { 'b',2 },
    CharFrequency { 'c',7 }
  };

  CharDistribution actual { Builder::accumulated_charcounts(begin(input),end(input)) };
  BOOST_CHECK(actual.size() == 3);
  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected),
      [](const CharFrequency &f1, const CharFrequency &f2){
    return ((f1.first == f2.first) && (f1.second == f2.second));
  }));
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test1)
{
  const std::basic_string<Char> input { (const Char *)"aecabfgc" };
  Builder::Trigrams expected {
    Trigram { 4,'b','f','g' },
    Trigram { 2,'c','a','b' },
    Trigram { 1,'e','c','a' },
    Trigram { 5,'f','g','c' }
  };

  Builder::Trigrams actual = Builder::make_23trigrams(begin(input),end(input));
  Builder::sort_23trigrams(actual);
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test2)
{
  /* Generate a random string of 40m characters. */
  constexpr std::size_t N = 40 * 1024 * 1024;
  std::basic_string<Char> input;
  input.resize(N);
  std::generate_n(std::begin(input),N,rlxutil::RandomSequenceGeneratorUniform<Char>(' ','z'));

  LTrigrams actual = Builder::make_23trigrams(begin(input),end(input));
  LBuilder::sort_23trigrams(actual);
  for (const LTrigram trigram : actual) {
    std::cout << std::get<1>(trigram)
        << std::get<2>(trigram)
        << std::get<3>(trigram)
        << '\n';
  }
  std::cout.flush();
//  BOOST_CHECK((actual.size() == expected.size()
//      && (equal(begin(actual),end(actual),begin(expected)))));
}
