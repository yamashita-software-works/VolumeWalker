### 0.1.28.0

- Volume Information,Physical Drive Information,Drive Layoutで更新(Refreshコマンド)できない不具合を修正。

- ソースコードの整理・同期、不具合修正。

- fsvolumehelp.dllのヒープメモリ使用方法の見直し。

### 0.1.27.0

- Volume Filesウィンドウで検索ダイアログが開かない不具合を修正。

- Aboutダイアログに表示される情報を"Windows Version"から"System Information"に整理・変更。

- その他、ソースコードの整理。

### 0.1.26.0

- Disk Performance ウィンドウを試験的に追加。   
Volumes,MS-DOS DrivesまたはPhysical Drivesウィンドウからボリュームまたはドライブを選択して開きます。   
IOCTL_DISK_PERFORMANCE I/Oコントロールで得られた値をそのまま表示します。

### 0.1.25.0

- ボリューム内のファイルを参照するウィンドウを追加。

### 0.1.24.0

- ビルド環境、ソースコードの整理。

### 0.1.23.0

- ビルド環境、ソースコードの整理。

- Aboutダイアログのシステム情報の表示順を調整。

### 0.1.22.0

- サブツールとして Volumeinfo.exe を公開ビルドに組み込み。   
VolumeWalkerウィンドウの一部をSDI Frameに組み込んだ簡易版です。
VolumeWalker本体と異なりシステムでダークモードを設定した場合、メインウィンドウのみダークモード表示されます（ダイアログボックスは除く）。

- その他軽微な不具合修正。

### 0.1.21.0

- ビルド環境整理。   

- メニュー項目並びの調整。

### 0.1.20.0

- ビルド環境整理のみ。   
  フォルダの名称変更 fsvoumewalker/ ⇒ volumewalker/    
  機能の追加、変更等はありません。

### 0.1.19.0

- 実行ファイルの名称を変更。   
 fsvoumewalker.exe ⇒ volumewalker.exe

- Volume Contents Browserの仕様を全面見直しの為機能を一旦削除。プロジェクトからも削除。   
  0.1.8.0以来含まれていたfsvolumecontents.dllは削除されました。

### 0.1.18.0

- Volume Informationにボリューム使用量バー表示の表示不具合を修正。

- Volume Contents Browser - Filesウィンドウの仕様を変更するために一部機能を制限・再構成。   
ComboBoxは無効化されました。

- 他プロジェクトと同期の為にソース・ライブラリ整理。

### 0.1.17.0

- Volume Informationにボリューム使用量バー表示行（ Usage% 行 ）を追加。

### 0.1.16.0

- Volume Contents Browser - Filesウィンドウを更新。

- ウィンドウレイアウトを保存して終了後、次回起動時にPhysical Drive Informationの参照していたドライブが存在しないと起動できない不具合を修正。

- ウィンドウレイアウト保存モード時で終了する時、SimpleHexDumpが開いているとヒープ破壊エラーを起こす不具合を修正。

### 0.1.15.0

- FDドライブが存在するPCで実行した時、ディスクが挿入されていない状態でアクセスする際に表示されるエラーメッセージを抑止。

- Volume Listの列に\[Creation Time\]を追加。

- Volume InformationのGeneralグループにDeviceType項目を追加。

- プロセスのプレースホルダー互換性モードを開示に設定。   
Volume Contents Browser - Filesウィンドウの表示に影響します。

- AboutダイアログにWindows Product名とSystem Install Dateを追加。

- 暫定的にWindows Vistaでも実行可能にした。（永続的なものではありません）

- その他、ソース・ライブラリ整理。

### 0.1.14.0

- MS-DOS DrivesウィンドウからOpen Locationを実行するとクラッシュする不具合を修正。

- 32bit版の Volume Contents Browser - Filesウィンドウで C:\を参照できない不具合を修正。

- Volume Contents Browser - Filesウィンドウの [属性]列のソートが、HexまたはBit表示モードの時適切ではない不具合を修正。

- 32bit版でDisk Drive Layoutを開くとアプリケーションエラーで終了する不具合を修正。

- 簡易ダンプウィンドウで、データが読み込みエラーになると、ツールバーのGotoボタンも無効化されて移動ができなくなる不具合を修正。

