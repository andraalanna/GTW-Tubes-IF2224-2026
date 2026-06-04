#include <iomanip>
#include <iostream>
#include <stdexcept>
#include "symbolTable.hpp"
using namespace std;


SymbolTable::SymbolTable(){
    init();
}
void SymbolTable::init(){   
    //clear dulu semua
    tab.clear();
    atab.clear();
    btab.clear();
    display.clear();
    currentLevel = 0;
    currentBlock = 0;

    enterBtab(0,0,0,0);
    // slot 0 i tab dikosongin, krn link = 0 dipakai sebagai tanda sudah habis saat nelusurin linked list.
    tab.push_back({"", 0, AllowedObj::VARIABLE, DataType::UNKNOWN, 0, 0, 0, 0});

    // tambahin 32 reserved words (indeks 1 - 32)
    const vector<string> reservedWords = {
        "AND", "ARRAY", "BEGIN", "CASE", "CONST", "DIV", "DOWNTO", "DO",
        "ELSE", "END", "FOR", "FUNCTION", "IF", "MOD", "NOT", "OF",
        "OR", "PROCEDURE", "PROGRAM", "RECORD", "REPEAT",
        "INTEGER", "REAL", "BOOLEAN", "CHAR", "STRING",
        "THEN", "TO", "TYPE", "UNTIL", "VAR", "WHILE"
    };

    for (int i = 0; i < (int)reservedWords.size(); i++) {
        string w = reservedWords[i];
        if (i >= 21 && i <= 25) {
            DataType dt = DataType::UNKNOWN;
            if (w == "INTEGER") dt = DataType::INTEGER;
            else if (w == "REAL") dt = DataType::REAL;
            else if (w == "BOOLEAN") dt = DataType::BOOLEAN;
            else if (w == "CHAR") dt = DataType::CHAR;
            else if (w == "STRING") dt = DataType::STRING;
            addPredefined(w, AllowedObj::TYPE, dt, 0, 1, 0, 0);
        } else {
            addPredefined(w, AllowedObj::KEYWORD, DataType::UNKNOWN, 0, 1, 0, 0);
        }
    }

    // constant
    addPredefined("true", AllowedObj::CONSTANT, DataType::BOOLEAN, 0, 1, 0, 1); // idx 33
    addPredefined("false", AllowedObj::CONSTANT, DataType::BOOLEAN, 0, 1, 0, 0); // idx 34

    // procedure
    addPredefined("writeln", AllowedObj::PROCEDURE, DataType::VOID, 0, 1, 0, 0); // idx 35
    addPredefined("readln", AllowedObj::PROCEDURE, DataType::VOID, 0, 1, 0, 0); // idx 36
    addPredefined("write",   AllowedObj::PROCEDURE, DataType::VOID, 0, 1, 0, 0); 
    
    // slot kosong dulu sampe USER_START
    for (int i = (int)tab.size(); i < PredefinedIdx::USER_START; i++) {
        tab.push_back({"", 0, AllowedObj::VARIABLE, DataType::UNKNOWN, 0, 0, 0, 0});
    }

    // setup display dan global block
    display.push_back(0);
}

void SymbolTable::addPredefined(const string& name, AllowedObj obj, DataType type, int ref, int nrm, int lev, int adr){
    int link = btab[currentBlock].last;
    tab.push_back({name, link, obj, type, ref, nrm, lev, adr});
    btab[currentBlock].last = (int)tab.size()-1;

}

// operasi tab
// masukin ident baru ke tab, return indeksnya
int SymbolTable::enterTab(const string& name, AllowedObj obj, DataType type, int ref, int nrm, int lev, int adr){
    int link = btab[currentBlock].last;
    TabEntry entry;
    entry.name = name;
    entry.link = link;
    entry.obj  = obj;
    entry.type = type;
    entry.ref  = ref;
    entry.nrm  = nrm;
    entry.lev  = lev;
    entry.adr  = adr;
 
    tab.push_back(entry);
    int idx = (int)tab.size() - 1;
 
    btab[currentBlock].last = idx;
 
    return idx;
}

int SymbolTable::lookup(const string& name) const {
    for (int lvl = currentLevel; lvl >= 0; lvl--) {
        int blockIdx = display[lvl];
        int i = btab[blockIdx].last;
        // Telusuri linked list identifier di blok ini
        while (i > 0) {
            if (tab[i].name == name) return i;
            i = tab[i].link;
        }
    }
    return -1;
}

int SymbolTable::lookupCurrentScope(const string &name) const
{
    int i = btab[currentBlock].last;
    while (i > 0)
    {
        
        if (tab[i].lev < currentLevel && currentLevel > 0)
            break;
        if (tab[i].name == name)
            return i;
        i = tab[i].link;
    }
    return -1;
}

// Operasi btab
// Buat entri blok baru di btab, return indeksnya
int SymbolTable::enterBtab(int last, int lpar, int psze, int vsze) {
    BTabEntry entry;
    entry.last = last;
    entry.lpar = lpar;
    entry.psze = psze;
    entry.vsze = vsze;
    btab.push_back(entry);
    return (int)btab.size() - 1;
}

