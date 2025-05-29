Quick Guide: Set Up GTK+ in Code::Blocks (Windows) & play Multiplication Game
Requirements:
Code::Blocks
MSYS2 (Download from https://www.msys2.org)
Step-1:  Install GTK+
Open MSYS2 MinGW 64-bit & run:
pacman -Syu
pacman -S mingw-w64-x86_64-gtk3

Step 2: Configure Code::Blocks

Open Code::Blocks → Settings → Compiler.
Set the Compiler’s installation directory to:
C:\msys64\mingw64


Set paths:

C Compiler: gcc.exe
C++ Compiler: g++.exe
Linker: g++.exe
Make: mingw32-make.exe
(All from C:\msys64\mingw64\bin)



Step 3: Link GTK in the Project
Build Options → Linker settings → Add these libraries:

gtk-3 gdk-3 pangowin32-1.0 pangocairo-1.0 pango-1.0
harfbuzz atk-1.0 cairo-gobject cairo gdk_pixbuf-2.0
gio-2.0 gobject-2.0 glib-2.0 intl

Search directories:
Compiler: C:\msys64\mingw64\include
Linker: C:\msys64\mingw64\lib

Step 5: Play 
