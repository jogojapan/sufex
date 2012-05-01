#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <utility>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>

namespace qi     = boost::spirit::qi;
namespace ascii  = boost::spirit::ascii;
namespace fusion = boost::fusion;

typedef fusion::vector<std::string,std::string,int,int>  annotation_t;
typedef std::vector<annotation_t>                        annotations_t;

struct S2SChunk
{
  std::string   surface_text;
  annotations_t annotations;
};

template <typename Stream>
Stream &operator<<(Stream &stream,const S2SChunk &chunk)
{
  stream << chunk.surface_text << '[';
  for (auto it = chunk.annotations.begin() ; it != chunk.annotations.end() ; ++it) {
    stream << fusion::at_c<0>(*it) << ','
	   << fusion::at_c<1>(*it) << ','
	   << fusion::at_c<2>(*it) << ','
	   << fusion::at_c<2>(*it) << "##";
  }
  // std::for_each(chunk.annotations.begin(),chunk.annotations.end(),
  // 		[&stream](decltype(chunk.annotations.begin()) it) {
  // 		  stream << fusion::at_c<0>(*it) << "##";
  //   });
  stream << ']';
  return stream;
}


BOOST_FUSION_ADAPT_STRUCT(
			  S2SChunk,
			  (std::string,surface_text)
			  (annotations_t,annotations)
			  )

template <typename Iterator>
struct S2SChunkGrammar : boost::spirit::qi::grammar<Iterator,S2SChunk()>
{
private:
  qi::rule<Iterator,std::string()>             _m_surface_text;
  qi::rule<Iterator,annotation_t()>            _m_annotation;
  qi::rule<Iterator,std::string()>             _m_token_type;
  qi::rule<Iterator,std::string()>             _m_annotation_type;
  qi::rule<Iterator,annotations_t()>           _m_annotations;
  qi::rule<Iterator,S2SChunk()>                _m_s2s_chunk;
public:
  S2SChunkGrammar()
    :S2SChunkGrammar::base_type(_m_s2s_chunk)
  {
    _m_surface_text =
      +(ascii::char_ - '\n') >>
      '\n';
    _m_token_type      = +(ascii::char_ - ascii::space);
    _m_annotation_type = +(ascii::char_ - ascii::space);
    _m_annotation      = 
      qi::lit('\t') >> _m_token_type >> qi::lit('\t') >> _m_annotation_type
		    >> +qi::lit(' ') >> qi::int_ >> +qi::lit(' ') >> qi::int_ >> qi::lit('\n');
    _m_annotations     = +_m_annotation;
    _m_s2s_chunk       = _m_surface_text >> _m_annotations;
  }
};

int main() {
  std::ostringstream input_str;
  input_str << " The big house\n\tMOR\tSB 0 1\n\tT\tDT 1 4\n\tT\ts 4 5\n\tT\tJJ 5 8\n";
  input_str << " My green house\n\tMOR\tSB 0 1\n\tT\tDT 1 3\n\tT\ts 3 4\n\tT\tJJ 4 9\n";
  const std::string input(input_str.str());
  auto it = std::begin(input);
  while (it != std::end(input)) {
    S2SChunk s2s_chunk;
    qi::parse(it,std::end(input),S2SChunkGrammar<decltype(it)>(),s2s_chunk);
    std::cout << "[PARSED] " << s2s_chunk << std::endl;
  }
}
