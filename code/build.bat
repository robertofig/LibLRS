@echo off

SET CompilerFlags= /EHa- /GR- /W4 /wd4100 /wd4189 /wd4505
SET Includes=/I "C:\Program Files\QGIS 3.18\include"
SET Optimizations= /GL /O2 /Oi
SET LinkerFlags= /OPT:ref /DLL /INCREMENTAL:NO
SET Libraries=kernel32.lib "C:\Program Files\QGIS 3.18\lib\gdal_i.lib"
SET Exports= /EXPORT:Dist2D /EXPORT:Dist3D /EXPORT:DistHaversine /EXPORT:DistLAEllipsoidal /EXPORT:DistLASpherical /EXPORT:GetMeasurementAtPoint /EXPORT:LocatePointFromMeasurement /EXPORT:MeasureLine /EXPORT:PrepareRasters /EXPORT:TransformGRS802DToXYZ /EXPORT:TransformGRS803DToXYZ

IF .%1==.-debug (
   pushd ..\build
   cl ..\code\liblrs.cpp /Z7 %Includes% /LD %CompilerFlags% /DMENSURA_DEBUG /link %LinkerFlags% /PDB:liblrs.pdb %Libraries% %Exports%
   popd
) ELSE (
   pushd ..\build
   cl ..\code\liblrs.cpp %Includes% /LD %CompilerFlags% %Optimizations% /link %LinkerFlags% %Libraries% %Exports%
   popd
)
