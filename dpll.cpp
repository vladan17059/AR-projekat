#include "dpll.hpp"

void DPLL::load_dimacs(std::istream& inputStream) {
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

std::optional<PartialValuation> DPLL::solve() {
    // ponavljamo sledece dok se ne zaustavimo:
    // ako imamo konflikt radimo bektrek
    //     ako imamo literal po kom smo pravili izbor, dodajemo ga negiranog
    //     inace, vracamo UNSAT
    // ako imamo jedinicnu klauzu, dodajemo odgovarajuci literal i nastavljamo
    // ako imamo nedefinisan atom, pravimo izbor po njemu
    // default - vracamo valuaciju (SAT)
    Literal l;
    while(true) {
        //m_valuation.print();
        if(conflict()) {
            l = m_valuation.backtrack();
            if(l != NullLiteral)
                m_valuation.push(-l, false);
            else
                break;
        }
        else if((l = unitClause()) != NullLiteral)
            m_valuation.push(l, false);
        else if((l = m_valuation.nextLiteral()) != NullLiteral)
            m_valuation.push(l, true);
        else
            return m_valuation;
    }
    return {};
}

Literal DPLL::unitClause() const {
    Literal literal;
    for(auto& clause : m_formula)
        if((literal = m_valuation.isUnitClause(clause)) != NullLiteral)
            return literal;
    return NullLiteral;
}

bool DPLL::conflict() const {
    for(auto& clause : m_formula)
        if(m_valuation.isConflict(clause))
            return true;
    return false;
}