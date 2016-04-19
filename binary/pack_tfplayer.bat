del tfplayer4_packed.exe
del tfplayer4_bep.exe
..\tools\kkrunchy.exe tfplayer4.exe --new --out tfplayer4_kkrunchy.exe --best --refsize 10
java -jar ./KKrunchyAnalyzer.jar tfplayer4.kkm
..\tools\bep.exe tfplayer4.exe tfplayer4_bep.exe +ca=ctx1 +x +sphe -fuls
pause