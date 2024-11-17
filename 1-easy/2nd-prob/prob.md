# 問題

書籍一覧をページ数が多い順に並び替えてください。
出力項目はname(書籍名)とtotal_page(総ページ数)です。

# table

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


```
select name, total_page from books order by total_page desc
```

![alt text]({9B4DA8F0-FB66-4F82-A3CF-6D90912AF73C}.png)