#include <boost/test/minimal.hpp>

#include "../../XMPCore/source/XMPUtils.hpp"

using boost::unit_test::test_suite;

// void test_xmpfiles()
int test_main(int argc, char* argv[])
{


  XMP_Int64 value = XMPUtils::ConvertToInt64("0x123456");
  BOOST_CHECK(value == 0x123456);

  value = XMPUtils::ConvertToInt64("123456");
  BOOST_CHECK(value == 123456);

  bool did_throw = false;
  try {
    value = XMPUtils::ConvertToInt64("abcdef");
  } catch(const XMP_Error & e) {
    did_throw = true;
  }
  BOOST_CHECK(did_throw);

  return 0;
}
