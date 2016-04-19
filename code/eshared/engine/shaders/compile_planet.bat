..\..\..\..\tools\shader_minifier.exe  --hlsl --preserve-all-globals -o ps_fx_planet.hpp ps_fx_planet.hlsl
..\..\..\..\tools\shader_minifier.exe --hlsl --preserve-all-globals --shader-only -o test_planet.hlsl ps_fx_planet.hlsl
"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Utilities\bin\x86\fxc" /O3 /Tps_5_0 test_planet.hlsl

pause