#include "ASTBuilder.h"
#include <stdexcept>

ASTBuilder::ASTBuilder(SymbolTable &st) : st(st) {}

// Utility
shared_ptr<ParseNode> ASTBuilder::child(shared_ptr<ParseNode> n, const string &tag) {
    if (!n) return nullptr;
    for (auto &c : n->children) {
        if (c && c->type == tag) return c;
    }
    return nullptr;
}

vector<shared_ptr<ParseNode>> ASTBuilder::children(shared_ptr<ParseNode> n, const string &tag) {
    vector<shared_ptr<ParseNode>> res;
    if (!n) return res;
    for (auto &c : n->children) {
        if (c && c->type == tag) res.push_back(c);
    }
    return res;
}

vector<string> ASTBuilder::collectIdents(shared_ptr<ParseNode> identListNode) {
    vector<string> res;
    if (!identListNode) return res;
    for (auto &c : identListNode->children) {
        if (c && c->type == "ident") res.push_back(c->value);
    }
    return res;
}

ASTNodePtr ASTBuilder::build(shared_ptr<ParseNode> root) {
    if (!root || root->type != "<program>") {
        throw runtime_error("ASTBuilder: expected <program> root");
    }
    return buildProgram(root);
}

ASTNodePtr ASTBuilder::buildProgram(shared_ptr<ParseNode> n) {
    auto header = child(n, "<program-header>");
    string name = "Unknown";
    if (header) {
        auto ident = child(header, "ident");
        if (ident) name = ident->value;
    }

    auto prog = make_shared<ProgramNode>(name);

    auto declPart = child(n, "<declaration-part>");
    if (declPart) {
        buildDeclarationPart(declPart, prog->declarations);
    }

    auto compStmt = child(n, "<compound-statement>");
    if (compStmt) {
        prog->body = buildCompoundStatement(compStmt);
    }

    return prog;
}

void ASTBuilder::buildDeclarationPart(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out) {
    for (auto &c : n->children) {
        if (!c) continue;
        if (c->type == "<const-declaration>") buildConstDeclaration(c, out);
        else if (c->type == "<type-declaration>") buildTypeDeclaration(c, out);
        else if (c->type == "<var-declaration>") buildVarDeclaration(c, out);
        else if (c->type == "<subprogram-declaration>") buildSubprogramDeclaration(c, out);
    }
}

void ASTBuilder::buildConstDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out) {
    string currentIdent = "";
    for (size_t i = 0; i < n->children.size(); i++) {
        auto &c = n->children[i];
        if (!c) continue;
        if (c->type == "ident") {
            currentIdent = c->value;
        } else if (c->type == "<constant>") {
            auto cinfo = resolveConstant(c);
            out.push_back(make_shared<ConstDeclNode>(currentIdent, cinfo.type, cinfo.value));
        }
    }
}

void ASTBuilder::buildTypeDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out) {
    string currentIdent = "";
    for (size_t i = 0; i < n->children.size(); i++) {
        auto &c = n->children[i];
        if (!c) continue;
        if (c->type == "ident") {
            currentIdent = c->value;
        } else if (c->type == "<type>") {
            auto tinfo = resolveType(c);
            out.push_back(make_shared<TypeDeclNode>(currentIdent, tinfo.type, tinfo.ref));
            
            string lowerName = currentIdent;
            for (char &ch : lowerName) ch = tolower(ch);
            declaredTypes[lowerName] = tinfo;
        }
    }
}

void ASTBuilder::buildVarDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out) {
    vector<string> currentIdents;
    for (size_t i = 0; i < n->children.size(); i++) {
        auto &c = n->children[i];
        if (!c) continue;
        if (c->type == "<identifier-list>") {
            currentIdents = collectIdents(c);
        } else if (c->type == "<type>") {
            auto tinfo = resolveType(c);
            for (auto &id : currentIdents) {
                out.push_back(make_shared<VarDeclNode>(id, tinfo.type, tinfo.ref));
            }
            currentIdents.clear();
        }
    }
}

