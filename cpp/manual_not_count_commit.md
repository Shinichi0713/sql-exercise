はい、**別のアカウント（または別のGit Author情報）を使ってコミットを作成・Pushし、自分のアカウントの草（Contributionグラフ）やコミット履歴に反映させないようにすることは可能**です。

GitHub（およびGit）において「誰のコミットか」を判定する仕組みと、それを回避する設定方法について解説します。

---

### GitHubがコミットをアカウントに紐付ける仕組み

GitHubは、コミットに含まれる **Gitのメールアドレス（User Email）** を見て、どのGitHubアカウントのコミット（草・Contribution）かを判定しています。

つまり、ローカル環境のGitのメールアドレス設定が自分のGitHubアカウントに紐づいていないアドレスになっていれば、そのコミットは自分のアカウントの履歴やContributionとしてカウントされません。

---

### 具体的な対処法・手順

#### 方法1: ローカルGit設定のメールアドレスを変更する（最も簡単・確実）

特定のレポジトリでのみ、自分のGitHubアカウントとは無関係なメールアドレス（または別アカウントのメールアドレス）を設定してコミットします。

1. **対象のレポジトリに移動する**
2. **そのレポジトリ限定でUser Emailを変更する**
```bash
git config local user.name "別のアカウント名（任意）"
git config local user.email "別のアカウントのメールアドレス（または無関係なアドレス）"

```


* `--global` ではなく `--local` を使うことで、そのプロジェクト（レポジトリ）のみの設定になります。


3. **設定を確認する**
```bash
git config user.email

```


4. **通常通りコミット・Pushする**
* コミットの作成者（Author）が変更したメールアドレスになるため、自分のGitHubアカウントの草（Contribution）には反映されなくなります。



---

#### 方法2: 別のGitHubアカウントとしてPush・PR作成を行う

完全に別のアカウントとしてGitHub上に履歴を残したい場合は、別アカウントの認証情報（SSHキーまたはPersonal Access Token）を使って操作します。

1. **ローカルのGit設定を変更（方法1と同様）**
* 別アカウントの登録メールアドレスに設定します。


2. **SSH接続設定（`~/.ssh/config`）の切り替え**
* 自分のメインアカウントとは別のSSHキーを設定し、別アカウント権限でGitHubへPushします。



```text
# ~/.ssh/config の記述例
Host github.com-sub-account
  HostName github.com
  User git
  IdentityFile ~/.ssh/id_ed25519_sub

```

```bash
# クローンやリモートURLの指定時にHost名を変更
git remote set-url origin git@github.com-sub-account:ユーザー名/レポジトリ名.git

```

---

### 注意点・補足

* **既存コミットの差し替えについて:**
過去に自分のメールアドレスで作成してしまったコミットは、後から `git config` を変更しても勝手には変わりません。過去のコミットのAuthorを変更したい場合は、`git commit --amend` や `git rebase -i` などで過去コミットのAuthor情報を修正（`git commit --amend --author="Name <email>"`）した上で強制Pushする必要があります。
* **レポジトリのアクセス権限:**
別のアカウントで直接Pushする場合は、そのレポジトリに対して別アカウント側にCollaborator権限（Write権限）が付与されているか、またはフォークしてPull Requestを送る必要があります。
* **GPG/SSH署名（Commit Verification）:**
コミット署名（Verifiedバッジ）を設定している場合、鍵の所有者とコミットメールアドレスの一致が必要になりますので、必要に応じて設定をオフにするか別鍵を用意してください。

GitHubでマージやレビューを行った段階でコミット数がカウントされた場合、それは**マージコミット（Merge Commit）の発生**、あるいは**マージされたことによるメインブランチへのコミットの反映**が原因です。

この「マージ時のカウント」も、以下の方法で消去・回避が可能です。

---

### 原因1: マージコミット（Merge Commit）が自分のアカウントで作成された

プルリクエスト（PR）をマージする際、デフォルトの「Create a merge commit」を選択すると、**マージ操作を行った人（レビュアー）のアカウント名で新しい「マージコミット」が1つ作成**されます。このマージコミットが原因で、あなたのContributions（草）やコミット数としてカウントされます。

#### 対処法 A: コミットを残さないマージ方式を使う

PRをマージする際、マージボタンのドロップダウンから以下のいずれかを選択します。

* **Rebase and merge:**
マージコミットを一切作成せず、トピックブランチのコミットをターゲットブランチに直線状につなぎ替えます。マージ操作をした人（あなた）のコミットは発生しません。
* **Squash and merge:**
トピックブランチの全コミットを1つにまとめてマージします。この際、**コミットのAuthor（作成者）はPRの作成者のまま**になります。マージボタンを押した人（あなた）のコミットとしてはカウントされません。

#### 対処法 B: リポジトリの設定で「Merge commit」を無効化する

リポジトリの `Settings` > `General` > `Pull Requests` セクションで、**「Allow merge commits」のチェックを外し**、「Allow squash merging」または「Allow rebase merging」のみを許可するように設定します。これにより、今後誤ってマージコミットを作成してしまうのを防げます。

---

### 原因2: 自分のアカウントで作成した過去のコミットがメインブランチに取り込まれた

PR作成者（あるいはブランチのコミット作成者）のメールアドレス設定があなたのメインアカウントのままだった場合、PRをマージしたタイミングで、それらのコミットがターゲットブランチ（`main`など）に反映され、まとめてContributions（草）としてカウントされます。

#### 対処法: 履歴から自分のコミットを消す（すでにマージ済みの場合）

すでにマージされてリモートの`main`ブランチなどに反映されてしまった場合は、**ブランチの履歴を遡ってAuthor情報を書き換えた上で、強力に強制Push（Force Push）** する必要があります。

1. **`git filter-repo` または `git rebase` で過去コミットのAuthorを変更する**
ローカルで対象ブランチ（例: `main`）に移動し、問題のコミットのAuthorメールアドレスを別のアドレスに書き換えます。
* **直近のマージコミットのみ書き換える場合:**
```bash
git checkout main
git pull
git commit --amend --author="別名 <sub-account-email@example.com>" --no-edit

```


* **過去数個分のコミットのAuthorを一括で書き換える場合（`git rebase -i`）:**
```bash
git rebase -i HEAD~5

```


エディタが開いたら、書き換えたいコミットの行を `pick` から `edit`（または `e`）に変更して保存・終了します。
止まったコミットごとに以下を実行して進めます。
```bash
git commit --amend --author="別名 <sub-account-email@example.com>" --no-edit
git rebase --continue

```




2. **リモートへ強制Pushする**
```bash
git push --force-with-lease origin main

```



---

### まとめ

* **今後マージする際:** PRマージ時に **Squash and merge** または **Rebase and merge** を使う（マージコミットを発生させない）。
* **すでにカウントされてしまった分を消したい場合:** `git commit --amend --author="..."` や `git rebase -i` で過去コミットのメールアドレスを変更し、`git push --force` でリモート履歴を上書きする。

※メインブランチへの Force Push は、チーム開発を行っている場合は他のメンバーの環境と不整合を起こすリスクがあるため、作業前にバックアップ用のブランチを作成しておくことを推奨します。