// Operasi atab
// Buat entri array baru di atab, return indeksnya
int SymbolTable::enterAtab(DataType xtyp, DataType etyp, int eref,
                            int low, int high, int elsz) {
    ATabEntry entry;
    entry.xtyp = xtyp;
    entry.etyp = etyp;
    entry.eref = eref;
    entry.low  = low;
    entry.high = high;
    entry.elsz = elsz;
    entry.size = (high - low + 1) * elsz;
    atab.push_back(entry);
    return (int)atab.size() - 1;
}

int SymbolTable::pushScope() {
    currentLevel++;
 
    // Buat blok baru di btab
    int newBlock = enterBtab(0, 0, 0, 0);
    currentBlock = newBlock;
 
    // Update display
    if (currentLevel < (int)display.size()) {
        display[currentLevel] = newBlock;
    } else {
        display.push_back(newBlock);
    }
 
    return newBlock;
}
 
void SymbolTable::popScope() {
    if (currentLevel <= 0) {
        throw runtime_error("SymbolTable::popScope() dipanggil saat sudah di level global");
    }
 
    // Kembali ke blok parent
    currentLevel--;
    currentBlock = display[currentLevel];
}
 
// Utilitas
string SymbolTable::DataTypeToString(DataType t) {
    switch (t) {
        case DataType::INTEGER:    return "Integer";
        case DataType::REAL:       return "Real";
        case DataType::CHAR:       return "Char";
        case DataType::BOOLEAN:    return "Boolean";
        case DataType::STRING:     return "String";
        case DataType::SUBRANGE:   return "Subrange";
        case DataType::ENUMERATED: return "Enumerated";
        case DataType::ARRAY:      return "Array";
        case DataType::RECORD:     return "Record";
        case DataType::VOID:       return "Void";
        case DataType::UNKNOWN:    return "Unknown";
        default:                   return "?";
    }
}
 
string SymbolTable::AllowedObjToString(AllowedObj o) {
    switch (o) {
        case AllowedObj::CONSTANT:  return "constant";
        case AllowedObj::VARIABLE:  return "variable";
        case AllowedObj::TYPE:      return "type";
        case AllowedObj::PROCEDURE: return "procedure";
        case AllowedObj::FUNCTION:  return "function";
        case AllowedObj::PROGRAM:   return "program";
        case AllowedObj::KEYWORD:   return "keyword";
        default:                    return "?";
    }
}
 
// Print isi tabel ke terminal 
void SymbolTable::printTables() const {
    // tab 
    cout << "\n TAB (Identifier Table) " << endl;
    cout << left
         << setw(5)  << "idx"
         << setw(16) << "name"
         << setw(12) << "obj"
         << setw(12) << "type"
         << setw(5)  << "ref"
         << setw(5)  << "nrm"
         << setw(5)  << "lev"
         << setw(5)  << "adr"
         << setw(5)  << "link"
         << endl;
    cout << string(70, '-') << endl;
 
    for (int i = 1; i < (int)tab.size(); i++) {
        const TabEntry& e = tab[i];
        if (e.name.empty()) continue; // skip slot kosong
        cout << left
             << setw(5)  << i
             << setw(16) << e.name
             << setw(12) << AllowedObjToString(e.obj)
             << setw(12) << DataTypeToString(e.type)
             << setw(5)  << e.ref
             << setw(5)  << e.nrm
             << setw(5)  << e.lev
             << setw(5)  << e.adr
             << setw(5)  << e.link
             << endl;
    }
 
    //  btab 
    cout << "\n BTAB (Block Table)" << endl;
    cout << left
         << setw(5)  << "idx"
         << setw(8)  << "last"
         << setw(8)  << "lpar"
         << setw(8)  << "psze"
         << setw(8)  << "vsze"
         << endl;
    cout << string(40, '-') << endl;
 
    for (int i = 0; i < (int)btab.size(); i++) {
        const BTabEntry& e = btab[i];
        cout << left
             << setw(5) << i
             << setw(8) << e.last
             << setw(8) << e.lpar
             << setw(8) << e.psze
             << setw(8) << e.vsze
             << endl;
    }
 
    //  atab 
    cout << "\n ATAB (Array Table)" << endl;
    if (atab.empty()) {
        cout << "(kosong)" << endl;
    } else {
        cout << left
             << setw(5)  << "idx"
             << setw(12) << "xtyp"
             << setw(12) << "etyp"
             << setw(6)  << "eref"
             << setw(6)  << "low"
             << setw(6)  << "high"
             << setw(6)  << "elsz"
             << setw(6)  << "size"
             << endl;
        cout << string(60, '-') << endl;
 
        for (int i = 0; i < (int)atab.size(); i++) {
            const ATabEntry& e = atab[i];
            cout << left
                 << setw(5)  << i
                 << setw(12) << DataTypeToString(e.xtyp)
                 << setw(12) << DataTypeToString(e.etyp)
                 << setw(6)  << e.eref
                 << setw(6)  << e.low
                 << setw(6)  << e.high
                 << setw(6)  << e.elsz
                 << setw(6)  << e.size
                 << endl;
        }
    }
}