void ASTBuilder::buildSubprogramDeclaration(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out) {
    for (auto &c : n->children) {
        if (!c) continue;
        if (c->type == "<procedure-declaration>") out.push_back(buildProcedureDeclaration(c));
        else if (c->type == "<function-declaration>") out.push_back(buildFunctionDeclaration(c));
    }
}

ASTNodePtr ASTBuilder::buildBlock(shared_ptr<ParseNode> n, vector<ASTNodePtr> &declsOut) {
    auto declPart = child(n, "<declaration-part>");
    if (declPart) buildDeclarationPart(declPart, declsOut);
    
    auto compStmt = child(n, "<compound-statement>");
    if (compStmt) {
        auto cs = dynamic_pointer_cast<CompoundStmtNode>(buildCompoundStatement(compStmt));
        if (cs) {
            // Prepend local declarations into the statements vector
            vector<ASTNodePtr> allStmts;
            allStmts.insert(allStmts.end(), declsOut.begin(), declsOut.end());
            allStmts.insert(allStmts.end(), cs->statements.begin(), cs->statements.end());
            cs->statements = std::move(allStmts);
            return cs;
        }
    }
    
    auto cs = make_shared<CompoundStmtNode>();
    cs->statements = std::move(declsOut);
    return cs;
}

ASTNodePtr ASTBuilder::buildProcedureDeclaration(shared_ptr<ParseNode> n) {
    auto ident = child(n, "ident");
    auto p = make_shared<ProcDeclNode>(ident ? ident->value : "");
    
    auto fpl = child(n, "<formal-parameter-list>");
    if (fpl) buildFormalParameterList(fpl, p->params);
    
    auto blk = child(n, "<block>");
    if (blk) {
        vector<ASTNodePtr> localDecls;
        p->body = buildBlock(blk, localDecls);
    }
    return p;
}

ASTNodePtr ASTBuilder::buildFunctionDeclaration(shared_ptr<ParseNode> n) {
    auto idents = children(n, "ident");
    string funcName = "";
    DataType retType = DataType::UNKNOWN;
    
    if (idents.size() > 0) funcName = idents[0]->value;
    if (idents.size() > 1) {
        string t = idents[1]->value;
        string tl;
        for (char c : t) tl += tolower(c);
        if (tl == "integer") retType = DataType::INTEGER;
        else if (tl == "real") retType = DataType::REAL;
        else if (tl == "char") retType = DataType::CHAR;
        else if (tl == "boolean") retType = DataType::BOOLEAN;
        else if (tl == "string") retType = DataType::STRING;
    }

    auto f = make_shared<FuncDeclNode>(funcName, retType);
    
    auto fpl = child(n, "<formal-parameter-list>");
    if (fpl) buildFormalParameterList(fpl, f->params);

    auto blk = child(n, "<block>");
    if (blk) {
        vector<ASTNodePtr> localDecls;
        f->body = buildBlock(blk, localDecls);
    }
    return f;
}

void ASTBuilder::buildFormalParameterList(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out) {
    for (auto &c : n->children) {
        if (c && c->type == "<parameter-group>") {
            buildParameterGroup(c, out);
        }
    }
}