- 簡易ダンプウィンドウで、バイトデータの0x20(空白)が[Character]列で'.'と表示されている不具合を修正。

- Volume Informationウィンドウの NTFSグループ,Mft2StartLcn項目のラベル綴り誤りを修正（"Mft2StartLcnt"->"Mft2StartLcn"）。

- バージョン情報ダイアログにx64またはx86の表記を追加。

- その他、ソース・ライブラリ整理。

### 0.1.13.0

- Volume Contents Browser - Files ヒストリ機能を実験的に追加。

- Volume Contents Browser - Files カラムローダーのメモリリークを修正。

- Storage Devicesの以下の列タイトルを変更。   
  "Last Arrival Date"-> "Arrival Date"   
  "Last Removal Date"-> "Removal Date"   

- コメント内の誤記"Auther"を"Author"に訂正。

- Physical Drive Information のメモリ（m_pvi）がリークしていた不具合を修正。

### 0.1.12.0

- VolumesとMS-DOS Drivesの\[Usage Rate\]列の名称を\[Usage %\]に変更。

- MS-DOS DrivesからContents Browser - Filesを開く場合、NTデバイス名ではなくドライブ名を使用する様に変更。

- MS-DOS Drivesでボリューム容量が0(CD Audioの様な)の場合、Usage%が誤って表示される不具合を修正。

- VolumesとMS-DOS Drivesに Open Locationコマンドを追加。  
  Explorer,Terminal,PowerShell,Prompt,Bashの選択を用意。

- グループビューが有効のウィンドウ（Volume/Drive Information,Storage Devices,Minifilter Driver)で、グループ項目にフォーカスを置いた時にSpace,Enterキーでグループを開閉できる様にした。

- SimpleHexDumpでCD Audioを読み込んだ時の挙動を修正。

- SimpleHexDumpのツールバーの挙動を整理。

- Contents Browser - Files
  32767文字パス(16383階層)のディレクトリから上のディレクトリに移動した際、誤った位置に移動してしまう不具合を修正。

- Contents Browser - Files
  以下の列にドロップダウンボタンを設けて列の表示内容を変更できる様にした。   
   \[Attributes\]   
   \[Size\]   
   \[Allocation Size\]   
   \[Date\]   
   \[Creation Time\]   
   \[Last Access Time\]   
   \[Change Time\]   

- Contents Browser - Files のGotoコマンドで"."を入力するとタイトルバーにテキストが表示されなくなり、構成保存が有効な場合 iniファイルに不正なパスが保存されて次回起動時にエラーになることがある不具合を修正。

- Contents Browser - Change Journal Viewer のコンテキストメニューが無効化のままだった不具合を修正。

### 0.1.11.0

- Volume Information,Disk Drive Information ウィンドウで、ウィンドウ内テキスト検索をした時に、一致したテキストが含まれる行を降順（または昇順）に正しく選択しない不具合を修正。

- Volume Informationのグループ名 'Generic' を 'General' に変更。

### 0.1.10.0

- ソフトウェアの名称をVolumeWalkerに変更しました。

- Storage Devicesウィンドウに\[Last Arrival Date\]\[Last Removal Date\]列を追加。

- Storage DevicesウィンドウでVolumesグループを追加（従来コメントアウトされていたものを有効化）

- Mounted Deviceの\[identifier\]列のソートが実装されていなかった不具合を修正。

- ウィンドウレイアウトの保存機能を仮実装。   
  オプション -we を付けて起動すると終了時にexeと同じ場所にlayout.iniを作成します。次回-we を付けて起動すると保存したウィンドウレイアウトを読み込みます。

- 引数で言語IDを指定できる様にした（暫定機能）。

### 0.1.9.0

- 全ウィンドウに簡易的なウィンドウ内テキスト単純検索機能を追加。

- Contenrt Browser-Fileを調整・更新。

- Contenrt Browser-Change Jouranlを調整・更新。

### 0.1.8.0

- Volume Contents Browserウィンドウを暫定追加。   
 （注：限定的な実装です。将来削除されるかもしれません。）

  - Files   
    選択されたボリュームに含まれるファイルをディレクトリ単位で簡易表示します。
  - Change Journal   
    選択されたボリュームの変更ジャーナルを表示します。   
    管理者モードで実行時（またはボリュームをFILE_READ_DATAでオープンできた場合）のみ表示されます。

