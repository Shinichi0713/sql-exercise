# 問題

書籍一覧をカテゴリー毎に集計して多い順に、カテゴリー名は昇順に並び替えてください。データの取得件数は先頭から3件のみにしてください。
出力項目はname(カテゴリー名)とnum(書籍数)です。

# テーブル

id	name	release_year	total_page
1111	aaa	2024	500
2222	sss	2020	300
3333	aaa	2034	600
1	コードと回路	2017	245
2	宇宙の歴史	2019	314
3	星の記憶		181
4	リファクタリング	2004	315
5	時短レシピ100	2008	298
6	世界一周マップ	2005	187


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