void ASTBuilder::buildParameterGroup(shared_ptr<ParseNode> n, vector<ASTNodePtr> &out) {
    auto identList = child(n, "<identifier-list>");
    auto idents = collectIdents(identList);
    
    DataType t = DataType::UNKNOWN;
    int ref = 0;
    
    auto arr = child(n, "<array-type>");
    if (arr) {
        auto tinfo = resolveArrayType(arr);
        t = tinfo.type;
        ref = tinfo.ref;
    } else {
        auto identNodes = children(n, "ident");
        if (identNodes.size() > 0) {
            string tl = identNodes[0]->value;
            for (auto &ch : tl) ch = tolower(ch);
            if (tl == "integer") t = DataType::INTEGER;
            else if (tl == "real") t = DataType::REAL;
            else if (tl == "char") t = DataType::CHAR;
            else if (tl == "boolean") t = DataType::BOOLEAN;
            else if (tl == "string") t = DataType::STRING;
            else {
                string lowerName = tl;
                auto it = declaredTypes.find(lowerName);
                if (it != declaredTypes.end()) {
                    t = it->second.type;
                    ref = it->second.ref;
                }
            }
        }
    }
    
    for (auto &id : idents) {
        out.push_back(make_shared<VarDeclNode>(id, t, ref));
    }
}

ASTNodePtr ASTBuilder::buildCompoundStatement(shared_ptr<ParseNode> n) {
    auto stmtList = child(n, "<statement-list>");
    auto cs = make_shared<CompoundStmtNode>();
    if (stmtList) {
        for (auto &c : stmtList->children) {
            if (c && c->type == "<statement>") {
                auto s = buildStatement(c);
                if (s) cs->statements.push_back(s);
            }
        }
    }
    return cs;
}

ASTNodePtr ASTBuilder::buildStatement(shared_ptr<ParseNode> n) {
    if (!n) return nullptr;
    for (auto &c : n->children) {
        if (!c) continue;
        if (c->type == "<if-statement>") return buildIfStatement(c);
        if (c->type == "<while-statement>") return buildWhileStatement(c);
        if (c->type == "<for-statement>") return buildForStatement(c);
        if (c->type == "<repeat-statement>") return buildRepeatStatement(c);
        if (c->type == "<case-statement>") return buildCaseStatement(c);
        if (c->type == "<assignment-statement>") return buildAssignmentStatement(c);
        if (c->type == "<procedure/function-call>") return buildProcFuncCall(c);
        if (c->type == "<compound-statement>") return buildCompoundStatement(c);
    }
    return nullptr;
}

ASTNodePtr ASTBuilder::buildAssignmentStatement(shared_ptr<ParseNode> n) {
    auto varNode = child(n, "<variable>");
    auto exprNode = child(n, "<expression>");
    return make_shared<AssignNode>(buildVariable(varNode), buildExpression(exprNode));
}

ASTNodePtr ASTBuilder::buildIfStatement(shared_ptr<ParseNode> n) {
    auto expr = child(n, "<expression>");
    auto stmts = children(n, "<statement>");
    ASTNodePtr thenBranch = nullptr;
    ASTNodePtr elseBranch = nullptr;
    if (stmts.size() > 0) thenBranch = buildStatement(stmts[0]);
    if (stmts.size() > 1) elseBranch = buildStatement(stmts[1]);
    return make_shared<IfNode>(buildExpression(expr), thenBranch, elseBranch);
}

ASTNodePtr ASTBuilder::buildWhileStatement(shared_ptr<ParseNode> n) {
    auto expr = child(n, "<expression>");
    auto compStmt = child(n, "<compound-statement>");
    return make_shared<WhileNode>(buildExpression(expr), buildCompoundStatement(compStmt));
}

ASTNodePtr ASTBuilder::buildRepeatStatement(shared_ptr<ParseNode> n) {
    auto stmtList = child(n, "<statement-list>");
    auto expr = child(n, "<expression>");
    vector<ASTNodePtr> stmts;
    if (stmtList) {
        for (auto &c : stmtList->children) {
            if (c && c->type == "<statement>") {
                auto s = buildStatement(c);
                if (s) stmts.push_back(s);
            }
        }
    }
    return make_shared<RepeatNode>(std::move(stmts), buildExpression(expr));
}

