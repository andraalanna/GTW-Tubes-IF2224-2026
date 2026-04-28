#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "token.h"
#include "dfa.h"

using namespace std;

/**
 * File lexer.h sebagai blueprint objek lexer,
 * semua fungsi dan data yang dibutuhkan lexer.
 */
class Lexer
{
private:
    string source;      // Nyimpan seluruh isi source code
    int pos;            // Posisi karakter yang sedang dibaca
    State currentState; // State DFA saat ini

    // Melihat karakter di posisi sekarang TANPA MAJU,
    // dipakai untuk cek karakter berikutnya.
    char peek();

    // Membaca karakter di posisi sekarang lalu MAJU ke berikutnya
    //  Dipakai ketika sudah yakin karakter ini bagian dari TOKEN
    char advance();

    // Mengecek apakah string val adalah keyword Arion
    // if yes -> return nama token keyword (mis: "beginsy")
    // if no -> return "ident"
    // bersifat case-INSENSITIVE ("BEGIN" == "begin")
    string classifyKeyword(string val);

    // Membaca angka dan menghasilkan token intcon atau realcon
    // Dipanggil dari tokenize() ketika peek() adalah digit (0-9)
    Token readNumber();

    // token charcon (1 karakter) atau string (lebih dari 1)
    // Dipanggil dari tokenize() ketika peek() adalah '\''
    Token readString();

    // Dipanggil dari tokenize() ketika peek() adalah + - * / = < >
    Token readOperator(char ch);

    // Dipanggil dari tokenize() ketika peek() adalah : ( ) [ ] , ; .
    Token readPunctuation(char ch);

    // Menghasilkan token ident atau salah satu dari 27 keyword Arion
    // Dipanggil dari tokenize() ketika peek() adalah huruf (a-z, A-Z)
    Token readIdentOrKeyword(char ch);

    // ch = karakter pembuka komentar yang sudah diketahui
    // Dipanggil dari tokenize() ketika peek() adalah { atau (*
    Token readComment(char ch);

    // Untuk baca karakter yang tidak diketahui
    Token readUnknown(char ch);

public:
    Lexer(string src);
    vector<Token> tokenize();
};

#endif