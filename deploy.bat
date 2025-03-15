set fo4dir=e:\games\Fallout4\overwrite
set builddir=.\build\release-msvc\release
copy %builddir%\*.dll %fo4dir%\F4SE\plugins\
copy %builddir%\*.pdb %fo4dir%\F4SE\plugins\