ASTNodePtr ASTBuilder::buildForStatement(shared_ptr<ParseNode> n) {
    auto ident = child(n, "ident");
    auto exprs = children(n, "<expression>");
    auto compStmt = child(n, "<compound-statement>");
    
    ASTNodePtr fromExpr = nullptr;
    ASTNodePtr toExpr = nullptr;
    if (exprs.size() > 0) fromExpr = buildExpression(exprs[0]);
    if (exprs.size() > 1) toExpr = buildExpression(exprs[1]);
    
    bool isDownto = child(n, "downtosy") != nullptr;
    
    return make_shared<ForNode>(ident ? ident->value : "", fromExpr, toExpr, isDownto, buildCompoundStatement(compStmt));
}

ASTNodePtr ASTBuilder::buildCaseStatement(shared_ptr<ParseNode> n) {
    auto expr = child(n, "<expression>");
    auto cNode = make_shared<CaseNode>(buildExpression(expr));
    
    auto blocks = children(n, "<case-block>");
    for (auto &b : blocks) {
        if (!b) continue;
        vector<string> consts;
        auto constNodes = children(b, "<constant>");
        for (auto &cn : constNodes) {
            consts.push_back(resolveConstant(cn).value);
        }
        auto stmt = child(b, "<statement>");
        cNode->branches.push_back({consts, buildStatement(stmt)});
    }
    return cNode;
}

ASTNodePtr ASTBuilder::buildProcFuncCall(shared_ptr<ParseNode> n) {
    string name = "";
    
    // Coba ambil nama dari <variable> dulu
    auto varNode = child(n, "<variable>");
    if (varNode) {
        auto id = child(varNode, "ident");
        if (id) name = id->value;
    }
    // Kalau tidak ada <variable>, ambil langsung dari ident leaf
    if (name.empty()) {
        for (auto &c : n->children) {
            if (c && c->type == "ident") {
                name = c->value;
                break;
            }
        }
    }

    auto pNode = make_shared<ProcCallNode>(name);
    
    auto plist = child(n, "<parameter-list>");
    if (plist) {
        auto exprs = children(plist, "<expression>");
        for (auto &e : exprs) {
            pNode->args.push_back(buildExpression(e));
        }
    }
    return pNode;
}
ASTNodePtr ASTBuilder::buildExpression(shared_ptr<ParseNode> n) {
    if (!n) return nullptr;
    auto simples = children(n, "<simple-expression>");
    auto relOp = child(n, "<relational-operator>");
    
    if (simples.size() == 1) {
        return buildSimpleExpression(simples[0]);
    } else if (simples.size() == 2 && relOp) {
        auto op = relOp->children.empty() ? "" : relOp->children[0]->type;
        return make_shared<BinOpNode>(op, buildSimpleExpression(simples[0]), buildSimpleExpression(simples[1]));
    }
    return nullptr;
}

ASTNodePtr ASTBuilder::buildSimpleExpression(shared_ptr<ParseNode> n) {
    if (!n) return nullptr;
    auto terms = children(n, "<term>");
    auto addOps = children(n, "<additive-operator>");
    
    ASTNodePtr left = nullptr;
    size_t termIdx = 0;
    
    if (n->children.size() > 0 && (n->children[0]->type == "plus" || n->children[0]->type == "minus")) {
        string op = n->children[0]->type;
        if (terms.size() > 0) {
            left = make_shared<UnaryOpNode>(op, buildTerm(terms[0]));
            termIdx = 1;
        }
    } else {
        if (terms.size() > 0) {
            left = buildTerm(terms[0]);
            termIdx = 1;
        }
    }
    
    for (size_t i = 0; i < addOps.size() && termIdx < terms.size(); i++, termIdx++) {
        string op = addOps[i]->children.empty() ? "" : addOps[i]->children[0]->type;
        left = make_shared<BinOpNode>(op, left, buildTerm(terms[termIdx]));
    }
    
    return left;
}

