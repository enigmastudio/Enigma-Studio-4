del eplayer4_packed.exe
del eplayer4_bep.exe
..\tools\kkrunchy.exe eplayer4.exe --new --out eplayer4_kkrunchy.exe --best --refsize 64
java -jar ./KKrunchyAnalyzer.jar eplayer4.kkm
..\tools\bep.exe eplayer4.exe eplayer4_bep.exe +ca=ctx1 +x +sphe -fuls
pause