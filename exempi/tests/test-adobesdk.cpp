

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include "../../XMPCore/source/XMPUtils.hpp"

using boost::unit_test::test_suite;

struct Fixture {
  Fixture() {
    XMPMeta::Initialize();
  }
  ~Fixture() {
    XMPMeta::Terminate();
  }
};

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_SUITE(test_adobesdk)

BOOST_AUTO_TEST_CASE(test_composeArray)
{
  XMP_VarString fullPath;

  BOOST_CHECK_THROW(XMPUtils::ComposeArrayItemPath("", "myArray",
                                                   42, &fullPath), XMP_Error);
  BOOST_CHECK_THROW(XMPUtils::ComposeArrayItemPath("http://ns.figuiere.net/test/", "myArray",
                                                   42, &fullPath), XMP_Error);
  BOOST_CHECK_THROW(XMPUtils::ComposeArrayItemPath(kXMP_NS_DC, "",
                                                   42, &fullPath), XMP_Error);

  XMPUtils::ComposeArrayItemPath(kXMP_NS_DC, "myArray",
                                 42, &fullPath);
  BOOST_CHECK(fullPath == "myArray[42]");

  XMPUtils::ComposeArrayItemPath(kXMP_NS_DC, "myArray",
                                 kXMP_ArrayLastItem, &fullPath);
  BOOST_CHECK(fullPath == "myArray[last()]");
}

BOOST_AUTO_TEST_CASE(test_composeStructFieldPath)
{
  XMP_VarString fullPath;

  BOOST_CHECK_THROW(XMPUtils::ComposeStructFieldPath("", "struct",
                                                     kXMP_NS_XML, "field", &fullPath), XMP_Error);
  BOOST_CHECK_THROW(XMPUtils::ComposeStructFieldPath(kXMP_NS_DC, "struct",
                                                     "", "field", &fullPath), XMP_Error);
  BOOST_CHECK_THROW(XMPUtils::ComposeStructFieldPath(kXMP_NS_DC, "",
                                                     kXMP_NS_XML, "field", &fullPath), XMP_Error);
  BOOST_CHECK_THROW(XMPUtils::ComposeStructFieldPath(kXMP_NS_DC, "struct",
                                                     kXMP_NS_XML, "", &fullPath), XMP_Error);

  XMPUtils::ComposeStructFieldPath(kXMP_NS_DC, "struct",
                                   kXMP_NS_XML, "field", &fullPath);
  BOOST_CHECK(fullPath == "struct/xml:field");
}

BOOST_AUTO_TEST_CASE(test_convertToInt64)
{
  XMP_Int64 value = XMPUtils::ConvertToInt64("0x123456789012345");
  BOOST_CHECK(value == 0x123456789012345);

  value = XMPUtils::ConvertToInt64("123456");
  BOOST_CHECK(value == 123456);

  BOOST_CHECK_THROW(XMPUtils::ConvertToInt64("abcdef"), XMP_Error);
}


BOOST_AUTO_TEST_SUITE_END()
