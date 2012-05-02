//#include "./S2SParser.hpp"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE S2SParserTest
#include <boost/test/included/unit_test.hpp>

#include "../S2SParser.hpp"
#include <sstream>
#include <iostream>

BOOST_AUTO_TEST_CASE(s2s_parser_test)
{
  std::ostringstream input_str;
  input_str << " The big house\n\tMOR\tSB 0 1\n\tT\tDT 1 4\n\tT\ts 4 5\n\tT\tJJ 5 8\n";
  input_str << " My green house\n\tMOR\tSB 0 1\n\tT\tDT 1 3\n\tT\ts 3 4\n\tT\tJJ 4 9\n";
  const std::string input(input_str.str());

  std::string expected [] {
    " The big house[MOR,SB,0,0##T,DT,1,1##T,s,4,4##T,JJ,5,5##]",
    " My green house[MOR,SB,0,0##T,DT,1,1##T,s,3,3##T,JJ,4,4##]"
  };

  std::ostringstream output_str;
  auto it     = std::begin(input);
  auto exp_it = std::begin(expected);
  while (it != std::end(input)) {
    S2SChunk s2s_chunk;
    qi::parse(it,std::end(input),S2SChunkGrammar<decltype(it)>(),s2s_chunk);
    output_str.str("");
    output_str << s2s_chunk;
    std::cout << *exp_it << ":::" << s2s_chunk << std::endl;
    BOOST_CHECK(*exp_it == output_str.str());
    ++exp_it;
  }
}
