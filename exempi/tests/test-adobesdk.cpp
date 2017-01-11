

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <math.h>

#include <boost/test/unit_test.hpp>

#include "../../XMPCore/source/XMPUtils.hpp"
#include "../source/EndianUtils.hpp"

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

// endian flip of the 4 bytes array
static void flip4(uint8_t *bytes) {
  std::swap(bytes[0], bytes[3]);
  std::swap(bytes[1], bytes[2]);
}
// endian flip of the 8 bytes array
static void flip8(uint8_t *bytes) {
  std::swap(bytes[0], bytes[7]);
  std::swap(bytes[1], bytes[6]);
  std::swap(bytes[2], bytes[5]);
  std::swap(bytes[3], bytes[4]);
}

BOOST_AUTO_TEST_CASE(test_getFloat)
{
  // this to create a byte representation of the float and double.
  union float_union {
    float value;
    uint8_t bytes[sizeof(float)];
  };
  union double_union {
    double value;
    uint8_t bytes[sizeof(double)];
  };
  float_union piF;
  piF.value = M_PI;
  double_union piD;
  piD.value = M_PIl;

  // copy these byte representations
  // and flip endian as needed.
  uint8_t floatLE[sizeof(float)];
  memcpy(floatLE, piF.bytes, sizeof(float));
  if (kBigEndianHost) {
    flip4(floatLE);
  }
  uint8_t floatBE[sizeof(float)];
  memcpy(floatBE, piF.bytes, sizeof(float));
  if (kLittleEndianHost) {
    flip4(floatBE);
  }
  uint8_t doubleLE[sizeof(double)];
  memcpy(doubleLE, piD.bytes, sizeof(double));
  if (kBigEndianHost) {
    flip8(doubleLE);
  }
  uint8_t doubleBE[sizeof(double)];
  memcpy(doubleBE, piD.bytes, sizeof(double));
  if (kLittleEndianHost) {
    flip8(doubleBE);
  }

  // check getting float
  float resultF = GetFloatLE(floatLE);
  BOOST_CHECK(resultF == (float)M_PI);
  resultF = GetFloatBE(floatBE);
  BOOST_CHECK(resultF == (float)M_PI);

  // check getting double
  double resultD = GetDoubleLE(doubleLE);
  BOOST_CHECK(resultD == (double)M_PIl);
  resultD = GetDoubleBE(doubleBE);
  BOOST_CHECK(resultD == (double)M_PIl);
}

BOOST_AUTO_TEST_SUITE_END()
