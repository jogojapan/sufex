#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TrigramTest
#include <boost/test/included/unit_test.hpp>

#include <iterator>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ratio>
#include <glog/logging.h>

#include "../trigram.hpp"
#include "../alphabet.hpp"
#include "../../util/random.hpp"
#include "../../util/proctime.hpp"

using namespace std;

typedef unsigned char             Char;
typedef unsigned short            Pos;
typedef unsigned long             LPos;

typedef sux::TrigramSorter<Char,Pos>  SSorter;
typedef sux::TrigramSorter<Char,LPos> LSorter;

typedef sux::TrigramMaker<sux::TGImpl::arraytuple,Char,Pos> SAMaker;
typedef SAMaker::trigram_type        SATrigram;
typedef SAMaker::trigram_vec_type    SATrigrams;
typedef SSorter::CharFrequency       SCharFrequency;
typedef SSorter::CharDistribution    SCharDistribution;

typedef sux::TrigramMaker<sux::TGImpl::arraytuple,Char,LPos> LAMaker;
typedef LAMaker::trigram_type       LATrigram;
typedef LAMaker::trigram_vec_type   LATrigrams;
typedef LSorter::CharFrequency      LACharFrequency;
typedef LSorter::CharDistribution   LACharDistribution;

typedef sux::TrigramMaker<sux::TGImpl::pointer,Char,LPos> LPMaker;
typedef LPMaker::trigram_type     LPTrigram;
typedef LPMaker::trigram_vec_type LPTrigrams;

typedef sux::TrigramMaker<sux::TGImpl::tuple,Char,LPos> LTMaker;
typedef LTMaker::trigram_type     LTTrigram;
typedef LTMaker::trigram_vec_type LTTrigrams;


BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_3)
{
  const std::basic_string<Char> input { (const Char *)"abc" };
  SATrigrams expected {
  };

  SATrigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_4)
{
  const std::basic_string<Char> input { (const Char *)"abcd" };
  SATrigrams expected {
    SATrigram { 1,'b','c','d' },
  };

  SATrigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_5)
{
  const std::basic_string<Char> input { (const Char *)"abcde" };
  SATrigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' }
  };

  SATrigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_6)
{
  const std::basic_string<Char> input { (const Char *)"abcdef" };
  SATrigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' }
  };

  SATrigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_7)
{
  const std::basic_string<Char> input { (const Char *)"abcdefg" };
  SATrigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' },
    SATrigram { 4,'e','f','g' }
  };

  SATrigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_trigram_test_8)
{
  const std::basic_string<Char> input { (const Char *)"abcdefgh" };
  SATrigrams expected {
    SATrigram { 1,'b','c','d' },
    SATrigram { 2,'c','d','e' },
    SATrigram { 4,'e','f','g' },
    SATrigram { 5,'f','g','h' }
  };

  SATrigrams actual = SAMaker::make_23trigrams(begin(input),end(input));
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_chardistribution_test)
{
  using sux::Alphabet;
  using sux::AlphabetClass;
  const std::basic_string<Char> input { (const Char *)"abcabbbbcc" };

  typedef Alphabet<AlphabetClass::sparse,Char,Pos>::char_freq_type  freq_type;
  typedef Alphabet<AlphabetClass::sparse,Char,Pos>::freq_table_type table_type;
  table_type expected {
    { 'a',0 },
    { 'b',2 },
    { 'c',7 }
  };

  auto actual = sux::alphabet_tools::make_freq_table<table_type>(
      begin(input),end(input),sux::cid<Char>);
  Alphabet<AlphabetClass::sparse,Char,Pos>::make_cumulative(actual);
  BOOST_CHECK(actual.size() == 3);
  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected),
      [](const freq_type &f1, const freq_type &f2){
    return ((f1.first == f2.first) && (f1.second == f2.second));
  }));
}

BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test1)
{
  using rlxutil::parallel::portions;

  const std::basic_string<Char> input { (const Char *)"aecabfgc" };
  std::vector<SATrigram> expected
  {
    SATrigram { 4,'b','f','g' },
    SATrigram { 2,'c','a','b' },
    SATrigram { 1,'e','c','a' },
    SATrigram { 5,'f','g','c' }
  };

  auto actual = SAMaker::make_23trigrams(begin(input),end(input));

  SSorter::AlphabetSpecific<sux::AlphabetClass::sparse>::sort_23trigrams(actual,1);
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

/**
 * Auxiliary function that performs trigram creation and sorting
 * for a relatively large string and measures time. It also
 * checks whether the result of trigram sorting is correct.
 */
template <sux::TGImpl tgimpl>
void perform_multi_threaded_trigram_sorting()
{
  LOG(INFO) << "Performing sorting test for the "
      << sux::repr<tgimpl>::str
      << " trigram implementation";

  using sux::sort_23trigrams;
  using sux::AlphabetClass;

  /* Prepare for precise time measurements. */
  typedef std::chrono::duration<double,std::milli> MS;

  /* Generate a random string of 40m characters. */
  constexpr std::size_t N = 40 * 1024 * 1024;
  std::basic_string<Char> input;
  input.resize(N);
  std::generate_n(begin(input),N,
      rlxutil::RandomSequenceGeneratorUniform<Char>(' ','z'));

  /* Generate trigrams. */
  typedef sux::TrigramMaker<tgimpl,Char,LPos>     TrigramMaker;
  typedef typename TrigramMaker::trigram_type     Trigram;

  auto actual = TrigramMaker::make_23trigrams(begin(input),end(input));
  /* Make a copy of the trigrams, which will later be sorted them separately. */
  std::vector<Trigram> expected
  { actual };

  /* Trigam sort. */
  auto tp1 = rlxutil::combined_clock<std::micro>::now();
  sort_23trigrams<AlphabetClass::sparse>(actual,4);
  auto tp2 = rlxutil::combined_clock<std::micro>::now();
  /* Alternative trigram sort, as reference. */
  auto tp3 = rlxutil::combined_clock<std::micro>::now();
  std::stable_sort(std::begin(expected),std::end(expected),
      [](const Trigram &tri1, const Trigram &tri2) {
          return ((sux::triget1(tri1) < sux::triget1(tri2))
                  || ((sux::triget1(tri1) == sux::triget1(tri2))
                      && (sux::triget2(tri1) < sux::triget2(tri2)))
                  || ((sux::triget1(tri1) == sux::triget1(tri2))
                      && (sux::triget2(tri1) == sux::triget2(tri2))
                      && (sux::triget3(tri1) < sux::triget3(tri2))));
    });
  auto tp4 = rlxutil::combined_clock<std::micro>::now();

  auto duration_radix  = tp2 - tp1;
  auto duration_stable = tp4 - tp3;
  /* Print time measurements. */
  cout << setw(18) << "Total trigrams:" << setw(10)
      << distance(begin(expected),end(expected)) << '\n'
      << setw(18) << "Radix sort:" << setw(10)
      << chrono::duration_cast<MS>(duration_radix) << '\n'
      << setw(18) << "Stable sort:" << setw(10)
      << chrono::duration_cast<MS>(duration_stable) << endl;

  /* Check for equality of the two results. */
  BOOST_CHECK(equal(begin(actual),end(actual),begin(expected)));
}

//BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test_tuple)
//{
//  perform_multi_threaded_trigram_sorting<sux::TGImpl::tuple>();
//}
//
//BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test_arraytuple)
//{
//  perform_multi_threaded_trigram_sorting<sux::TGImpl::arraytuple>();
//}
//
//BOOST_AUTO_TEST_CASE(sux_builder_sort_23trigrams_test_pointer)
//{
//  perform_multi_threaded_trigram_sorting<sux::TGImpl::pointer>();
//}

template <sux::TGImpl tgimpl>
std::vector<typename sux::TrigramMaker<tgimpl,char,Pos>::trigram_type>
make_boundary_adjustment_testinput()
{
  typedef typename sux::TrigramMaker<tgimpl,char,Pos>::trigram_type elem_type;
  return {
    elem_type { 3,'a','e','c' },
    elem_type { 2,'c','a','e' },
    elem_type { 5,'c','a','f' },
    elem_type { 1,'e','c','a' },
    elem_type { 4,'e','c','a' }
  };
}

template <>
std::vector<typename sux::TrigramMaker<sux::TGImpl::pointer,char,Pos>::trigram_type>
make_boundary_adjustment_testinput<sux::TGImpl::pointer>()
{
  typedef typename sux::TrigramMaker<sux::TGImpl::pointer,char,Pos>::trigram_type elem_type;
  return {
    elem_type { "aec" },
    elem_type { "cae" },
    elem_type { "caf" },
    elem_type { "eca" },
    elem_type { "eca" }
  };
}

template <sux::TGImpl tgimpl>
void perform_boundary_adjustment_test()
{
  using sux::TrigramMaker;
  using sux::TGImpl;
  using sux::trigram_tools::content_equal;

  typedef TrigramMaker<tgimpl,char,Pos>           maker;
  typedef typename maker::trigram_type            elem_type;
  typedef std::vector<elem_type>                  vec_type;
  typedef typename vec_type::const_iterator       It;
  typedef rlxutil::parallel::portions::adjustment adjustment;

  vec_type vec
  { make_boundary_adjustment_testinput<tgimpl>() };

  rlxutil::parallel::portions portions
  { begin(vec) , end(vec) , 5 , 1 };

  auto actual = portions.get_boundaries();

  typename vec_type::const_iterator beg
  { vec.begin() };

  decltype(actual) expected
  {
    { 0 , 1 },
    { 1 , 2 },
    { 2 , 3 },
    { 3 , 4 },
    { 4 , 5 }
  };

  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));

  portions.assign(begin(vec),end(vec),5,
      [](It /*beg*/,It it,It end)
      {
        if (it != end) {
          It nx = next(it);
          if ((nx != end) && content_equal(*it,*nx))
            return adjustment::needed;
        }
        return adjustment::unneeded;
      });

  actual   = portions.get_boundaries();
  expected =
  {
    { 0 , 1 },
    { 1 , 2 },
    { 2 , 3 },
    { 3 , 5 }
  };
  BOOST_CHECK((actual.size() == expected.size()
      && (equal(begin(actual),end(actual),begin(expected)))));
}

