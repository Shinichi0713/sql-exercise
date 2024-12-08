# 問題

書籍一覧をカテゴリー毎に集計して多い順に、カテゴリー名は昇順に並び替えてください。データの取得件数は先頭から3件のみにしてください。
出力項目はname(カテゴリー名)とnum(書籍数)です。

# テーブル


| id   | name             | release_year | total_page |
| ---- | ---------------- | ------------ | ---------- |
| 1111 | aaa              | 2024         | 500        |
| 2222 | sss              | 2020         | 300        |
| 3333 | aaa              | 2034         | 600        |
| 1    | コードと回路     | 2017         | 245        |
| 2    | 宇宙の歴史       | 2019         | 314        |
| 3    | 星の記憶         |              | 181        |
| 4    | リファクタリング | 2004         | 315        |
| 5    | 時短レシピ100    | 2008         | 298        |
| 6    | 世界一周マップ   | 2005         | 187        |


## 結果


![1731932642768](image/problem/1731932642768.png)


```
select categories.name, count(categories.name) as num from books 
left join book_categories on books.id = book_categories.book_id 
left join categories on categories.id = book_categories.category_id 
group by categories.name 
order by num desc,  categories.name 
limit 3;
```

retry
```sql
select categories.name, count(categories.name) as num from books 
left join book_categories on books.id=book_categories.book_id 
left join categories on book_categories.category_id=categories.id 
group by categories.name 
order by num desc, categories.name 
limit 3;
```

## サマリー

* テーブルの結合はjoin(優先するテーブルが右ならright、左ならleft)
* グループ化したいならばgroup by
* 並び替えするならばorder by
* 表示件数を絞るならばlimit


## エラー
>ERROR:  column "categories.name" must appear in the GROUP BY clause or be used in an aggregate function
>LINE 1: select categories.name, count(categories.name) from books  l...

"""
このエラーは、SQLのクエリで GROUP BY 句を使用する際に、SELECT 句に含まれるすべての列が GROUP BY 句に含まれていないために発生しています。具体的には、categories.name 列が GROUP BY 句に含まれていないためです。
つまり
countをつかう場合は、group byでグループ化が必要。
"""
