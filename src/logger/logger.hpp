/**
 By Johannes Goller, created on 2012-05-01
 @author $Author: jogojapan $
 @version $Revision: 1 $
 @lastrevision $Date: 2010-05-07 20:16:23 +0900 (金, 07 5 2010) $

 Logging functions, based on boost.
*/

#ifndef _LOGGER_H_
#define _LOGGER_H_

#define BOOST_LOG_DYN_LINK

#include <boost/log/core.hpp>
#include <boost/log/filters.hpp>

namespace sufex {

  enum class log_severity_t {
    trace,debug,info,warn,error
  };

  typedef src::severity_logger<log_severity_t> logger_t;

} // end of namespace
