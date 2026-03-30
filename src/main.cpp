#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"

using namespace std;

int main(int argc, char* argv[]) {

    // Pastikan user memasukkan dua argumen:
    // argv[1] = nama file input
    // argv[2] = nama file output
    // Contoh: ./lexer input.txt output.txt
    if (argc < 3) {
        cerr << "Usage: ./lexer input.txt output.txt" << endl;
        return 1;
    }

    // Buka file input untuk dibaca
    ifstream inFile(argv[1]);

    // Cek apakah file berhasil dibuka
    if (!inFile) {
        cerr << "ERROR: file tidak ditemukan: " << argv[1] << endl;
        return 1;
    }

    // Baca seluruh isi file input menjadi satu string
    // stringstream dipakai sebagai jembatan antara file dan string
    stringstream ss;
    ss << inFile.rdbuf();
    string source = ss.str();

    // Tutup file input setelah selesai dibaca
    inFile.close();

    // Buat objek Lexer dengan source code yang sudah dibaca
    // Constructor Lexer akan menyimpan source dan set pos = 0
    Lexer lexer(source);

    // Jalankan lexer → hasilkan semua token dari source code
    vector<Token> tokens = lexer.tokenize();

    // Buka file output untuk ditulis
    ofstream outFile(argv[2]);

    // Cek apakah file output berhasil dibuka
    if (!outFile) {
        cerr << "ERROR: tidak bisa membuat file: " << argv[2] << endl;
        return 1;
    }

    // Tulis setiap token ke file output
    for (Token t : tokens) {
        // Token tertentu perlu ditampilkan dengan value-nya
        // Contoh: intcon (48), ident (myVar), string ('IRK')
        // Token lain cukup tampilkan type-nya saja
        // Contoh: plus, semicolon, beginsy
        if (tokenHasValue(t.type))
            outFile << t.type << " (" << t.value << ")" << endl;
        else
            outFile << t.type << endl;
    }

    // Tutup file output setelah selesai ditulis
    outFile.close();

    // Beri tahu user bahwa program selesai
    cout << "Selesai! Output tersimpan di: " << argv[2] << endl;

    return 0;
}