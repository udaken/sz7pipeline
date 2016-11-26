# sz7pipeline
エクスプローラーで表示している順番で、テキストファイル(.sz7)を作成するツールです。<br>
axpathlist.spi(http://artisticimitation.web.fc2.com/adbtest/test.html) とSusie Plugin対応アプリと組み合わせて使用することで、
エクスプローラーの並び順を保ったまま、Susie Plugin対応アプリで開くことができます。

## 動作について
ツールを起動すると、前面に表示しているエクスプローラーに表示している対象ファイルのリストを取得してテキストファイルを作成します。<br>
作成したファイルを関連付けられたアプリケーションで開きます。(その為、拡張子sz7にアプリケーションを登録しておく必要があります。)

## インストール
1. zipファイルを、適当なフォルダに展開してください。
2. エクスプローラーで `shell:sendto` を開き、 `sz7pipeline.exe` へのショートカットを作成してください。

## 使用方法
エクスプローラーでSusie Plugin対応アプリで開きたいフォルダの中の任意のファイルを選択し、 `送る` から `sz7pipeline.exe へのショートカット` を選択してください。 

## 設定ファイル
sz7pipeline.iniの下記を編集することで、一部動作を変更できます。
* `PATTERN` テキストファイルに含めるファイルを、ワイルドカードで記述します。デフォルトは `*.jpg;*.png;*.gif;*.bmp` です。
すべてのファイルを対象にする場合は `*.*` と。設定してください
* `PATH` テキストファイルを作成する場所です。からの場合、exeと同じ場所に作成します。
* `FILENAME` テキストファイルの名前です。`XXXXXX.sz7` のように設定すると、 *XXXXXX* はランダムな値に置き換わります。

## アンインストール
レジストリは使用しませんので、展開したファイルを削除してください。

## 注意
* フォルダ情報のみ使用するので、エクスプローラーで選択するファイルはどれでも構いません。
* Unicodeには対応していません
(Shift-JISに変換できない文字を含んだファイルには対応していません)

## License
The MIT License (MIT)

Copyright (c) 2016 Kenichi Uda

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
