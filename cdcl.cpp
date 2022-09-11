#include "cdcl.hpp"

// operatori za ispis
//--------------------------------------

std::ostream& operator<<(std::ostream& os, Clause& clause) {
    for(auto literal : clause)
        os<<literal<<" ";
    return os;
}

std::ostream& operator<<(std::ostream& os, std::deque<Literal>& clause) {
    for(auto literal : clause)
        os<<literal<<" ";
    return os;
}

std::ostream& operator<<(std::ostream& os, NormalForm& formula) {
    os<<"-------\n";
    for(auto clause : formula) {
        os<<clause<<"\n";
    }
    os<<"-------\n";
    return os;
}

//--------------------------------------


// ucitavanje formule - dimacs format
//--------------------------------------

void CDCL::load_dimacs(std::istream& inputStream) {
    std::string buffer;
    do {
        inputStream >> buffer;
        if(buffer == "c")
            inputStream.ignore(std::string::npos, '\n');
    } while(buffer != "p");

    inputStream >> buffer; // cnf

    unsigned varCount, clauseCount;
    inputStream >> varCount >> clauseCount;

    m_valuation.init(varCount);
    levels.resize(varCount+1, 0);
    implication_graph.resize(varCount+1);
    scores.resize(varCount+1, 0.0);
    is_decision.resize(varCount+1, false);
    n_clauses = clauseCount;

    while(clauseCount--) {
        Clause clause;
        Literal literal;
        inputStream >> literal;
        while(literal != NullLiteral) {
            clause.push_back(literal);
            inputStream >> literal;
        }
        m_formula.push_back(clause);
    }
}

//--------------------------------------


// resavanje
//--------------------------------------

std::optional<PartialValuation> CDCL::solve() {
    double delta = 1;
    // ponavljamo sledece dok se ne zaustavimo:
    // ako imamo konflikt: 
    //     ako je "decisions" prazan: konflikt bez odluke - UNSAT
    //     inace: radimo analizu konflikata, dodajemo naucenu klauzu(*) zatim backjump
    // ako imamo jedinicnu klauzu: dodajemo odgovarajuci literal i nastavljamo 
    // ako imamo nedefinisan atom: pravimo izbor po njemu(**)
    // default: SAT - vracamo valuaciju
    //
    //(*) TODO: heuristike zaboravljanja
    //(**) definisani atom cemo traziti pomocu heuristike (naci cemo onaj sa najvecim score-om)

    // praticemo nivoe - redosled literala odluke (bitno za backjump)
    unsigned level = 0;
    Literal l;
    while(true) {
        Clause conflictClause = conflict();
        // ako postoji konflikt
        if(!conflictClause.empty()) {
            // ako do konflikta nije doslo nasom odlukom - UNSAT
            if(decisions.empty()) {
                // ispisimo na kraju koliko je velika nasa formula (koliko smo klauza naucili)
                // ovo nam moze posluziti pri uvodjenju ZABORAVLJANJA i RESTARTOVANJA
                std::cout<<"Formula size: "<<m_formula.size()<<std::endl;
                return {};
            }
            // ANALIZA KONFLIKATA
            Clause learnedClause = conflictAnalysis(conflictClause);


            // heuristika: povecavamo skor atomima koji su ucestvovali u konfliktu
            for(Literal l : learnedClause)
                scores[std::abs(l)] += delta;

            delta *= 1.05;


            // ako delta postane preveliko
            if(delta > 10000) {
                for(int i=1; i<=scores.size(); i++)
                    scores[i] /= 10000;
                delta /= 10000;
            }


            // UCENJE KLAUZA - dodajemo naucenu klauzu u skup klauza
            m_formula.push_back(learnedClause);

            // BACKJUMP - brisemo sve do odgovarajuceg nivoa (drugog najveceg u konfl. klauzi)
            // prvo pronalazimo nivo do kojeg brisemo
            level = backjump_level(learnedClause);
            backjump(level);

        }
        else if((l = unitClause()) != NullLiteral) {
            // ako postoji jedinicna klauza
            // stavljamo literal jedinicne klauze na stek (parcijalnu valuaciju), azuriramo njen polaritet
            m_valuation.m_stack.push_back(l);
            m_valuation.m_value[std::abs(l)] = l > 0 ? TriBool::True : TriBool::False;
        }
        else if((l = nextLiteralEVSIDS()) != NullLiteral) {
            // posto je ovo literal odluke, povecavamo nivo
            levels[std::abs(l)] = ++level;
            // stavljamo ga na stek, azuriramo polaritet
            m_valuation.m_stack.push_back(l);
            m_valuation.m_value[std::abs(l)] = l > 0 ? TriBool::True : TriBool::False;
            // odrzavamo i skup literala odluka (zbog UNSAT uslova)
            decisions.push_back(l);
        }
        else {
            // ako nije nista od navedenog (nema konflikta, nedefinisanih literala) 
            // => SAT  i vracamo valuaciju

            std::cout<<"Formula size: "<<m_formula.size()<<std::endl;
            return m_valuation;
        }
    }
    return {};
}