BOOST_AUTO_TEST_CASE(sux_builder_boundary_adjustment)
{
  perform_boundary_adjustment_test<sux::TGImpl::tuple>();
  perform_boundary_adjustment_test<sux::TGImpl::arraytuple>();
  perform_boundary_adjustment_test<sux::TGImpl::pointer>();
}

BOOST_AUTO_TEST_CASE(sux_builder_lexicographical_renaming)
{
  /* Lexicographical renaming is tested only for one particular
   * implemenatation of trigrams, `TGImpl::pointer`, because
   * tests for this are the easiest to code. Aspects of the
   * other implementations that would affect lexicographical
   * renaming are assumed to be covered by other tests. */

  typedef sux::TrigramMaker<sux::TGImpl::pointer,char,Pos>   maker;
  typedef typename maker::trigram_type                       elem_type;
  std::vector<elem_type> input {
    elem_type { "aec" },
    elem_type { "aef" },
    elem_type { "bhj" },
    elem_type { "bhj" },
    elem_type { "bhj" },
    elem_type { "bhj" },
    elem_type { "dkh" },
    elem_type { "dnr" },
    elem_type { "dnr" },
    elem_type { "dnr" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "eca" },
    elem_type { "kuw" },
    elem_type { "kuw" },
    elem_type { "lpp" },
    elem_type { "lpy" },
    elem_type { "qqq" },
    elem_type { "qxz" },
    elem_type { "rst" },
    elem_type { "rsu" },
    elem_type { "rua" },
    elem_type { "rub" },
    elem_type { "ruc" }
  };

  std::vector<Pos> expected
  { 0,1,2,2,2,2,3,4,4,4,5,5,5,5,5,6,6,7,8,9,10,11,12,13,14,15 };

  typedef sux::lexicographical_renaming lex;

  auto results = sux::rename_lexicographically(input);
  std::vector<Pos> renamed_vec
  { lex::move_newstring_from(results) };

  for (const auto &elem : renamed_vec)
    {
      cout << elem << ',';
    }
  cout << endl;

  BOOST_CHECK(equal(begin(expected),end(expected),begin(renamed_vec)));
}
