Adobe XMP Toolkit for Java Version 4.4.0
========================================

For information about the Extensible Metadata Platform (XMP), 
visit the XMP product page on the Adobe website: http://www.adobe.com/xmp.

The Java API contains only the XMPCore part of the XMP Toolkit;
it does NOT contain the XMPFiles component.

This file contains instructions for installing the XMPCore Java library
and example project for the Eclipse Java IDE 3.0 and higher,
and Java SDK 1.4.2 to 1.5. The Java SDK 1.6 is not currently supported.


To set up the projects in Eclipse 3.2 and higher:

1. Start Eclipse with an empty workspace
2. Choose  File > Import.
3. In the Wizard, choose Existing Projects into Workspace > Next
3. Click "Select root directory" and browse for the folder XMP-Toolkit-SDK-4.4.0/java
4. Click Finish.


To set up the projects in Eclipse 3.0.x:

1. Start Eclipse with an empty workspace
2. Choose File > New > Project > Java Project
3. Enter the Project Name "XMPCore"
4. Click "Create project at external location" and select the folder "XMPCore" 
   (in the folder that contains this readme.txt file).
5. Click Finish.
6. To install the example, repeat steps 2 to 5, replacing "XMPCore" with "XMPCoreCoverage"


To build debug and release libraries of XMPCore, run the ANT script "build.xml" 
that is contained in the XMPCore project.

Note: If you use Java 1.4.2, make sure that the class file compliance is set to 1.4.
Otherwise, the assert statements do not compile. To change this setting:
1. Open the Preferences dialog and select Java > Compiler.
2. Uncheck "Use default compliance settings"
3. Set both "Generated .class files compatibility" and "Source compatibility"  to 1.4.
