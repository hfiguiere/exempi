// Endian check for Unix, refactored because examples needs it.
//

#ifdef CHECKED_ENDIANNESS

# if defined(WORDS_BIGENDIAN)
#   define kBigEndianHost 1
# else
#   define kBigEndianHost 0
# endif

#else

# if __sun
#  include <sys/isa_defs.h>
#  ifdef _LITTLE_ENDIAN
#    define kBigEndianHost 0
#  else
#    define kBigEndianHost 1
#  endif
# else
#  include <endian.h>
#  if BYTE_ORDER == BIG_ENDIAN
#   define kBigEndianHost 1
#  elif BYTE_ORDER == LITTLE_ENDIAN
#   define kBigEndianHost 0
#  else
#   error "Neither BIG_ENDIAN nor LITTLE_ENDIAN is set"
#  endif
# endif

#endif // CHECKED_ENDIANNESS
