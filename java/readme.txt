Adobe XMP Toolkit for Java Version 4.1.1
========================================

It contains the XMPCore part of the toolkit and NOT XMPFiles.
To get more information about the "Extensible Metadata Platform" (XMP), 
please visit the XMP product page on the Adobe website (http://www.adobe.com/xmp).

This readme.txt covers the setup of the XMPCore and the example project
for the Eclipse Java IDE 3.0 and above and Java SDK 1.4.2 and above.


Setup the projects in Eclipse 3.2 and above:

1. Start Eclipse with an empty workspace
2. In the menu select File --> Import...
   --> Existing Projects into Workspace --> Next
3. Select "Select root directory" and browse for this directory (XMP-SDK/java)
4. Press <Finish>


Setup the projects in Eclipse 3.0.x:

1. Start Eclipse with an empty workspace
2. In the menu select File --> New --> Project --> Java Project
3. Enter Project Name "XMPCore"
4. Select "Create project at external location" and select the folder "XMPCore" 
   which you find as sibling of this readme.txt file.
5. Press <Finish>
6. To install the example please repeat steps 2. to 5. replacing "XMPCore" by "XMPCoreCoverage"


To build debug and release libraries of XMPCore, run the ANT script "build.xml" 
that is contained in the XMPCore project.

Note: If you use Java 1.4.2, please ensure that the class file compliance is set to 1.4 
(default is 1.2). Otherwise the assert statements do not compile.
To change this setting, open the Preferences dialog and select --> Java --> Compiler:
Uncheck "Use default compliance settings" and set "Generated .class files compatibility" 
and "Source compatibility" both to 1.4.