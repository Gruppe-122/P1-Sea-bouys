## Kom igang med projektet
Her en lille guide til at hvordan man får det sat op med git. Guiden forudsætter af Git allerede er installeret på computeren
Først  skal man initiere projektet i den mappe man gerne vil bruge f.eks bruger\dokumenter
```
git init
```

Hent projekt ned på computer, aka klon repositoriet ned til computeren
```
git clone https://github.com/Gruppe-122/P1-Sea-bouys.git
```

Tilføje det repo man følger med i/tracker
```
git remote add origin https://github.com/Gruppe-122/P1-Sea-bouys.git
```

Vi commiter ændringer i projektet ved at lave en ny branch
```
git checkout -b feature/feature_name
```

For at vælge ændringer man gerne vil committe aka at "stage changes"
```
git add fil_der_er_blevet_ændret
```

For at sætte disse ændringer igang skal man committe dem
```
git commit -m "Deskription af ændringer og tilføjelser til fil/filer"
```

Nu er den branch man arbejder på blevet ændret
```
git push -u feature/feature_name
```

Dernæst skal man ind i Github under Pull request
-> Tryk New pull request -> Vælg main branch og feature/feature_name -> Tilføj en reviewer og send afsted
