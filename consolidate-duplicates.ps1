param(
    [Parameter(Mandatory=$false)]
    [string]$Root = ".",
    [switch]$WhatIf
)

# --- 基本設定 ---
$ExcludeDirs = @("bin","obj",".git")  # 必要なら追加
$ProjPatterns = @("*.vcxproj","*.vcxproj.filters")

Write-Host "Root: $Root"
Write-Host "DryRun: $WhatIf"

# Helper: 相対パスを取得（.NET Core / PowerShell 7 の場合）
function Get-RelativePath($from, $to) {
    return [System.IO.Path]::GetRelativePath($from, $to)
}

# 1) 全ファイルを集める（除外ディレクトリを除く）
$files = Get-ChildItem -Path $Root -Recurse -File |
    Where-Object {
        foreach($ex in $ExcludeDirs) {
            if ($_.FullName -match [regex]::Escape("\$ex\")) { return $false }
        }
        return $true
    }

# 2) 同名ファイルでグループ化
$nameGroups = $files | Group-Object -Property Name | Where-Object {$_.Count -gt 1}

if ($nameGroups.Count -eq 0) {
    Write-Host "同名ファイルは見つかりませんでした。"
    exit 0
}

foreach ($g in $nameGroups) {
    Write-Host "`n== 処理: $($g.Name) （候補 $($g.Count) 件） =="

    # 2a) 各ファイルのハッシュを計算して同一内容ごとに更にグループ化
    $hashGroups = @{}
    foreach ($fi in $g.Group) {
        $h = (Get-FileHash -Algorithm SHA256 -Path $fi.FullName).Hash
        if (-not $hashGroups.ContainsKey($h)) { $hashGroups[$h] = @() }
        $hashGroups[$h] += $fi
    }

    foreach ($kv in $hashGroups.GetEnumerator()) {
        $hash = $kv.Key
        $items = $kv.Value
        if ($items.Count -lt 2) {
            # 同名だが内容が違うファイルのみ -> スキップ（手動確認を推奨）
            Write-Host " - ハッシュ $hash : ユニーク（内容異なる） => スキップ"
            continue
        }

        Write-Host " - ハッシュ $hash : $($items.Count) 件 が同一内容"

        # 3) 残すファイルを選択（最短パス＝親フォルダ側優先）
        $keep = $items | Sort-Object { $_.FullName.Length } | Select-Object -First 1
        Write-Host "   -> KEEP: $($keep.FullName)"

        # 4) 置換対象（削除）リスト
        $toRemove = $items | Where-Object { $_.FullName -ne $keep.FullName }

        # 5) プロジェクトファイル内のパス参照を置換
        $projFiles = @()
        foreach ($pattern in $ProjPatterns) {
            $projFiles += Get-ChildItem -Path $Root -Recurse -Filter $pattern -File -ErrorAction SilentlyContinue
        }
        foreach ($proj in $projFiles) {
            $projDir = $proj.DirectoryName
            $text = Get-Content -Raw -Path $proj.FullName -ErrorAction SilentlyContinue
            if (-not $text) { continue }

            $modified = $false
            foreach ($dup in $toRemove) {
                # プロジェクト内で使われる可能性のある相対パスを生成
                $dupRel = Get-RelativePath($projDir, $dup.FullName) -replace '/', '\'
                $keepRel = Get-RelativePath($projDir, $keep.FullName) -replace '/', '\'

                # さらにファイル名のみ参照されている場合の対処（単純置換）
                $dupName = [System.IO.Path]::GetFileName($dup.FullName)
                $keepName = [System.IO.Path]::GetFileName($keep.FullName)

                # 正確一致を優先して置換
                if ($text -match [regex]::Escape($dupRel)) {
                    $text = $text -replace [regex]::Escape($dupRel), $keepRel
                    $modified = $true
                    Write-Host "     プロジェクト $($proj.Name) : $dupRel -> $keepRel"
                } elseif ($text -match [regex]::Escape($dupName)) {
                    # ファイル名だけの記述を親側に置換（注意：同名ファイルが他にあると影響する）
                    $text = $text -replace "(?<![\\\/])" + [regex]::Escape($dupName) + "(?![\\\/])", $keepRel
                    $modified = $true
                    Write-Host "     プロジェクト $($proj.Name) : ファイル名参照 $dupName を $keepRel に置換"
                }
            }
            if ($modified) {
                if ($WhatIf) {
                    Write-Host "     (DRY RUN) 変更を破棄します: $($proj.FullName)"
                } else {
                    # 保存（上書き）
                    Set-Content -Path $proj.FullName -Value $text -Encoding UTF8
                    Write-Host "     保存しました: $($proj.FullName)"
                }
            }
        }

        # 6) 実ファイルの削除（または dry-run 表示）
        foreach ($d in $toRemove) {
            if ($WhatIf) {
                Write-Host "   (DRY RUN) 削除予定: $($d.FullName)"
            } else {
                try {
                    Remove-Item -Path $d.FullName -Force
                    Write-Host "   削除: $($d.FullName)"
                } catch {
                    Write-Warning "   削除失敗: $($d.FullName) - $_"
                }
            }
        }
    }
}

Write-Host "`n完了。プロジェクトをビルドして問題がないか確認してください。"