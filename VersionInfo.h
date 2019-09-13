#pragma once

#include <sstream>
#include <string>

const unsigned MAJOR_VERSION = 1;
const unsigned MINOR_VERSION = 3;
const unsigned BUILD_VERSION = 0;

/// 1.0.0: - Initial release of filter with version control
/// 1.1.0: - Added preset and tune configuration options
struct VersionInfo
{
  static std::string toString()
  {
    std::ostringstream ostr;
    ostr << MAJOR_VERSION << "." << MINOR_VERSION << "." << BUILD_VERSION;
    return ostr.str();
  }

};
