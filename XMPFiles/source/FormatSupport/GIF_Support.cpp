// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
//
// Derived from PNG_Support.cpp by Ian Jacobi
// Ported to the CS6 version by Hubert Figuiere
// =================================================================================================

#include "GIF_Support.hpp"
#include <string.h>

#include "source/XIO.hpp"

typedef std::basic_string<unsigned char> filebuffer;

// Don't need CRC.

namespace GIF_Support
{
        // This only really counts for extension blocks.
	enum blockType {
		bGraphicControl = 0xF9,
		bComment        = 0xFE,
		bPlainText      = 0x01,
		bApplication    = 0xFF,
		// Hacky.  Don't like the following, but there's no easy way.
		bImage          = 0x2C,
		bExtension      = 0x21,
		bTerminator     = 0x3B,
		bHeader         = 0x47
	};

	// =============================================================================================

	long OpenGIF (  XMP_IO* fileRef, BlockState & inOutBlockState )
	{
		XMP_Uns64 pos = 0;
		unsigned char name;
		XMP_Uns32 len;
		BlockData newBlock;
	
		pos = fileRef->Seek ( 0, kXMP_SeekFromStart );
		// header needs to be a block, mostly for our safe write.
		pos = ReadHeader ( fileRef );
		if (pos < 13)
			return 0;
		
		newBlock.pos = 0;
		newBlock.len = pos;
		newBlock.type = bHeader;
		inOutBlockState.blocks.push_back(newBlock);
	
		// read first and following blocks
		while ( ReadBlock ( fileRef, inOutBlockState, &name, &len, pos) ) {}
	
		return inOutBlockState.blocks.size();

	}

	// =============================================================================================

	long ReadHeader ( XMP_IO* fileRef )
	{
		long bytesRead;
		long headerSize;
		long tableSize = 0;
		long bytesPerColor = 0;
		unsigned char buffer[768];
		
		headerSize = 0;
		bytesRead = fileRef->Read ( buffer, GIF_SIGNATURE_LEN );
		if ( bytesRead != GIF_SIGNATURE_LEN ) return 0;
		if ( memcmp ( buffer, GIF_SIGNATURE_DATA, GIF_SIGNATURE_LEN) != 0 ) return 0;
		headerSize += bytesRead;
		bytesRead = fileRef->Read ( buffer, 3 );
		if ( bytesRead != 3 ) return 0;
		if ( memcmp ( buffer, "87a", 3 ) != 0 && memcmp ( buffer, "89a", 3 ) != 0 ) return 0;
		headerSize += bytesRead;
		bytesRead = fileRef->Read ( buffer, 4 );
		if ( bytesRead != 4 ) return 0;
		headerSize += bytesRead;
		bytesRead = fileRef->Read ( buffer, 3 );
		if ( bytesRead != 3 ) return 0;
		headerSize += bytesRead;
		if ( buffer[0] & 0x80 ) tableSize = (1 << ((buffer[0] & 0x07) + 1)) * 3;
		bytesRead = fileRef->Read ( buffer, tableSize );
		if ( bytesRead != tableSize ) return 0;
		headerSize += bytesRead;
		
		return headerSize;
	}

	// =============================================================================================

	bool  ReadBlock ( XMP_IO* fileRef, BlockState & inOutBlockState, unsigned char * blockType, XMP_Uns32 * blockLength, XMP_Uns64 & inOutPosition )
	{
		try
		{
			XMP_Uns64 startPosition = inOutPosition;
			long bytesRead;
			long blockSize;
			unsigned char buffer[768];

			bytesRead = fileRef->Read ( buffer, 1 );
			if ( bytesRead != 1 ) return false;
			inOutPosition += 1;
			if ( buffer[0] == bImage )
			{
				// Image is a special case.
			        long tableSize = 0;
				bytesRead = fileRef->Read ( buffer, 4 );
				if ( bytesRead != 4 ) return false;
				inOutPosition += 4;
				bytesRead = fileRef->Read ( buffer, 4 );
				if ( bytesRead != 4 ) return false;
				inOutPosition += 4;
				bytesRead = fileRef->Read ( buffer, 1 );
				if ( bytesRead != 1 ) return false;
				inOutPosition += 1;
				if ( buffer[0] & 0x80 ) tableSize = (1 << ((buffer[0] & 0x07) + 1)) * 3;
				bytesRead = fileRef->Read ( buffer, tableSize );
				if ( bytesRead != tableSize ) return 0;
				inOutPosition += tableSize;
				bytesRead = fileRef->Read ( buffer, 1 );
				if ( bytesRead != 1 ) return false;
				inOutPosition += 1;
				bytesRead = fileRef->Read ( buffer, 1 );
				if ( bytesRead != 1 ) return false;
				inOutPosition += 1;
				tableSize = buffer[0];
				while ( tableSize != 0x00 )
				{
					bytesRead = fileRef->Read ( buffer, tableSize );
					if ( bytesRead != tableSize ) return false;
					inOutPosition += tableSize;
					bytesRead = fileRef->Read ( buffer, 1 );
					if ( bytesRead != 1 ) return false;
					inOutPosition += 1;
					tableSize = buffer[0];
				}

				BlockData newBlock;

				newBlock.pos = startPosition;
				newBlock.len = inOutPosition - startPosition;
				newBlock.type = bImage;

				inOutBlockState.blocks.push_back ( newBlock );
			}
			else if ( buffer[0] == bExtension )
			{
				unsigned char type;
				long tableSize = 0;

				BlockData newBlock;

				newBlock.pos = startPosition;

				bytesRead = fileRef->Read ( buffer, 1 );
				if ( bytesRead != 1 ) return false;
				inOutPosition += 1;
				type = buffer[0];
				newBlock.type = type;

				bytesRead = fileRef->Read ( buffer, 1 );
				if ( bytesRead != 1 ) return false;
				inOutPosition += 1;
				tableSize = buffer[0];
				while ( tableSize != 0x00 )
				{
					bytesRead = fileRef->Read ( buffer, tableSize );
					if ( bytesRead != tableSize ) return false;
					inOutPosition += tableSize;
					if ( inOutPosition - startPosition == 14 && type == bApplication )
					{
						// Check for XMP identifier...
						CheckApplicationBlockHeader ( fileRef, inOutBlockState, newBlock, inOutPosition );

						if ( newBlock.xmp == true )
						{
							newBlock.len = inOutPosition - startPosition;

							inOutBlockState.blocks.push_back ( newBlock );

							return true;
						}
					}
					bytesRead = fileRef->Read ( buffer, 1 );
					if ( bytesRead != 1 ) return false;
					inOutPosition += 1;
					tableSize = buffer[0];
				}

				newBlock.len = inOutPosition - startPosition;

				inOutBlockState.blocks.push_back ( newBlock );
			}
			else if ( buffer[0] == bTerminator )
			{
				BlockData newBlock;

				newBlock.pos = startPosition;
				newBlock.len = 1;
				newBlock.type = buffer[0];

				inOutBlockState.blocks.push_back ( newBlock );
			}

		} catch ( ... ) {

			return false;

		}
	
		return true;

	}

