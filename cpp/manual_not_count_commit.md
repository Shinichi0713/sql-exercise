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
