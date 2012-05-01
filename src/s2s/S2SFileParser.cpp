#include <iostream>

#define BOOST_FILESYSTEM_DYN_LINK
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "./S2SParser.hpp"

namespace sufex {

  class S2SFileParser
  {
  public:
    void parse_file(const char *const path)
    {
      using namespace boost::interprocess;
      namespace fs = boost::filesystem;

      if (!fs::exists(path) || !fs::is_regular(path))
	return;
      const size_t file_size = fs::file_size(path);

      /* Map input file to memory. */
      file_mapping  mapping(path,read_only);
      mapped_region region(mapping,read_only,0,file_size);
      const char *cptr = static_cast<const char *>(region.get_address());
      const char *const eptr = cptr + file_size;

      /* Parse it. */
      S2SChunk s2s_chunk;
      namespace qi = boost::spirit::qi;
      qi::parse(cptr,eptr,S2SChunkGrammar<const char*>(),s2s_chunk);
      std::cout << "[PARSE-RESULT]\n"
		<< s2s_chunk
		<< std::endl;
    }
  };

}

int main()
{
  sufex::S2SFileParser fparser;
  fparser.parse_file("./test1.s2s");
}
