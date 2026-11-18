SUF読み込みの回帰テストです。Visual Studioの「x64 Native Tools Command Prompt」で、プロジェクトのルートから次を実行します。

```bat
if not exist .verification mkdir .verification
cl /nologo /std:c++20 /EHsc /MDd /D_AFXDLL /D_UNICODE /DUNICODE /utf-8 /I. /Fo.verification\ /Fe.verification\AssemblyTests.exe tests\AssemblyTests.cpp PartAssembly.cpp glsuf\SufModel.cpp /link /SUBSYSTEM:CONSOLE
.verification\AssemblyTests.exe
```

終了コード0で成功です。SUF追加、引用符ありのL3P・引用符なしの旧L2P、相対パス、移動・回転・拡大縮小後の範囲、空のSUF・存在しないSUFの拒否を確認します。
