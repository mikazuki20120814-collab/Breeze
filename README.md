# Breeze

CEF（Chromium Embedded Framework）+ C++ で作る Windows 用 Web ブラウザです。

## 機能
- タブ（追加 / 閉じる / 切り替え）、ページタイトルをタブに表示
- アドレスバー（URL 以外を入力すると Google 検索）
- 戻る / 進む / 更新（読み込み中は Stop）/ ホーム
- `target=_blank` や `window.open` は新しいタブで開く
- ダウンロード（保存ダイアログ付き）
- 開発者ツール（F12）
- プロファイル保存先: `%LOCALAPPDATA%\Breeze\User Data`

## ショートカット
| キー | 動作 |
|---|---|
| Ctrl+T | 新しいタブ |
| Ctrl+W | タブを閉じる |
| Ctrl+L | アドレスバーへ移動 |
| Ctrl+R / F5 | 更新 |
| Ctrl+Tab | 次のタブ |
| Alt+← / Alt+→ | 戻る / 進む |
| F12 | 開発者ツール |

## 必要なもの
1. Visual Studio 2022（「C++によるデスクトップ開発」ワークロード）
2. CMake 3.21 以上
3. Inno Setup 6（インストーラー作成用）
4. CEF 標準配布版 **Windows 64-bit**（バージョン **131 系** を推奨）
   https://cef-builds.spotifycdn.com/index.html から "Standard Distribution" を取得し、
   例として `C:\cef\cef_binary_131...` に展開します。

## ビルド手順
```bat
cd Breeze
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCEF_ROOT=C:/cef/cef_binary_131...
cmake --build build --config Release
build\Release\Breeze.exe
```

## インストーラー作成
```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\breeze.iss
```
`installer\Output\BreezeSetup.exe` ができます。

## 注意
- CEF の API はバージョンで変わります。131 系以外では `OnBeforePopup` や
  `OnBeforeDownload`、`GetBrowserRuntimeStyle` のシグネチャ調整が必要になることがあります。
- サンドボックスは無効（`no_sandbox = true`）です。
- ビルドエラーが出たら、そのメッセージを貼ってください。修正します。

## GitHub Actions でクラウドビルド（PCにVisual Studio不要）
`.github/workflows/build.yml` が、CEF 131.4.1 を自動でダウンロードしてビルドし、`BreezeSetup.exe` まで作ります。

1. GitHub で新しいリポジトリを作る（Private でOK）
2. このフォルダの中身をそのままアップロードする（`.github` フォルダも含める）
   ```bat
   cd Breeze
   git init
   git add .
   git commit -m "Breeze"
   git branch -M main
   git remote add origin https://github.com/<ユーザー名>/<リポジトリ名>.git
   git push -u origin main
   ```
3. リポジトリの **Actions** タブ → "Build Breeze" を開く（push で自動実行。手動なら "Run workflow"）
4. 緑のチェックで完了したら、実行結果ページ下部の **Artifacts** から
   - `BreezeSetup` … インストーラー
   - `Breeze-portable` … インストール不要版
   をダウンロードする

赤い×になったら、失敗したステップのログを貼ってください。
