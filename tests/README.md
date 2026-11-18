SUF読み込みの回帰テストです。Visual Studioの「x64 Native Tools Command Prompt」で、プロジェクトのルートから次を実行します。

```bat
if not exist .verification mkdir .verification
cl /nologo /std:c++20 /EHsc /MDd /D_AFXDLL /D_UNICODE /DUNICODE /utf-8 /I. /Fo.verification\ /Fe.verification\AssemblyTests.exe tests\AssemblyTests.cpp PartAssembly.cpp glsuf\SufModel.cpp /link /SUBSYSTEM:CONSOLE
.verification\AssemblyTests.exe
```

終了コード0で成功です。SUF追加、引用符ありのL3P・引用符なしの旧L2P、相対パス、移動・回転・拡大縮小後の範囲、空のSUF・存在しないSUFの拒否を確認します。

材質のテストでは、L3P内パレット、同じSUFへの異なる材質指定、指定なしのSUF材質維持、旧L2Pのatr.iniによる色・質感、1行／複数行ATR、単一値の色、光沢3成分、引用符付き材質名、不正な数値・括弧・成分数の拒否を確認します。模様・凹凸マップや反射の再現は対象外です。