// NAPOMENA: postoje heuristike formiranja naucene klauze (izbor preseka grafa implikacija).
// Ipak, zbog nebaratanja presecima grafova (u implementacionom smislu), bice preskocene.
Clause CDCL::conflictAnalysis(Clause& conflictClause) {
    // Kako cemo izvesti analizu konflikata?
    // - na raspolaganju nam je graf implikacija
    // - krenucemo od samog konflikta (konfliktne klauze)
    // - literale konfliktne klauze ubacujemo u red
    // - radimo sledece (dok god ne stignemo do kraja reda):
    //      - skinemo i sacuvamo literal sa vrha reda
    //      - proverimo da li je on literal odluke
    //          - jeste -> dodajemo ga u naucenu klauzu
    //          - nije -> obradjujemo (dodajemo u red) njegove "roditelje" iz grafa (zakljucujemo zasto je on izabran)

    // - pre svakog dodavanja proveravamo da literal vec nije dodat (u red ili u naucenu klauzu) - zbog uslova zaustavljanja

    std::deque<Literal> learned;
    Clause learnedClause;

    for(Literal l : conflictClause)
        learned.push_front(l);
    

    for(auto it = learned.rbegin(); it != learned.rend(); it++) {
        // uzmemo literal iz konflikta
        Literal current = learned.back();
        learned.pop_back();
        // zasto je taj literal izabran u nekom trenutku?
        // mozemo da vidimo u implication_graph-u!

        

        // ako je literal odluke (i nije vec u naucenoj klauzi), stavljamo ga u naucenu klauzu
        if(implication_graph[std::abs(current)].empty() && 
                std::find(learnedClause.begin(), learnedClause.end(), current) == learnedClause.end() &&
                std::find(learnedClause.begin(), learnedClause.end(), -current) == learnedClause.end()) {
            learnedClause.push_back(current);
            continue;
        }
        
        // ako nije literal odluke,
        // obradjujemo njegove roditelje (objasnjavamo zasto je on izabran u nekom trenutku)
        for(Literal parent : implication_graph[std::abs(current)]) {
            // ako mu je roditelj literal odluke, proveravamo da li smo ga vec ubacili
            // ako nismo ubacujemo ga
            if(implication_graph[std::abs(parent)].empty() && 
                std::find(learnedClause.begin(), learnedClause.end(), parent) == learnedClause.end() &&
                std::find(learnedClause.begin(), learnedClause.end(), -parent) == learnedClause.end()) {

                learnedClause.push_back(parent);
                continue;
            }
                

            // ako smo nekog "roditelja" vec obradili, nema potrebe da ga ubacujemo (da bi mogli da zavrsimo petlju)
            if(std::find(learned.begin(), learned.end(), parent) != learned.end() || 
                std::find(learned.begin(), learned.end(), -parent) != learned.end())
                continue;
            
            // inace ubacujemo taj literal za dalje objasnjavanje
            // trenutno nam polaritet nije bitan, to cemo na kraju da podesimo
            learned.push_front(parent);

        }

    }

    // Kad smo zavrsili petlju, ostali su nam literali odluke krivi za konflikt.
    // Kako nismo obracali paznju da li su + ili -, to cemo sad popraviti.
    // Analizom smo zakljucili da je do konflikta doslo jer vazi npr. +1 && -3 && +4.
    // Ovi uslovi ne mogu istovremeno da vaze, te negiranjem dobijamo naucenu klauzu
    // [ -1, 3, -4 ]
    

    Clause polarizedLearnedClause;
    for(Literal l : learnedClause) 
        polarizedLearnedClause.push_back(m_valuation.m_value[std::abs(l)] == TriBool::True ? -std::abs(l) : std::abs(l));
    
    return polarizedLearnedClause;
}

