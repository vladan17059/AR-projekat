#  7. Implementacija nehronoloskog vracanja unazad u DPLL SAT resavacima
---
* Projekat iz predmeta Automatsko rezonovanje
* Student: Vladan Kovacevic 1013/2021

---
## Zadatak
---
Implementirati ***nehronolosko vracanje*** (***backjumping***) u **DPLL SAT** resavacu. 

## O projektu
---

Kod klasicne DPLL procedure cesto dolazi do suvisnih grananja prilikom obilaska stabla pretrage.
Ubuduce, kad kazemo *nivo* mislice se na *nivo odluke* odn. broj *literala odluke* u trenutnoj parcijalnoj valuaciji.
Pri nailasku na konflikt, ***hronolosko vracanje unazad*** (***backtracking***) vraca se samo jedan nivo nize (ranije) i pokusava da nastavi pretragu negiranjem poslednjeg literala odluke.

To cesto dovodi do suvisnog grananja - cesto uzrok konflikta nije u poslednjem literalu odluke.

Kako bi se ovo donekle izbeglo **CDCL** algoritmi unapredjuju se ***analizom konflikata*** odn. tehnikom za otkrivanje uzroka konflikta.
*Analiza konflikata* polazi od konfliktne klauze: 
- svaki literal koji se nalazi u konfliktnoj klauzi, a nije literal odluke (nije nasom odlukom izabran), menja se literalima zbog kojih je tu dospeo (to moze da se desi jedino primenom pravila ***unit propagation***, *pure literal* necemo koristiti)
- postupak ponavljamo dok u konfliktnoj klauzi ne ostanu samo literali odluke 

Literali odluke koji ostanu u nasoj konfliktnoj klauzi su zapravo "glavni krivci" za konflikt.
Ako su `l1, l2, ..., ln` literali u toj klauzi, znamo da `l1 and l2 and ... and ln` ne sme da bude zadovoljeno, odnosno
`~l1 or ~l2 or ... or ~ln` mora da vazi.

Prethodna klauza zove se ***naucena klauza*** i nju u postupku zvanom ***ucenje*** ukljucujemo u nasu KNF formulu (pocetni skup klauza).
Na osnovu naucene klauze mi vrsimo ***nehronolosko vracanje unazad - backjumping*** do *drugog najviseg nivoa literala u naucenoj klauzi* - i time (nadamo se) izbegavamo veliki broj nepotrebnih grananja. [^1]

Napomenimo da postoji formalnija definicija konfliktne analize i nehronoloskog vracanja unazad.

Ta definicija odnosi se na formiranje ***grafa implikacija*** i izbor pogodnog ***reza*** tog grafa koji odredjuje naucenu klauzu.
Kako pogodan rez ne mora biti jedinstven (postupkom opisanim gore mi dobijamo samo jedan), efikasan izbor tog reza vodjen je heruristikama [^2][^3][^4].

Problem mogu predstavljati naucene klauze koje se gomilaju i usporavaju pretragu, pa je potrebno (takodje vodjeno heuristikama [^3][^4]) 
***zaboraviti neke naucene klauze*** i ***krenuti pretragu ispocetka*** (***forget and restart***).
Ovo nece biti implementirano u projektu.

Takodje, jos jedna korisna heuristika predstavlja izbor literala za grananje. U ovo projektu implementirana je **EVSIDS** (*Exponential Variable State Independent Decaying Sum*) heuristika [^5].

## Implementacija i rezultati
---

Projekat je implementiran u programskom jeziku ***C++***.

Osnovni kod preuzet je sa vezbi[^6] (*dpll.hpp, dpll.cpp, pval.hpp, pval.cpp*) i prilagodjen za implementaciju DPLL resavaca, dok je glavni kod (*cdcl.hpp, cdcl.cpp*) implementiran nezavisno.

Glavni deo koda je resavac (`class CDCL`) u fajlu `cdcl.hpp` koji sadrzi sve bitne komponente (parcijalnu valuaciju, metode, dodatna polja, ...).
Metode su implementirane u fajlu `cdcl.cpp`.
Kod je detaljno iskomentarisan, te nema potrebe ovde objasnjavati tehnicke detalje.
Pseudokod ovog algoritma takodje je u komentarima - u glavnoj metodi `solve` klase `CDCL`.

Analiza konflikata implementirana je oponasajuci graf implikacija (prvi postupak opisan gore, onaj koji ne ukljucuje rez), dok se nehronolosko vracanje radi na osnovu analize konflikata (prateci nivoe).

KNF formula se ucitava u `dimacs` formatu (u odeljku *Koriscenje* bice opisano kako) za sta sluzi metod `load_dimacs`.

Porede se performanse DPLL i CDCL (ako se ovo moze u potpunosti okarakterisati kao CDCL) algoritma.
Algoritme smo testirali na nekoliko manjih (ne previse znacajnih) KNF formula, dok su se glavne razlike pokazale kod dva veca primera.

Prvi veci primer je `50_sat.cnf` (50 promenljivih, 80 klauza, zadovoljiv) gde je CDCL nekoliko desetina puta brzi od DPLL.

Drugi veci primer je `100_unsat.cnf` (100 promenljivih, 160 klauza, nezadovljiv) gde je CDCL izuzetno brzo prijavio UNSAT, dok DPLL cak ni ne vredi cekati (vise od minut traje sigurno).

Oba primera i mnogi drugi dostupni su na [^7].
Ne preporucuje se testiranje ovog CDCL algoritma na drugim velikim primerima, jer zbog izostanka ***forget and restart*** pravila nije u mogucnosti da se izvrsi (i kad jeste, sigurno ne efikasno).

## Koriscenje
---
Za koriscenje ovog programa potreban je `g++` kompajler.
Fajlove skinuti, smestiti u jedan folder i prevesti sa `g++ main.cpp dpll.cpp pval.cpp cdcl.cpp`.
Pri izvrsavanju, ime datoteke koja sadrzi KNF formulu u dimacs formatu zadaje se kao argument komandne linije.

### Literature :
[^1]: [Automatsko rezonovanje – beleˇske sa predavanja Iskazno rezonovanje](http://poincare.matf.bg.ac.rs/~milan/preuzimanje/ar/ar-iskazna-logika.pdf)
[^2]: [Youtube - Lecture 4B: Modern SAT Solvers](https://www.youtube.com/watch?v=eDQcMBhWzvA&t=865s&ab_channel=UCLAAutomatedReasoningGroup)
[^3]: [UCLA Automated Reasoning Group](http://www.cs.cmu.edu/~mheule/15816-f20/schedule.html)
[^4]: [From DPLL to CDCL SAT solvers](https://www.cs.upc.edu/~erodri/webpage/cps/theory/sat/CDCL-SAT-solvers/slides.pdf)
[^5]: [Evaluating CDCL Variable Scoring Schemes](http://fmv.jku.at/papers/BiereFroehlich-SAT15.pdf)
[^6]: [ar-vezbe](https://github.com/idrecun/ar-vezbe/tree/main/kodovi)
[^7]: [CNF Files](https://people.sc.fsu.edu/~jburkardt/data/cnf/cnf.html)



