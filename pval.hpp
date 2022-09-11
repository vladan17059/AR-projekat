#ifndef __PVAL_HPP__
#define __PVAL_HPP__

#include <vector>
#include <deque>

using Literal = int;
using Clause = std::vector<Literal>;
using NormalForm = std::deque<Clause>;

const Literal NullLiteral = 0;

enum class TriBool {
    False,
    True,
    Undefined
};

class PartialValuation
{
public:
    PartialValuation(unsigned varCount = 0);
    void init(unsigned varCount);

    void push(Literal l, bool decide);
    Literal backtrack();

    Literal nextLiteral() const;
    bool isConflict(const Clause& clause) const;
    Literal isUnitClause(const Clause& clause) const;
    TriBool value_of(Literal l);

    void print() const;

    std::vector<Literal> m_stack;
    std::vector<TriBool> m_value;
};

#endif // __PVAL_HPP__