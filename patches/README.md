# ImGui のローカル変更

`third_party/imgui` は外部リポジトリのサブモジュールです。
親リポジトリのコミットだけではサブモジュール内の未コミット変更を保存できないため、現在の変更を `imgui-local.patch` に保存しています。

対象の ImGui コミット: `76f5be2749ed17f02f0fcef402894340baece0ac`

新しく取得した環境では、glpasm のルートで次のコマンドを実行してください。

```powershell
git submodule update --init --recursive
git -C third_party/imgui apply --check ../../patches/imgui-local.patch
git -C third_party/imgui apply ../../patches/imgui-local.patch
```

既に変更が適用されている環境では、再適用しないでください。次の確認が成功すれば適用済みです。

```powershell
git -C third_party/imgui apply --reverse --check ../../patches/imgui-local.patch
```

適用後はサブモジュールに変更ありと表示されますが、変更内容はこのパッチとして GitHub に保存されています。
