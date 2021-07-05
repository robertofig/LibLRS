@echo off

SET Includes= /I "C:\Program Files\QGIS 3.18\include"
SET Optimizations= /GL /O2 /Oi
SET Libraries=kernel32.lib "C:\Program Files\QGIS 3.18\lib\gdal_i.lib"

pushd ..\..\build
cl ..\code\utils\distcalc-bench.cpp %Includes% /EHa- /GR- %Optimizations% /link %Libraries%
popd
