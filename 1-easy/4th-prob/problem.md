## 問題

idが1のデータを削除後、イベント一覧を取得してください。


## テーブル


| id | name                       | max_num |
| -- | -------------------------- | ------- |
| 1  | 私達はなぜ世界を旅するのか | 30      |
| 2  | 最新の英語学習法を学ぼう   | 15      |


## コード


```
delete from events where id=1;
select * from events;
```

retry
```sql
delete from events where id=1;
select * from events;
```

## 結果


![1731933220739](image/problem/1731933220739.png)