ASTNodePtr ASTBuilder::buildTerm(shared_ptr<ParseNode> n) {
    if (!n) return nullptr;
    auto factors = children(n, "<factor>");
    auto multOps = children(n, "<multiplicative-operator>");
    
    ASTNodePtr left = nullptr;
    if (factors.size() > 0) left = buildFactor(factors[0]);
    
    for (size_t i = 0; i < multOps.size() && i + 1 < factors.size(); i++) {
        string op = multOps[i]->children.empty() ? "" : multOps[i]->children[0]->type;
        left = make_shared<BinOpNode>(op, left, buildFactor(factors[i + 1]));
    }
    
    return left;
}

ASTNodePtr ASTBuilder::buildFactor(shared_ptr<ParseNode> n) {
    if (!n || n->children.empty()) return nullptr;
    
    auto first = n->children[0];
    if (!first) return nullptr;
    
    if (first->type == "intcon") return make_shared<NumberNode>(first->value, DataType::INTEGER);
    if (first->type == "realcon") return make_shared<NumberNode>(first->value, DataType::REAL);
    if (first->type == "charcon") return make_shared<CharNode>(first->value);
    if (first->type == "string") return make_shared<StringNode>(first->value);
    
    if (first->type == "<variable>") return buildVariable(first);
    if (first->type == "<procedure/function-call>") return buildProcFuncCall(first);
    
    if (first->type == "lparent") {
        auto expr = child(n, "<expression>");
        return buildExpression(expr);
    }
    if (first->type == "notsy") {
        auto fac = child(n, "<factor>");
        return make_shared<UnaryOpNode>("notsy", buildFactor(fac));
    }
    return nullptr;
}

ASTNodePtr ASTBuilder::buildVariable(shared_ptr<ParseNode> n) {
    if (!n) return nullptr;
    auto ident = child(n, "ident");
    string name = ident ? ident->value : "";
    ASTNodePtr base = make_shared<VarNode>(name);
    
    auto comps = children(n, "<component-variable>");
    for (auto &comp : comps) {
        if (!comp) continue;
        auto list = child(comp, "<index-list>");
        if (list) {
            vector<ASTNodePtr> indices;
            for (auto &c : list->children) {
                if (!c || c->type == "comma") continue;
                if (c->type == "intcon") indices.push_back(make_shared<NumberNode>(c->value, DataType::INTEGER));
                else if (c->type == "charcon") indices.push_back(make_shared<CharNode>(c->value));
                else if (c->type == "ident") indices.push_back(make_shared<VarNode>(c->value));
            }
            base = make_shared<ArrayAccessNode>(base, indices);
        } else {
            auto field = child(comp, "ident");
            if (field) {
                base = make_shared<FieldAccessNode>(base, field->value);
            }
        }
    }
    return base;
}

