SUF読み込みの回帰テストです。Visual Studioの「x64 Native Tools Command Prompt」で、プロジェクトのルートから次を実行します。

```bat
if not exist .verification mkdir .verification
cl /nologo /std:c++20 /EHsc /MDd /D_AFXDLL /D_UNICODE /DUNICODE /utf-8 /I. /Fo.verification\ /Fe.verification\AssemblyTests.exe tests\AssemblyTests.cpp PartAssembly.cpp glsuf\SufModel.cpp /link /SUBSYSTEM:CONSOLE
.verification\AssemblyTests.exe
```

終了コード0で成功です。SUF追加、引用符ありのL3P・引用符なしの旧L2P、相対パス、移動・回転・拡大縮小後の範囲、空のSUF・存在しないSUFの拒否を確認します。

材質のテストでは、L3P内パレット、同じSUFへの異なる材質指定、指定なしのSUF材質維持、旧L2Pのatr.iniによる色・質感、1行／複数行ATR、単一値の色、光沢3成分、引用符付き材質名、不正な数値・括弧・成分数の拒否を確認します。模様・凹凸マップや反射の再現は対象外です。


パーツ編集履歴の回帰テストも、同じ開発者コマンドプロンプトから実行できます。

```bat
cl /nologo /std:c++20 /EHsc /MDd /D_AFXDLL /D_UNICODE /DUNICODE /utf-8 /I. /Fo.verification\ /Fe.verification\EditHistoryTests.exe tests\EditHistoryTests.cpp /link /SUBSYSTEM:CONSOLE
.verification\EditHistoryTests.exe
```

終了コード0で成功です。追加・複製・削除・配置変更に相当する状態の復元、選択位置と材質の復元、形状の共有、取り消し後の編集によるやり直し履歴の破棄、100件の履歴上限を確認します。

画面では、一覧で選択したパーツの外周がオレンジ色になること、Shift+Dで同じ位置に複製されること、X／Deleteで削除できること、Ctrl+Z／Ctrl+Shift+Zで配置と選択が戻ることを確認します。数値欄のドラッグまたは連続入力は一連の編集として取り消せます。数値欄でのX／Deleteはパーツ削除に使いません。別の組み立てファイルを開くと履歴を初期化します。
