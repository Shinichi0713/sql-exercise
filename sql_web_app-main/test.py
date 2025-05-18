import sqlite3
import pandas as pd


path_db = r"C:\Users\yoshinaga\Desktop\sql_web_app-main\chinook.db"

# establish connection
connection = sqlite3.connect(path_db)

# create cursor object
cursor = connection.cursor()

# sql
sql = "select albums.Title, artists.Name from albums join artists on albums.ArtistId = artists.ArtistId where artists.Name = 'Queen';"

# execute sql
cursor.execute(sql)
rows = cursor.fetchall()

# close connection
connection.close()

# show the result
df = pd.DataFrame(rows)
print(df)

