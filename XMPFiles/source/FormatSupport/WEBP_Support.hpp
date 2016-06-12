#ifndef __WEBP_Support_hpp__
#define __WEBP_Support_hpp__ 1

#include "public/include/XMP_Environment.h" // ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/Endian.h"
#include "source/XMPFiles_IO.hpp"

#include <array>
#include <map>
#include <vector>

// forward declaration:
class WEBP_MetaHandler;

namespace WEBP {

// Read 16, 24 or 32 bits stored in little-endian order.
//
// The inline functions provided in EndianUtils.hpp and XIO.hpp operate on
// pointers to XMP_Uns32, and then flip the bytes if the desired endianness
// differs from the host's. It seems to me that it is much simpler to read
// the data into XMP_Uns8 (i.e. unsigned char) arrays and just use shift
// operators (which operate on values rather than their representation
// in memory) to convert between values and the bytes read from or written to
// a file in a platform-independent way. And besides, WEBP stores dimensions
// in 24 bit LE, and converting that to and from a 32 bit pointer, having to
// account for the platform's endianness, would be a nightmare.
static inline XMP_Uns32 GetLE16(const XMP_Uns8* const data)
{
    return (XMP_Uns32)(data[0] << 0) | (data[1] << 8);
}

static inline XMP_Uns32 GetLE24(const XMP_Uns8* const data)
{
    return GetLE16(data) | (data[2] << 16);
}

static inline XMP_Uns32 GetLE32(const XMP_Uns8* const data)
{
    return (XMP_Uns32)GetLE16(data) | (GetLE16(data + 2) << 16);
}

// Store 16, 24 or 32 bits in little-endian order.
static inline void PutLE16(XMP_Uns8* const data, XMP_Uns32 val)
{
    assert(val < (1 << 16));
    data[0] = (val >> 0);
    data[1] = (val >> 8);
}

static inline void PutLE24(XMP_Uns8* const buf, XMP_Uns32 val)
{
    assert(val < (1 << 24));
    PutLE16(buf, val & 0xffff);
    buf[2] = (val >> 16);
}

static inline void PutLE32(XMP_Uns8* const data, XMP_Uns32 val)
{
    PutLE16(data, (XMP_Uns32)(val & 0xffff));
    PutLE16(data + 2, (XMP_Uns32)(val >> 16));
}

#define WEBP_MKFOURCC(a, b, c, d)                                              \
    ((XMP_Uns32)(a) | (b) << 8 | (c) << 16 | (d) << 24)

// VP8X Feature Flags.
typedef enum FeatureFlagBits {
    FRAGMENTS_FLAG_BIT, // Experimental, not enabled by default
    ANIMATION_FLAG_BIT,
    XMP_FLAG_BIT,
    EXIF_FLAG_BIT,
    ALPHA_FLAG_BIT,
    ICCP_FLAG_BIT
} FeatureFlagBits;

typedef enum ChunkId {
    WEBP_CHUNK_VP8X,    // VP8X
    WEBP_CHUNK_ICCP,    // ICCP
    WEBP_CHUNK_ANIM,    // ANIM
    WEBP_CHUNK_ANMF,    // ANMF
    WEBP_CHUNK_FRGM,    // FRGM
    WEBP_CHUNK_ALPHA,   // ALPH
    WEBP_CHUNK_IMAGE,   // VP8/VP8L
    WEBP_CHUNK_EXIF,    // EXIF
    WEBP_CHUNK_XMP,     // XMP
    WEBP_CHUNK_UNKNOWN, // Other chunks.
    WEBP_CHUNK_NIL
} ChunkId;

const XMP_Uns32 kChunk_RIFF = WEBP_MKFOURCC('R', 'I', 'F', 'F');
const XMP_Uns32 kChunk_WEBP = WEBP_MKFOURCC('W', 'E', 'B', 'P');
const XMP_Uns32 kChunk_VP8_ = WEBP_MKFOURCC('V', 'P', '8', ' ');
const XMP_Uns32 kChunk_VP8L = WEBP_MKFOURCC('V', 'P', '8', 'L');
const XMP_Uns32 kChunk_VP8X = WEBP_MKFOURCC('V', 'P', '8', 'X');
const XMP_Uns32 kChunk_XMP_ = WEBP_MKFOURCC('X', 'M', 'P', ' ');
const XMP_Uns32 kChunk_ANIM = WEBP_MKFOURCC('A', 'N', 'I', 'M');
const XMP_Uns32 kChunk_ICCP = WEBP_MKFOURCC('I', 'C', 'C', 'P');
const XMP_Uns32 kChunk_EXIF = WEBP_MKFOURCC('E', 'X', 'I', 'F');
const XMP_Uns32 kChunk_ANMF = WEBP_MKFOURCC('A', 'N', 'M', 'F');
const XMP_Uns32 kChunk_FRGM = WEBP_MKFOURCC('F', 'R', 'G', 'M');
const XMP_Uns32 kChunk_ALPH = WEBP_MKFOURCC('A', 'L', 'P', 'H');

static std::map<XMP_Uns32, ChunkId> chunkMap = {
    { kChunk_VP8X, WEBP_CHUNK_VP8X },  { kChunk_ICCP, WEBP_CHUNK_ICCP },
    { kChunk_ANIM, WEBP_CHUNK_ANIM },  { kChunk_ANMF, WEBP_CHUNK_ANMF },
    { kChunk_FRGM, WEBP_CHUNK_FRGM },  { kChunk_ALPH, WEBP_CHUNK_ALPHA },
    { kChunk_VP8_, WEBP_CHUNK_IMAGE }, { kChunk_VP8L, WEBP_CHUNK_IMAGE },
    { kChunk_EXIF, WEBP_CHUNK_EXIF },  { kChunk_XMP_, WEBP_CHUNK_XMP }
};

class Container;

class Chunk {
public:
    Chunk(Container* parent, WEBP_MetaHandler* handler);
    Chunk(Container* parent, XMP_Uns32 tag);
    virtual ~Chunk();

    virtual void write(WEBP_MetaHandler* handler);

    Container* parent;
    XMP_Uns32 tag;
    RawDataBlock data;
    XMP_Int64 pos;
    XMP_Int64 size;
    bool needsRewrite;
};

class XMPChunk : public Chunk {
public:
    XMPChunk(Container* parent, WEBP_MetaHandler* handler);
    XMPChunk(Container* parent);
    void write(WEBP_MetaHandler* handler);
};

class VP8XChunk : public Chunk {
public:
    VP8XChunk(Container* parent, WEBP_MetaHandler* handler);
    VP8XChunk(Container* parent);
    bool xmp();
    void xmp(bool);
    XMP_Uns32 width();
    XMP_Uns32 height();
    void width(XMP_Uns32);
    void height(XMP_Uns32);
};

typedef std::array<std::vector<Chunk*>, WEBP_CHUNK_NIL> Chunks;

class Container : public Chunk {
public:
    Container(WEBP_MetaHandler* handler);
    ~Container();

    void write(WEBP_MetaHandler* handler);
    void addChunk(Chunk*);
    Chunk* getExifChunk();

    Chunks chunks;
    VP8XChunk* vp8x;
};

} // namespace WEBP

#endif // __WEBP_Support_hpp__
