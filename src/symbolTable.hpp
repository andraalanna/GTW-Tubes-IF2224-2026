#pragma once
#include <string>
#include <vector>

using namespace std;

// Jenis objek yang bisa masuk ke symbol table
enum class AllowedObj{
    CONSTANT,
    VARIABLE,
    TYPE,
    FUNCTION,
    PROCEDURE,
    PROGRAM, // nama program
    KEYWORD  // reserved words
};

// jenis tipe data
enum class DataType {
    INTEGER,
    REAL,
    CHAR,
    BOOLEAN,
    STRING,
    SUBRANGE,
    ENUMERATED,
    ARRAY,
    RECORD,
    VOID, // untuk prosedur / statement
    UNKNOWN // belum diketahui atau error
};

// indeks 0-32 dipakai untuk reserved words dan predefined indentifiers
// Penjelasan di spesifkasi
struct TabEntry{
    string name;
    int link;
    AllowedObj obj;
    DataType type;
    int ref; 
    int nrm;
    int lev; 
    int adr; 
};

struct BTabEntry{
    int last; 
    int lpar;
    int psze;
    int vsze;
};

struct ATabEntry{
    DataType xtyp;
    DataType etyp;
    int eref;
    int low;
    int high;
    int elsz;
    int size;
};

namespace PredefinedIdx {
    constexpr int TYPE_INTEGER  = 22;
    constexpr int TYPE_REAL     = 23;
    constexpr int TYPE_BOOLEAN  = 24;
    constexpr int TYPE_CHAR     = 25;
    constexpr int TYPE_STRING   = 26;
    // built-in constants and procedures
    constexpr int CONST_TRUE    = 33;
    constexpr int CONST_FALSE   = 34;
    constexpr int PROC_WRITELN  = 35;
    constexpr int PROC_READLN   = 36;
    // Identifier user mulai dari indeks 37
    constexpr int USER_START    = 37;
}

class SymbolTable{
    public:
    // Tabel utama
    vector<TabEntry> tab;
    vector<ATabEntry> atab;
    vector<BTabEntry> btab;

    // state scope saat ini;
    int currentLevel; // lexical level 
    int currentBlock; // indeks btab dari blok aktif
    
    //display[level] = indeks btab dari blok pada level tersebut
    vector<int> display;

    // inisialisasi
    SymbolTable();
    void init();

    // operasi tab
    // masukin ident baru ke tab, return indeksnya
    int enterTab(const string& name, AllowedObj obj, DataType type, int ref, int nrm, int lev, int adr);

    int lookup(const std::string& name) const;
 
    // Cari identifier hanya di scope (blok) saat ini
    // Untuk deteksi redeclaration
    int lookupCurrentScope(const std::string& name) const;
 
    // Operasi btab
    // Buat entri blok baru di btab, return indeksnya
    int enterBtab(int last = 0, int lpar = 0, int psze = 0, int vsze = 0);
 
    // Operasi atab
    // Buat entri array baru di atab, return indeksnya
    int enterAtab(DataType xtyp, DataType etyp, int eref,
                  int low, int high, int elsz);

    // Masuk ke blok baru (prosedur, fungsi, record, atau compound statement)
    // Return indeks btab blok baru
    int pushScope();
 
    // Keluar dari blok aktif, kembali ke blok parent
    void popScope();
 

    // Konversi DataType ke string untuk print
    static string DataTypeToString(DataType t);
 
    // Konversi AllowedObj ke string untuk print
    static string AllowedObjToString(AllowedObj o);
 
    // Print isi tab, btab, atab ke terminal
    void printTables() const;
 
private:
    // Helper: inisialisasi satu predefined identifier ke tab
    void addPredefined(const string& name, AllowedObj obj, DataType type,
                       int ref, int nrm, int lev, int adr);
};
 