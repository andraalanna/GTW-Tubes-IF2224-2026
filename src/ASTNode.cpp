#include "ASTNode.h"
#include <sstream>

string ASTNode::annotation() const
{
    string s = " [type:" + SymbolTable::DataTypeToString(dtype);
    if (tabIndex >= 0)
        s += ", tab:" + to_string(tabIndex) + ", lev:" + to_string(lexLevel);
    s += "]";
    return s;
}

void ASTNode::printHeader(ostream &out,
                          const string &prefix,
                          bool isLast,
                          bool isRoot,
                          const string &extra) const
{
    if (isRoot)
    {
        out << nodeName() << annotation();
        if (!extra.empty())
            out << " " << extra;
        out << "\n";
    }
    else
    {
        out << prefix << (isLast ? "└── " : "├── ");
        out << nodeName() << annotation();
        if (!extra.empty())
            out << " " << extra;
        out << "\n";
    }
}

void ASTNode::printChildren(ostream &out,
                            const string &prefix,
                            bool isLast,
                            bool isRoot,
                            const vector<ASTNodePtr> &children) const
{
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    for (size_t i = 0; i < children.size(); i++)
    {
        if (children[i])
            children[i]->print(out, childPrefix, i == children.size() - 1, false);
    }
}

void ProgramNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));

    for (size_t i = 0; i < declarations.size(); i++)
    {
        bool last = (i == declarations.size() - 1) && !body;
        if (declarations[i])
            declarations[i]->print(out, childPrefix, last, false);
    }
    if (body)
        body->print(out, childPrefix, true, false);
}

void VarDeclNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
}

void ConstDeclNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
}

void TypeDeclNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
}

void ProcDeclNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    for (size_t i = 0; i < params.size(); i++)
    {
        if (params[i])
            params[i]->print(out, childPrefix, (i == params.size() - 1) && !body, false);
    }
    if (body)
        body->print(out, childPrefix, true, false);
}

void FuncDeclNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    for (size_t i = 0; i < params.size(); i++)
    {
        if (params[i])
            params[i]->print(out, childPrefix, (i == params.size() - 1) && !body, false);
    }
    if (body)
        body->print(out, childPrefix, true, false);
}

void CompoundStmtNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    printChildren(out, prefix, isLast, isRoot, statements);
}

void AssignNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (target)
        target->print(out, childPrefix, !value, false);
    if (value)
        value->print(out, childPrefix, true, false);
}

void BinOpNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (left)
        left->print(out, childPrefix, !right, false);
    if (right)
        right->print(out, childPrefix, true, false);
}

void UnaryOpNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (operand)
        operand->print(out, childPrefix, true, false);
}

void VarNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
}

void NumberNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
}

void CharNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
}

void StringNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
}

void ProcCallNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    printChildren(out, prefix, isLast, isRoot, args);
}

void IfNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    bool hasElse = (elseBranch != nullptr);
    if (condition)
        condition->print(out, childPrefix, !thenBranch && !hasElse, false);
    if (thenBranch)
        thenBranch->print(out, childPrefix, !hasElse, false);
    if (elseBranch)
        elseBranch->print(out, childPrefix, true, false);
}

// ============================================================
// WhileNode
// ============================================================
void WhileNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (condition)
        condition->print(out, childPrefix, !body, false);
    if (body)
        body->print(out, childPrefix, true, false);
}

void RepeatNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    for (size_t i = 0; i < statements.size(); i++)
    {
        if (statements[i])
            statements[i]->print(out, childPrefix, (i == statements.size() - 1) && !condition, false);
    }
    if (condition)
        condition->print(out, childPrefix, true, false);
}

void ForNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (fromExpr)
        fromExpr->print(out, childPrefix, !toExpr && !body, false);
    if (toExpr)
        toExpr->print(out, childPrefix, !body, false);
    if (body)
        body->print(out, childPrefix, true, false);
}

void CaseNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (selector)
        selector->print(out, childPrefix, branches.empty(), false);
    for (size_t i = 0; i < branches.size(); i++)
    {
        string label = "CaseBranch(";
        for (size_t j = 0; j < branches[i].first.size(); j++)
        {
            if (j)
                label += ",";
            label += branches[i].first[j];
        }
        label += ")";
        bool last = (i == branches.size() - 1);
        out << childPrefix << (last ? "└── " : "├── ") << label << "\n";
        string branchPrefix = childPrefix + (last ? "    " : "│   ");
        if (branches[i].second)
            branches[i].second->print(out, branchPrefix, true, false);
    }
}

void ArrayAccessNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (array)
        array->print(out, childPrefix, indices.empty(), false);
    for (size_t i = 0; i < indices.size(); i++)
    {
        if (indices[i])
            indices[i]->print(out, childPrefix, i == indices.size() - 1, false);
    }
}

void FieldAccessNode::print(ostream &out, const string &prefix, bool isLast, bool isRoot) const
{
    printHeader(out, prefix, isLast, isRoot);
    string childPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));
    if (record)
        record->print(out, childPrefix, true, false);
}