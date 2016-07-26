#include "public/include/XMP_Const.h"
#include "public/include/XMP_Environment.h" // ! XMP_Environment.h must be the first included header.

#include "XMPFiles/source/FileHandlers/WEBP_Handler.hpp"

#include "XMPFiles/source/FormatSupport/IPTC_Support.hpp"
#include "XMPFiles/source/FormatSupport/PSIR_Support.hpp"
#include "XMPFiles/source/FormatSupport/ReconcileLegacy.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"
#include "XMPFiles/source/FormatSupport/WEBP_Support.hpp"
#include "source/XIO.hpp"

using namespace std;

/// File format handler for WEBP.

XMPFileHandler* WEBP_MetaHandlerCTor(XMPFiles* parent)
{
    return new WEBP_MetaHandler(parent);
}

// Check that the file begins with "RIFF", a 4 byte length, then the chunkType
// (WEBP).
bool WEBP_CheckFormat(XMP_FileFormat format, XMP_StringPtr filePath,
                      XMP_IO* file, XMPFiles* parent)
{
    IgnoreParam(format);
    IgnoreParam(parent);
    XMP_Assert(format == kXMP_WEBPFile);

    if (file->Length() < 12)
        return false;
    file->Rewind();

    XMP_Uns8 chunkID[12];
    file->ReadAll(chunkID, 12);
    if (!CheckBytes(&chunkID[0], "RIFF", 4)) {
        return false;
    }
    if (CheckBytes(&chunkID[8], "WEBP", 4) && format == kXMP_WEBPFile) {
        return true;
    }
    return false;
}

WEBP_MetaHandler::WEBP_MetaHandler(XMPFiles* parent) : exifMgr(0)
{
    this->parent = parent;
    this->handlerFlags = kWEBP_HandlerFlags;
    this->stdCharForm = kXMP_Char8Bit;

    this->initialFileSize = 0;
    this->mainChunk = 0;
}

WEBP_MetaHandler::~WEBP_MetaHandler()
{
    if (this->mainChunk) {
        delete this->mainChunk;
    }
    if (this->exifMgr) {
        delete this->exifMgr;
    }
    if (this->iptcMgr) {
        delete this->iptcMgr;
    }
    if (this->psirMgr) {
        delete this->psirMgr;
    }
}

void WEBP_MetaHandler::CacheFileData()
{
    this->containsXMP = false; // assume for now

    XMP_IO* file = this->parent->ioRef;
    this->initialFileSize = file->Length();

    file->Rewind();

    XMP_Int64 filePos = 0;
    while (filePos < this->initialFileSize) {
        this->mainChunk = new WEBP::Container(this);
        filePos = file->Offset();
    }

    // covered before => internal error if it occurs
    XMP_Validate(file->Offset() == this->initialFileSize,
                 "WEBP_MetaHandler::CacheFileData: unknown data at end of file",
                 kXMPErr_InternalFailure);
}

void WEBP_MetaHandler::ProcessXMP()
{
    SXMPUtils::RemoveProperties(&this->xmpObj, 0, 0, kXMPUtil_DoAllProperties);

    bool readOnly = false;
    bool xmpOnly = false;
    bool haveExif = false;
    if (this->parent) {
        readOnly =
            !XMP_OptionIsSet(this->parent->openFlags, kXMPFiles_OpenForUpdate);
        xmpOnly =
            XMP_OptionIsSet(this->parent->openFlags, kXMPFiles_OpenOnlyXMP);
    }
    if (!xmpOnly) {
        if (readOnly) {
            this->exifMgr = new TIFF_MemoryReader();
        }
        else {
            this->exifMgr = new TIFF_FileWriter();
        }
        this->psirMgr = new PSIR_MemoryReader();
        this->iptcMgr = new IPTC_Reader();
        if (this->parent) {
            exifMgr->SetErrorCallback(&this->parent->errorCallback);
        }
        if (this->mainChunk) {
            WEBP::Chunk* exifChunk = this->mainChunk->getExifChunk();
            if (exifChunk != NULL) {
                haveExif = true;
                this->exifMgr->ParseMemoryStream(exifChunk->data.data() + 6,
                                                 exifChunk->data.size() - 6);
            }
        }
    }

    if (this->containsXMP) {
        this->xmpObj.ParseFromBuffer(this->xmpPacket.c_str(),
                                     (XMP_StringLen) this->xmpPacket.size());
    }
    if (haveExif) {
        XMP_OptionBits options = k2XMP_FileHadExif;
        if (this->containsXMP) {
            options |= k2XMP_FileHadXMP;
        }
        TIFF_Manager& exif = *this->exifMgr;
        PSIR_Manager& psir = *this->psirMgr;
        IPTC_Manager& iptc = *this->iptcMgr;
        ImportPhotoData(exif, iptc, psir, kDigestMatches, &this->xmpObj,
                        options);
        // Assume that, since the file had EXIF data, some of it was mapped to
        // XMP
        this->containsXMP = true;
    }
    this->processedXMP = true;
}

void WEBP_MetaHandler::UpdateFile(bool doSafeUpdate)
{
    XMP_Validate(this->needsUpdate, "nothing to update",
                 kXMPErr_InternalFailure);

    bool xmpOnly = false;
    if (this->parent) {
        xmpOnly =
            XMP_OptionIsSet(this->parent->openFlags, kXMPFiles_OpenOnlyXMP);
    }

    if (!xmpOnly && this->exifMgr) {
        WEBP::Chunk* exifChunk = this->mainChunk->getExifChunk();
        if (exifChunk != NULL) {
            ExportPhotoData(kXMP_TIFFFile, &this->xmpObj, this->exifMgr, 0, 0);
            if (this->exifMgr->IsLegacyChanged()) {
                XMP_Uns8* exifPtr;
                XMP_Uns32 exifLen =
                    this->exifMgr->UpdateMemoryStream((void**)&exifPtr);
                RawDataBlock exifData(&exifChunk->data[0], &exifChunk->data[6]);
                exifData.insert(exifData.begin() + 6, &exifPtr[0],
                                &exifPtr[exifLen]);
                exifChunk->data = exifData;
                exifChunk->size = exifLen + 6;
                exifChunk->needsRewrite = true;
            }
        }
    }

    this->packetInfo.charForm = stdCharForm;
    this->packetInfo.writeable = true;
    this->packetInfo.offset = kXMPFiles_UnknownOffset;
    this->packetInfo.length = kXMPFiles_UnknownLength;

    this->xmpObj.SerializeToBuffer(&this->xmpPacket, kXMP_OmitPacketWrapper);

    this->mainChunk->write(this);
    this->needsUpdate = false; // do last for safety
}

void WEBP_MetaHandler::WriteTempFile(XMP_IO* tempRef)
{
    IgnoreParam(tempRef);
    XMP_Throw("WEBP_MetaHandler::WriteTempFile: Not supported (must go through "
              "UpdateFile)",
              kXMPErr_Unavailable);
}