// nivo skoka (drugi najveci u naucenoj klauzi)
unsigned CDCL::backjump_level(Clause& learnedClause) {
    unsigned max_level = 0;

    for(auto literal : learnedClause) 
        max_level = std::max(max_level, levels[std::abs(literal)]);

    unsigned second_max_level = 0;
    for(auto literal : learnedClause) {
        if(levels[std::abs(literal)] == max_level)
            continue;
        second_max_level = std::max(second_max_level, levels[std::abs(literal)]);
    }
    return second_max_level;
}

// Nehronolosko vracanje - skidamo elemente sa steka (parc. valuacije), skupa odluka, azuriramo potrebne nizove i graf implikacija
// "Skacemo" do odredjenog nivoa (odredjenog analizom konflikata)
void CDCL::backjump(unsigned level) {
    Literal current;
    // skidamo elemente sa steka (parcijalne valuacije) dok god smo iznad datog nivoa
    for(auto it = m_valuation.m_stack.rbegin(); it+1 != m_valuation.m_stack.rend() && levels[std::abs(*(it+1))] >= level; it++) {
        current = m_valuation.m_stack.back();
        // uklanjamo iz parcijalne valuacije
        m_valuation.m_stack.pop_back();
        m_valuation.m_value[std::abs(current)] = TriBool::Undefined;
        // uklanjamo iz grafa implikacija 
        implication_graph[std::abs(current)].clear();
        // postavljamo nivo na nulti
        levels[std::abs(current)] = 0;
    }

    // uklanjamo sa "decisions" steka
    for(auto it = decisions.rbegin(); it+1 != decisions.rend() && levels[std::abs(*(it+1))] >= level; it++) {
        decisions.pop_back();
    }

    // poseban slucaj kad je level 0 (ostao nam jos jedan element)
    if(level == 0) {
        current = m_valuation.m_stack.back();
        m_valuation.m_stack.pop_back();
        m_valuation.m_value[std::abs(current)] = TriBool::Undefined;
        implication_graph[std::abs(current)].clear();
        levels[std::abs(current)] = 0;
        decisions.pop_back();
    }
}

// Ako postoji jedinicna klauza, vraca literal te klauze.
Literal CDCL::unitClause()  {
    Literal unit;
    for(auto& clause : m_formula)
        // Ako naletimo na jedinicnu klauzu:
        // *jedinicni literal - jedini nedefinisan literal jedinicne klauze
        if((unit = m_valuation.isUnitClause(clause)) != NullLiteral) {
            // - azuriramo graf implikacija (roditelje od jedinicnog literala jedinicne klauze)
            // - azuriramo nivo jedinicnog literala
            for(auto& l : clause) {
                // - ako smo naleteli na taj literal, preskocimo ga
                if(m_valuation.m_value[std::abs(l)] == TriBool::Undefined)
                    continue;
                levels[std::abs(unit)] = std::max(levels[std::abs(unit)], levels[std::abs(l)]);
                implication_graph[std::abs(unit)].push_back(std::abs(l));
            }
            return unit;
        }
    return NullLiteral;
}

// Proverava da li postoji konfliktna klauza, ako da, vraca je.
Clause CDCL::conflict() {
    for(auto& clause : m_formula)
        if(m_valuation.isConflict(clause))
            return clause;
    return {};
}

// DECISION: biramo sledeci literal (odluke) pomocu heuristike
// Heuristika EVSIDS (Exponential Variable State Independent Decaying Sum) - Pri svakom konfliktu: 
//  - povecamo skor literalima (atomima - ne gledamo polaritet) koji su ucestvovali za delta
//  - delta = delta*1.05
Literal CDCL::nextLiteralEVSIDS() {
    Literal best_literal = 0;
    double best_score = -1;
    for(int i=1; i<=scores.size(); i++) {
        // Biramo nedefinisan literal sa najboljim score-om
        if(m_valuation.m_value[i] == TriBool::Undefined && scores[i] > best_score) {
            best_score = scores[i];
            best_literal = i;
        }
    }


    if(best_literal == 0)
        return NullLiteral;
    
    return best_literal;
}

Literal CDCL::nextLiteral() {
    auto it = std::find(begin(m_valuation.m_value) + 1, end(m_valuation.m_value), TriBool::Undefined);
    return it != end(m_valuation.m_value) ? distance(begin(m_valuation.m_value), it) : NullLiteral;
}

