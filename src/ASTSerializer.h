#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include "ASTNode.h"
#include "symbolTable.hpp"

using json = nlohmann::json;

// ─── forward declarations ────────────────────────────────────────────────────
json serializeNode(const ASTNode *node);
ASTNodePtr deserializeNode(const json &j);

// ═══════════════════════════════════════════════════════════════════════════
//  ENUM helpers
// ═══════════════════════════════════════════════════════════════════════════
static std::string dtToStr(DataType d) {
    switch (d) {
        case DataType::INTEGER:    return "integer";
        case DataType::REAL:       return "real";
        case DataType::CHAR:       return "char";
        case DataType::BOOLEAN:    return "boolean";
        case DataType::STRING:     return "string";
        case DataType::SUBRANGE:   return "subrange";
        case DataType::ENUMERATED: return "enumerated";
        case DataType::ARRAY:      return "array";
        case DataType::RECORD:     return "record";
        case DataType::VOID:       return "void";
        default:                   return "unknown";
    }
}
static DataType strToDt(const std::string &s) {
    if (s=="integer")    return DataType::INTEGER;
    if (s=="real")       return DataType::REAL;
    if (s=="char")       return DataType::CHAR;
    if (s=="boolean")    return DataType::BOOLEAN;
    if (s=="string")     return DataType::STRING;
    if (s=="subrange")   return DataType::SUBRANGE;
    if (s=="enumerated") return DataType::ENUMERATED;
    if (s=="array")      return DataType::ARRAY;
    if (s=="record")     return DataType::RECORD;
    if (s=="void")       return DataType::VOID;
    return DataType::UNKNOWN;
}
static std::string objToStr(AllowedObj o) {
    switch (o) {
        case AllowedObj::CONSTANT:  return "constant";
        case AllowedObj::VARIABLE:  return "variable";
        case AllowedObj::TYPE:      return "type";
        case AllowedObj::FUNCTION:  return "function";
        case AllowedObj::PROCEDURE: return "procedure";
        case AllowedObj::PROGRAM:   return "program";
        default:                    return "keyword";
    }
}
static AllowedObj strToObj(const std::string &s) {
    if (s=="constant")  return AllowedObj::CONSTANT;
    if (s=="variable")  return AllowedObj::VARIABLE;
    if (s=="type")      return AllowedObj::TYPE;
    if (s=="function")  return AllowedObj::FUNCTION;
    if (s=="procedure") return AllowedObj::PROCEDURE;
    if (s=="program")   return AllowedObj::PROGRAM;
    return AllowedObj::KEYWORD;
}

// helper tulis annotation dasar ke json
static void writeBase(json &j, const ASTNode *n) {
    j["dtype"]    = dtToStr(n->dtype);
    j["tabIndex"] = n->tabIndex;
    j["lexLevel"] = n->lexLevel;
    j["typeRef"]  = n->typeRef;
}
static void readBase(const json &j, ASTNode *n) {
    n->dtype    = strToDt(j.value("dtype","unknown"));
    n->tabIndex = j.value("tabIndex", -1);
    n->lexLevel = j.value("lexLevel", 0);
    n->typeRef  = j.value("typeRef",  0);
}

