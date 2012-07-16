/*
 * rules.cpp
 *
 *  Created on: 2012/07/14
 *      Author: gollerjo
 */

#include <iostream>
#include <algorithm>
#include <string>
#include "rulex.hpp"

namespace rulex {

  using namespace std;

  class RulexParser
  {
  public:
    void parse_string(const std::string input)
    {
      typedef CEGrammar<string::const_iterator> Grammar;
      CEArg arg("myarg",{});
      namespace qi = boost::spirit::qi;
      qi::parse(begin(input),end(input),Grammar(),arg);
      cout << "[PARSE-RESULT][" << arg._name << ']';
      for_each(begin(arg._features),end(arg._features),[](pair<string,string> &key_val) {
        cout << '(' << key_val.first << ',' << key_val.second << ')';
      });
      cout << endl;
    }
  };
}

int main()
{
  rulex::RulexParser rparser;
  rparser.parse_string("ARG(num:s,cas:4)");
  return 0;
}