- AboutボックスにWindows Versionを追加。
  (FSWorkbenchと類似のもの)

- DiskLayoutでMBRパーティションのGAP開始位置の表示を修正。

### 0.1.7.0

- Volume Informationウィンドウに "Virtual Disk"グループを追加。   
  ボリュームやドライブが仮想ディスクの場合表示されます。   
  ※現状Dev Driveでアタッチされたの仮想ドライブを認識できません。   

- Disk Drive Informationウィンドウに "Virtual Disk"グループを追加。   
  ドライブが仮想ディスクの場合表示されます。

- Disk Drive InformationウィンドウのBasicグループにBusTypeを追加。

- Volumes,MS-DOS Drives,Physical Drivesの各ウィンドウで仮想ボリューム、ドライブの項目を識別できる様にした。   
  現在は青文字で表示されます。   
  ※現状Dev Driveでアタッチされたの仮想ドライブを認識できません。   

- Volumesウィンドウで、Dirtyビットがセットされているボリュームの配色を調整。

- MS-DOS DrivesウィンドウでもDirtyビットがセットされているドライブを識別できる様にした。

- SimpleHexDumpとMountedDeviceのリストカラムにあった不要なスプリットボタン属性を削除。

### 0.1.6.0

- ボリューム情報に "USN Journal Data"グループを追加。   
  管理者モードで実行時（またはボリュームをFILE_READ_DATAでオープンできた場合）のみ表示されます。

- Volumes,Volume Informationウィンドウで、メディアのセットされていない光学デバイスドライブ(CD/DVD/BDなど)にアクセスすると、ウィンドウを開くたびにボリュームハンドルがリークするする不具合を修正。

- Volumes,Volume Informationウィンドウで、メディアのセットされていない光学デバイスドライブ(CD/DVD/BDなど)の情報を一部取得できていなかった不具合を修正。

### 0.1.5.0

- ボリューム情報に "System Control"グループを追加。

- ボリューム情報に "Quota"グループを追加。   
  管理者モードで実行時（またはボリュームをFILE_READ_DATAでオープンできた場合）のみ表示されます。

- Sector/Cluster Dumpウィンドウを暫定追加。   
  管理者モードで実行時（またはボリュームをFILE_READ_DATAでオープンできた場合）のみ利用可能です。Volumes,MS-DOS Drives,Physical Drives,Disk Drive Layout各ウィンドウから開くことができます。   
 （注：限定的な実装です。将来削除されるかもしれません。）

- Minifilter Driverウィンドウを暫定追加。管理者モードで実行時のみ利用可能。   
 （注：限定的な実装です。将来削除されるかもしれません。）

### 0.1.4.0

- File System Statisticsウィンドウを追加。   
  VolumesまたはMS-DOS Drivesウィンドウでボリュームやドライブを選択し、メニューまたはコンテキストメニューから\[File System Statistics\]を選択します。

### 0.1.3.0

- DLLの名称変更。 fsvolumeinfo.dll -> fsvolumehelp.dll

- CSimpleValArrayはATLとクラス名が重複するため、CValArrayに変更。

- Disk Drive Information ウィンドウでパーティション情報にDOSドライブレターが表示されていなかった不具合を修正。

- コード整理、細かい不具合修正など。

### 0.1.2.1

- Aboutダイアログに表示されるSystem Boot Timeを、日時をローカル時間で取得した後、再度ローカル時間に変換して表示していた不具合を修正。

### 0.1.2.0

- VolumeList,DriveListに\[Usage\] (使用量)列を追加。

- Volume InformationでReFSのボリュームを表示した場合、REFS_VOLUME_DATA_BUFFER情報の表示を追加。

- DriveLayoutでのメモリリークを修正。

- 共通ライブラリを更新。

- その他コード整理。

### 0.1.1.0

- ボリュームや物理ドライブから開く情報ウィンドウを、ボリューム/ドライブ毎の単一ウィンドウ（シングルトン）に変更（以前は同じボリューム/ドライブのウィンドウを複数開くことができた）。

- MDI子ウィンドウを最大化している時に新しいウィンドウを開
いた時、メインメニューがちらつく現象を回避。

- その他コード整理、スペルミス修正など。 

### 0.1.0.0

- GitHubに公開。

