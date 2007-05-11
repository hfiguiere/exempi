The XMP Files handler for .mov files uses QuickTime. 
This will always be available for build and use on Macintosh. 
For Windows, Apple's QuickTime SDK version 7 and above is needed to build XMP Files 
and the public QuickTime Player is needed in order to use XMPFiles for .mov files. 
The QuickTime Player also installs the necessary QuickTime library.

To build XMP Files for Windows, first obtain a copy of the QuickTime SDK for Windows from
    http://developer.apple.com/sdk/

Unpack the SDK to any convenient location. Copy the contained CIncludes and Libraries folders
to the XMP development tree under .../third-party/QTWinDev. 
That is, the CIncludes and Libraries folders should appear as siblings of this ReadMe.txt file.