	// =============================================================================================

	bool WriteXMPBlock ( XMP_IO* fileRef, XMP_Uns32 len, const char* inBuffer )
	{
		bool ret = false;
		unsigned long datalen = (APPLICATION_HEADER_LEN + len + MAGIC_TRAILER_LEN);
		unsigned char* buffer = new unsigned char[datalen];

		try
		{
			size_t pos = 0;
			memcpy(&buffer[pos], APPLICATION_HEADER_DATA, APPLICATION_HEADER_LEN);
			pos += APPLICATION_HEADER_LEN;
			memcpy(&buffer[pos], inBuffer, len);
			pos += len;
			memcpy(&buffer[pos], MAGIC_TRAILER_DATA, MAGIC_TRAILER_LEN);

			fileRef->Write(buffer, datalen);

			ret = true;
		}
		catch ( ... ) {}

		delete [] buffer;

		return ret;
	}

	// =============================================================================================

	bool CopyBlock ( XMP_IO* sourceRef, XMP_IO* destRef, BlockData& block )
	{
		try
		{
			sourceRef->Seek ( block.pos, kXMP_SeekFromStart );
			XIO::Copy (sourceRef, destRef, (block.len));

		} catch ( ... ) {

			return false;

		}
	
		return true;
	}

	// =============================================================================================

	// Don't need CRC.

	// =============================================================================================

	unsigned long CheckApplicationBlockHeader ( XMP_IO* fileRef, BlockState& inOutBlockState, BlockData& inOutBlockData, XMP_Uns64& inOutPosition )
	{
		try
		{
			fileRef->Seek((inOutBlockData.pos), kXMP_SeekFromStart);

			unsigned char buffer[256];
			long bytesRead = fileRef->Read ( buffer, APPLICATION_HEADER_LEN );

			if (bytesRead == APPLICATION_HEADER_LEN)
			{
				if (memcmp(buffer, APPLICATION_HEADER_DATA, APPLICATION_HEADER_LEN) == 0)
				{
					// We still have to go through all of the data...
					long tableSize = 0;
					long xmpSize;
					
					inOutPosition = inOutBlockData.pos + APPLICATION_HEADER_LEN;
					inOutBlockState.xmpPos = inOutPosition;
					bytesRead = fileRef->Read ( buffer, 1 );
					if ( bytesRead != 1 ) return 0;
					inOutPosition += 1;
					tableSize = buffer[0];
					while ( tableSize != 0x00 )
					{
						bytesRead = fileRef->Read ( buffer, tableSize );
						if ( bytesRead != tableSize ) return false;
						inOutPosition += tableSize;
						bytesRead = fileRef->Read ( buffer, 1 );
						if ( bytesRead != 1 ) return false;
						inOutPosition += 1;
						tableSize = buffer[0];
					}
					inOutBlockState.xmpLen = inOutPosition - inOutBlockData.pos - APPLICATION_HEADER_LEN - MAGIC_TRAILER_LEN;
					inOutBlockState.xmpBlock = inOutBlockData;
					inOutBlockData.xmp = true;
				}
			}
		}
		catch ( ... ) {}
	
		return 0;
	}

	bool ReadBuffer ( XMP_IO* fileRef, XMP_Uns64 & pos, XMP_Uns32 len, char * outBuffer )
	{
		try
		{
			if ( (fileRef == 0) || (outBuffer == 0) ) return false;
		
			fileRef->Seek ( pos, kXMP_SeekFromStart );
			long bytesRead = fileRef->Read ( outBuffer, len );
			if ( XMP_Uns32(bytesRead) != len ) return false;
		
			return true;
		}
		catch ( ... ) {}

		return false;
	}

	bool WriteBuffer ( XMP_IO* fileRef, XMP_Uns64 & pos, XMP_Uns32 len, const char * inBuffer )
	{
		try
		{
			if ( (fileRef == 0) || (inBuffer == 0) ) return false;
		
			fileRef->Seek ( pos, kXMP_SeekFromStart );
			fileRef->Write( inBuffer, len );
		
			return true;
		}
		catch ( ... ) {}

		return false;
	}

} // namespace GIF_Support