// ═══════════════════════════════════════════════════════════════════════════
//  SERIALIZE
// ═══════════════════════════════════════════════════════════════════════════
json serializeNode(const ASTNode *node) {
    if (!node) return nullptr;
    json j;

    if (auto *n = dynamic_cast<const ProgramNode*>(node)) {
        j["kind"] = "Program";
        writeBase(j, n);
        j["name"] = n->name;
        for (auto &d : n->declarations) j["declarations"].push_back(serializeNode(d.get()));
        j["body"] = serializeNode(n->body.get());

    } else if (auto *n = dynamic_cast<const VarDeclNode*>(node)) {
        j["kind"]    = "VarDecl";
        writeBase(j, n);
        j["varName"] = n->varName;
        j["varType"] = dtToStr(n->varType);
        j["typeRef"] = n->typeRef;

    } else if (auto *n = dynamic_cast<const ConstDeclNode*>(node)) {
        j["kind"]      = "ConstDecl";
        writeBase(j, n);
        j["constName"] = n->constName;
        j["constType"] = dtToStr(n->constType);
        j["value"]     = n->value;

    } else if (auto *n = dynamic_cast<const TypeDeclNode*>(node)) {
        j["kind"]     = "TypeDecl";
        writeBase(j, n);
        j["typeName"] = n->typeName;
        j["baseType"] = dtToStr(n->baseType);
        j["typeRef"]  = n->typeRef;

    } else if (auto *n = dynamic_cast<const ProcDeclNode*>(node)) {
        j["kind"]     = "ProcDecl";
        writeBase(j, n);
        j["procName"] = n->procName;
        for (auto &p : n->params) j["params"].push_back(serializeNode(p.get()));
        j["body"] = serializeNode(n->body.get());

    } else if (auto *n = dynamic_cast<const FuncDeclNode*>(node)) {
        j["kind"]       = "FuncDecl";
        writeBase(j, n);
        j["funcName"]   = n->funcName;
        j["returnType"] = dtToStr(n->returnType);
        for (auto &p : n->params) j["params"].push_back(serializeNode(p.get()));
        j["body"] = serializeNode(n->body.get());

    } else if (auto *n = dynamic_cast<const CompoundStmtNode*>(node)) {
        j["kind"] = "CompoundStmt";
        writeBase(j, n);
        for (auto &s : n->statements) j["statements"].push_back(serializeNode(s.get()));

    } else if (auto *n = dynamic_cast<const AssignNode*>(node)) {
        j["kind"]   = "Assign";
        writeBase(j, n);
        j["target"] = serializeNode(n->target.get());
        j["value"]  = serializeNode(n->value.get());

    } else if (auto *n = dynamic_cast<const BinOpNode*>(node)) {
        j["kind"]  = "BinOp";
        writeBase(j, n);
        j["op"]    = n->op;
        j["left"]  = serializeNode(n->left.get());
        j["right"] = serializeNode(n->right.get());

    } else if (auto *n = dynamic_cast<const UnaryOpNode*>(node)) {
        j["kind"]    = "UnaryOp";
        writeBase(j, n);
        j["op"]      = n->op;
        j["operand"] = serializeNode(n->operand.get());

    } else if (auto *n = dynamic_cast<const VarNode*>(node)) {
        j["kind"]    = "Var";
        writeBase(j, n);
        j["varName"] = n->varName;

    } else if (auto *n = dynamic_cast<const NumberNode*>(node)) {
        j["kind"]     = "Number";
        writeBase(j, n);
        j["rawValue"] = n->rawValue;

    } else if (auto *n = dynamic_cast<const CharNode*>(node)) {
        j["kind"]     = "Char";
        writeBase(j, n);
        j["rawValue"] = n->rawValue;

    } else if (auto *n = dynamic_cast<const StringNode*>(node)) {
        j["kind"]     = "String";
        writeBase(j, n);
        j["rawValue"] = n->rawValue;

    } else if (auto *n = dynamic_cast<const ProcCallNode*>(node)) {
        j["kind"]     = "ProcCall";
        writeBase(j, n);
        j["procName"] = n->procName;
        j["tabIndex"] = n->tabIndex;
        for (auto &a : n->args) j["args"].push_back(serializeNode(a.get()));

    } else if (auto *n = dynamic_cast<const IfNode*>(node)) {
        j["kind"]      = "If";
        writeBase(j, n);
        j["condition"] = serializeNode(n->condition.get());
        j["then"]      = serializeNode(n->thenBranch.get());
        j["else"]      = serializeNode(n->elseBranch.get());

    } else if (auto *n = dynamic_cast<const WhileNode*>(node)) {
        j["kind"]      = "While";
        writeBase(j, n);
        j["condition"] = serializeNode(n->condition.get());
        j["body"]      = serializeNode(n->body.get());

    } else if (auto *n = dynamic_cast<const RepeatNode*>(node)) {
        j["kind"]      = "Repeat";
        writeBase(j, n);
        j["condition"] = serializeNode(n->condition.get());
        for (auto &s : n->statements) j["statements"].push_back(serializeNode(s.get()));

    } else if (auto *n = dynamic_cast<const ForNode*>(node)) {
        j["kind"]     = "For";
        writeBase(j, n);
        j["varName"]  = n->varName;
        j["from"]     = serializeNode(n->fromExpr.get());
        j["to"]       = serializeNode(n->toExpr.get());
        j["isDownto"] = n->isDownto;
        j["body"]     = serializeNode(n->body.get());

    } else if (auto *n = dynamic_cast<const CaseNode*>(node)) {
        j["kind"]     = "Case";
        writeBase(j, n);
        j["selector"] = serializeNode(n->selector.get());
        for (auto &[vals, stmt] : n->branches) {
            json branch;
            branch["values"] = vals;
            branch["stmt"]   = serializeNode(stmt.get());
            j["branches"].push_back(branch);
        }

    } else if (auto *n = dynamic_cast<const ArrayAccessNode*>(node)) {
        j["kind"]  = "ArrayAccess";
        writeBase(j, n);
        j["array"] = serializeNode(n->array.get());
        for (auto &i : n->indices) j["indices"].push_back(serializeNode(i.get()));

    } else if (auto *n = dynamic_cast<const FieldAccessNode*>(node)) {
        j["kind"]      = "FieldAccess";
        writeBase(j, n);
        j["record"]    = serializeNode(n->record.get());
        j["fieldName"] = n->fieldName;
    }

    return j;
}

