# IPK - Projekt 1

Projekt predstavuje jednoduchý server prijmajúci HTTP požiadavky troch typov - hostname, cpu-name a load. Požiadavok hostname vracia doménové meno. Požiadavok cpu-name vracia názov procesoru zariadenia, na ktorom pracuje server a požiadavok load vracia momentálne vyťaženie tohto procesoru. Aplikácia je podporovaná pre Linux Ubuntu 20.04 LTS.

## Preloženie a spustenie

V adresári sa nachádza Makefile, ktorý slúži na preloženie zdrojového kódu programu. Pre preloženie zdrojového kódu stačí napísať:

```
$ make
```

Pre spustenie servera zadajte príkaz

```
$ ./hinfosvc port
```
, kde port je číslo portu, na ktorom bude server príjmať požiadavky.
Príklad preloženia a spustenia teda vyzerá následovne:

```
$ make
$ ./hinfosvc 12345
```

Teraz by mal byť server spustený.

##  Posielanie požiadavkov na server

Na spustený server je možné zasielať HTTP požiadavky cez webové prehliadače alebo cez nástroje ako wget a curl.
Príklady požiadavkov:

```
$ GET http://servername:12345/hostname
$ GET http://servername:12345/cpu-name
$ GET http://servername:12345/load
```

## Titulky

Projekt vypracoval Hugo Hežel ako školskú prácu pre predmet Počítačové komunikace a sítě na Fakulte Informačních Technológií VUT v Brně.