ASTBuilder::TypeInfo ASTBuilder::resolveType(shared_ptr<ParseNode> n) {
    TypeInfo info{DataType::UNKNOWN, 0};
    if (!n) return info;
    
    auto arr = child(n, "<array-type>");
    if (arr) return resolveArrayType(arr);
    
    auto enm = child(n, "<enumerated>");
    if (enm) {
        TypeInfo einfo{DataType::ENUMERATED, 0};
        int newBlock = st.enterBtab();
        einfo.ref = newBlock;
        
        int valIdx = 0;
        for (auto &c : enm->children) {
            if (c && c->type == "ident") {
                st.enterTab(
                    c->value,
                    AllowedObj::CONSTANT,
                    DataType::ENUMERATED,
                    newBlock,
                    1,
                    st.currentLevel,
                    valIdx++
                );
            }
        }
        return einfo;
    }

    auto rec = child(n, "<record-type>");
    if (rec) {
        TypeInfo rinfo{DataType::RECORD, 0};
        int newBlock = st.pushScope();
        rinfo.ref = newBlock;
        
        auto fieldList = child(rec, "<field-list>");
        if (fieldList) {
            auto fieldParts = children(fieldList, "<field-part>");
            for (auto &part : fieldParts) {
                if (!part) continue;
                auto idList = child(part, "<identifier-list>");
                auto typeNode = child(part, "<type>");
                if (idList && typeNode) {
                    vector<string> idents = collectIdents(idList);
                    TypeInfo tinfo = resolveType(typeNode);
                    
                    for (const auto &id : idents) {
                        int offset = st.btab[newBlock].vsze;
                        int elsz = 1;
                        if (tinfo.type == DataType::REAL) elsz = 2;
                        else if (tinfo.type == DataType::STRING) elsz = 4;
                        else if (tinfo.type == DataType::ARRAY) elsz = st.atab[tinfo.ref].size;
                        else if (tinfo.type == DataType::RECORD) elsz = st.btab[tinfo.ref].vsze;
                        
                        st.btab[newBlock].vsze += elsz;
                        
                        st.enterTab(
                            id,
                            AllowedObj::VARIABLE,
                            tinfo.type,
                            tinfo.ref,
                            1,
                            st.currentLevel,
                            offset
                        );
                    }
                }
            }
        }
        st.popScope();
        return rinfo;
    }
    
    auto ident = child(n, "ident");
    if (ident) {
        string tl = ident->value;
        for (auto &ch : tl) ch = tolower(ch);
        if (tl == "integer") info.type = DataType::INTEGER;
        else if (tl == "real") info.type = DataType::REAL;
        else if (tl == "char") info.type = DataType::CHAR;
        else if (tl == "boolean") info.type = DataType::BOOLEAN;
        else if (tl == "string") info.type = DataType::STRING;
        else {
            string lowerName = tl;
            auto it = declaredTypes.find(lowerName);
            if (it != declaredTypes.end()) {
                info = it->second;
            }
        }
    }
    return info;
}

ASTBuilder::TypeInfo ASTBuilder::resolveArrayType(shared_ptr<ParseNode> n) {
    TypeInfo info{DataType::ARRAY, 0};
    
    auto rangeNode = child(n, "<range>");
    DataType xtyp = DataType::INTEGER;
    int low = 0, high = 0;
    
    if (rangeNode) {
        auto consts = children(rangeNode, "<constant>");
        if (consts.size() == 2) {
            auto c1 = resolveConstant(consts[0]);
            auto c2 = resolveConstant(consts[1]);
            xtyp = c1.type;
            try { low = stoi(c1.value); } catch(...) {}
            try { high = stoi(c2.value); } catch(...) {}
        }
    }
    
    auto elemTypeNode = child(n, "<type>");
    TypeInfo elemType = resolveType(elemTypeNode);
    
    int elsz = 1;
    if (elemType.type == DataType::REAL) elsz = 2;
    else if (elemType.type == DataType::STRING) elsz = 4;
    else if (elemType.type == DataType::ARRAY) {
        elsz = st.atab[elemType.ref].size;
    }
    
    info.ref = st.enterAtab(xtyp, elemType.type, elemType.ref, low, high, elsz);
    return info;
}

ASTBuilder::ConstInfo ASTBuilder::resolveConstant(shared_ptr<ParseNode> n) {
    ConstInfo info{DataType::UNKNOWN, ""};
    if (!n) return info;
    string sign = "";
    for (auto &c : n->children) {
        if (!c) continue;
        if (c->type == "plus") sign = "";
        else if (c->type == "minus") sign = "-";
        else if (c->type == "intcon") { info.type = DataType::INTEGER; info.value = sign + c->value; }
        else if (c->type == "realcon") { info.type = DataType::REAL; info.value = sign + c->value; }
        else if (c->type == "charcon") { info.type = DataType::CHAR; info.value = c->value; }
        else if (c->type == "string") { info.type = DataType::STRING; info.value = c->value; }
        else if (c->type == "ident") { 
            info.type = DataType::UNKNOWN; 
            info.value = sign + c->value; 
        }
    }
    return info;
}