// ═══════════════════════════════════════════════════════════════════════════
//  DESERIALIZE
// ═══════════════════════════════════════════════════════════════════════════
ASTNodePtr deserializeNode(const json &j) {
    if (j.is_null()) return nullptr;
    std::string kind = j.value("kind", "");

    if (kind == "Program") {
        auto n = std::make_shared<ProgramNode>(j.value("name",""));
        readBase(j, n.get());
        for (auto &d : j.value("declarations", json::array()))
            n->declarations.push_back(deserializeNode(d));
        n->body = deserializeNode(j.value("body", json()));
        return n;

    } else if (kind == "VarDecl") {
        auto n = std::make_shared<VarDeclNode>(
            j.value("varName",""),
            strToDt(j.value("varType","unknown")),
            j.value("typeRef",0));
        readBase(j, n.get());
        return n;

    } else if (kind == "ConstDecl") {
        auto n = std::make_shared<ConstDeclNode>(
            j.value("constName",""),
            strToDt(j.value("constType","unknown")),
            j.value("value",""));
        readBase(j, n.get());
        return n;

    } else if (kind == "TypeDecl") {
        auto n = std::make_shared<TypeDeclNode>(
            j.value("typeName",""),
            strToDt(j.value("baseType","unknown")),
            j.value("typeRef",0));
        readBase(j, n.get());
        return n;

    } else if (kind == "ProcDecl") {
        auto n = std::make_shared<ProcDeclNode>(j.value("procName",""));
        readBase(j, n.get());
        for (auto &p : j.value("params", json::array()))
            n->params.push_back(deserializeNode(p));
        n->body = deserializeNode(j.value("body", json()));
        return n;

    } else if (kind == "FuncDecl") {
        auto n = std::make_shared<FuncDeclNode>(
            j.value("funcName",""),
            strToDt(j.value("returnType","unknown")));
        readBase(j, n.get());
        for (auto &p : j.value("params", json::array()))
            n->params.push_back(deserializeNode(p));
        n->body = deserializeNode(j.value("body", json()));
        return n;

    } else if (kind == "CompoundStmt") {
        auto n = std::make_shared<CompoundStmtNode>();
        readBase(j, n.get());
        for (auto &s : j.value("statements", json::array()))
            n->statements.push_back(deserializeNode(s));
        return n;

    } else if (kind == "Assign") {
        auto target = deserializeNode(j.value("target", json()));
        auto value  = deserializeNode(j.value("value",  json()));
        auto n = std::make_shared<AssignNode>(target, value);
        readBase(j, n.get());
        return n;

    } else if (kind == "BinOp") {
        auto left  = deserializeNode(j.value("left",  json()));
        auto right = deserializeNode(j.value("right", json()));
        auto n = std::make_shared<BinOpNode>(j.value("op",""), left, right);
        readBase(j, n.get());
        return n;

    } else if (kind == "UnaryOp") {
        auto operand = deserializeNode(j.value("operand", json()));
        auto n = std::make_shared<UnaryOpNode>(j.value("op",""), operand);
        readBase(j, n.get());
        return n;

    } else if (kind == "Var") {
        auto n = std::make_shared<VarNode>(j.value("varName",""));
        readBase(j, n.get());
        return n;

    } else if (kind == "Number") {
        auto n = std::make_shared<NumberNode>(
            j.value("rawValue",""),
            strToDt(j.value("dtype","integer")));
        readBase(j, n.get());
        return n;

    } else if (kind == "Char") {
        auto n = std::make_shared<CharNode>(j.value("rawValue",""));
        readBase(j, n.get());
        return n;

    } else if (kind == "String") {
        auto n = std::make_shared<StringNode>(j.value("rawValue",""));
        readBase(j, n.get());
        return n;

    } else if (kind == "ProcCall") {
        auto n = std::make_shared<ProcCallNode>(j.value("procName",""));
        readBase(j, n.get());
        n->tabIndex = j.value("tabIndex", -1);
        for (auto &a : j.value("args", json::array()))
            n->args.push_back(deserializeNode(a));
        return n;

    } else if (kind == "If") {
        auto cond = deserializeNode(j.value("condition", json()));
        auto then = deserializeNode(j.value("then", json()));
        auto els  = deserializeNode(j.value("else", json()));
        auto n = std::make_shared<IfNode>(cond, then, els);
        readBase(j, n.get());
        return n;

    } else if (kind == "While") {
        auto cond = deserializeNode(j.value("condition", json()));
        auto body = deserializeNode(j.value("body", json()));
        auto n = std::make_shared<WhileNode>(cond, body);
        readBase(j, n.get());
        return n;

    } else if (kind == "Repeat") {
        std::vector<ASTNodePtr> stmts;
        for (auto &s : j.value("statements", json::array()))
            stmts.push_back(deserializeNode(s));
        auto cond = deserializeNode(j.value("condition", json()));
        auto n = std::make_shared<RepeatNode>(stmts, cond);
        readBase(j, n.get());
        return n;

    } else if (kind == "For") {
        auto from = deserializeNode(j.value("from", json()));
        auto to   = deserializeNode(j.value("to",   json()));
        auto body = deserializeNode(j.value("body", json()));
        auto n = std::make_shared<ForNode>(
            j.value("varName",""), from, to,
            j.value("isDownto", false), body);
        readBase(j, n.get());
        return n;

    } else if (kind == "Case") {
        auto sel = deserializeNode(j.value("selector", json()));
        auto n = std::make_shared<CaseNode>(sel);
        readBase(j, n.get());
        for (auto &b : j.value("branches", json::array())) {
            std::vector<std::string> vals = b.value("values", std::vector<std::string>{});
            auto stmt = deserializeNode(b.value("stmt", json()));
            n->branches.push_back({vals, stmt});
        }
        return n;

    } else if (kind == "ArrayAccess") {
        auto arr = deserializeNode(j.value("array", json()));
        std::vector<ASTNodePtr> indices;
        for (auto &i : j.value("indices", json::array()))
            indices.push_back(deserializeNode(i));
        auto n = std::make_shared<ArrayAccessNode>(arr, indices);
        readBase(j, n.get());
        return n;

    } else if (kind == "FieldAccess") {
        auto rec = deserializeNode(j.value("record", json()));
        auto n = std::make_shared<FieldAccessNode>(rec, j.value("fieldName",""));
        readBase(j, n.get());
        return n;
    }

    return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SYMBOL TABLE serialize / deserialize
// ═══════════════════════════════════════════════════════════════════════════
inline json serializeSymbolTable(const SymbolTable &st) {
    json j;
    // tab
    for (auto &e : st.tab) {
        json t;
        t["name"] = e.name;
        t["link"] = e.link;
        t["obj"]  = objToStr(e.obj);
        t["type"] = dtToStr(e.type);
        t["ref"]  = e.ref;
        t["nrm"]  = e.nrm;
        t["lev"]  = e.lev;
        t["adr"]  = e.adr;
        j["tab"].push_back(t);
    }
    // btab
    for (auto &b : st.btab) {
        json t;
        t["last"] = b.last;
        t["lpar"] = b.lpar;
        t["psze"] = b.psze;
        t["vsze"] = b.vsze;
        j["btab"].push_back(t);
    }
    // atab
    for (auto &a : st.atab) {
        json t;
        t["xtyp"] = dtToStr(a.xtyp);
        t["etyp"] = dtToStr(a.etyp);
        t["eref"] = a.eref;
        t["low"]  = a.low;
        t["high"] = a.high;
        t["elsz"] = a.elsz;
        t["size"] = a.size;
        j["atab"].push_back(t);
    }
    j["currentLevel"] = st.currentLevel;
    j["currentBlock"] = st.currentBlock;
    j["display"]      = st.display;
    return j;
}

inline void deserializeSymbolTable(const json &j, SymbolTable &st) {
    st.tab.clear();
    st.btab.clear();
    st.atab.clear();

    for (auto &t : j.value("tab", json::array())) {
        TabEntry e;
        e.name = t.value("name","");
        e.link = t.value("link", 0);
        e.obj  = strToObj(t.value("obj","keyword"));
        e.type = strToDt(t.value("type","unknown"));
        e.ref  = t.value("ref", 0);
        e.nrm  = t.value("nrm", 0);
        e.lev  = t.value("lev", 0);
        e.adr  = t.value("adr", 0);
        st.tab.push_back(e);
    }
    for (auto &b : j.value("btab", json::array())) {
        BTabEntry e;
        e.last = b.value("last", 0);
        e.lpar = b.value("lpar", 0);
        e.psze = b.value("psze", 0);
        e.vsze = b.value("vsze", 0);
        st.btab.push_back(e);
    }
    for (auto &a : j.value("atab", json::array())) {
        ATabEntry e;
        e.xtyp = strToDt(a.value("xtyp","unknown"));
        e.etyp = strToDt(a.value("etyp","unknown"));
        e.eref = a.value("eref", 0);
        e.low  = a.value("low",  0);
        e.high = a.value("high", 0);
        e.elsz = a.value("elsz", 1);
        e.size = a.value("size", 0);
        st.atab.push_back(e);
    }
    st.currentLevel = j.value("currentLevel", 1);
    st.currentBlock = j.value("currentBlock", 0);
    st.display      = j.value("display", std::vector<int>{});
}

// ═══════════════════════════════════════════════════════════════════════════
//  TOP-LEVEL: simpan / muat file .json
// ═══════════════════════════════════════════════════════════════════════════
inline void saveToJson(const ASTNodePtr &ast, const SymbolTable &st,
                       const std::string &path) {
    json root;
    root["ast"] = serializeNode(ast.get());
    root["symtable"] = serializeSymbolTable(st);
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Tidak bisa buat file: " + path);
    f << root.dump(2);
}

inline std::pair<ASTNodePtr, SymbolTable> loadFromJson(const std::string &path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("File tidak ditemukan: " + path);
    json root = json::parse(f);
    ASTNodePtr ast = deserializeNode(root.at("ast"));
    SymbolTable st;
    deserializeSymbolTable(root.at("symtable"), st);
    return {ast, st};
}