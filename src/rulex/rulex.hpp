#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <utility>
#include <memory>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/home/phoenix/bind/bind_member_function.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_stl.hpp>

namespace qi      = boost::spirit::qi;
namespace enc     = boost::spirit::standard_wide;
namespace fusion  = boost::fusion;
namespace phoenix = boost::phoenix;

namespace rulex {

  enum class CeType : unsigned short { PRED,ARG,LIT };

  class CE
  {
  public:
    virtual CeType type()  const = 0;
    virtual CE *   clone() const = 0;
    virtual ~CE()                = 0;
  };

  inline CE::~CE() = default;

  template <CeType cetype, typename T>
  class CEBase : public CE
  {
  public:
    virtual ~CEBase() = default;
    virtual CeType type() const {
      return cetype;
    }
    virtual CE *clone() const {
      return nullptr;
      //return new T(static_cast<const T &>(*this));
    }
  };

  template <CeType cetype, typename T>
  class CEFeatured : public CEBase<cetype,T>
  {
  public:
    typedef std::vector<std::pair<std::string,std::string>> fvec;
    std::string   _name;
    fvec          _features;
    void set_features(const fvec &features) { _features = features; }

    CEFeatured(
        std::string &&name     = std::string(),
        fvec        &&features = fvec())
    : _name(std::move(name)),_features(std::move(features))
    {}
  };

  class CEPred : public CEFeatured<CeType::PRED,CEPred>
  {
  public:
    CEPred(
        std::string &&name     = std::string(),
        fvec        &&features = fvec())
    : CEFeatured::CEFeatured(std::move(name),std::move(features))
    {}
  };

  class CEArg : public CEFeatured<CeType::ARG,CEArg>
  {
  public:
    CEArg(
        std::string &&name     = std::string(),
        fvec        &&features = fvec())
    : CEFeatured::CEFeatured(std::move(name),std::move(features))
    {}
    CEArg(const CEArg &arg) = delete;
  };

  class CELit : public CEBase<CeType::LIT,CELit>
  {
  public:
    std::string _data;

    CELit(std::string &&data = std::string())
    : _data(std::move(data))
    {}
  };

  typedef fusion::vector<std::string,std::string,int,int>  annotation_t;
  typedef std::vector<annotation_t>                        annotations_t;

  template <typename Iterator>
  struct CEGrammar : boost::spirit::qi::grammar<Iterator,CEArg()>
  {
    typedef std::string              string;
    typedef std::pair<string,string> pair;
    typedef CEPred::fvec             fvec;
  private:
    qi::rule<Iterator,pair()>   _m_key_val;
    qi::rule<Iterator,fvec()>   _m_fvec;
    qi::rule<Iterator,CEArg()>  _m_arg;
  public:
    CEGrammar()
    :CEGrammar::base_type(_m_arg)
    {
      _m_key_val  = qi::lexeme[+(enc::alnum) >> ':' >> +(enc::alnum)];
      _m_fvec     = _m_key_val >> *(',' >> _m_key_val);
      _m_arg      = qi::lit("ARG(") >> _m_fvec[phoenix::bind(&CEArg::set_features,qi::_val,qi::_1)] >> ')';
    }
  };

} // end of namespace
