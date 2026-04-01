#ifndef DFA_H
#define DFA_H
/**
 * File dfa.h befungsi untuk mendefinisikan semua state keadaan dalam DFA yang dipakai lexer
 */
enum State
{
    // State awal
    S0,

    // Konstata
    S_INT,
    S_REAL_DOT,
    S_REAL,

    // string & charcon
    S_QUOTE,
    S_CHAR,

    // identifier & charcon
    S_IDENT,

    // operator aritmatika
    S_PLUS,  // '+'
    S_MINUS, // '-'
    S_TIMES, // '*'
    S_RDIV,  // '/'

    // operator perbandingan
    S_EQL1,         // baru baca "=" yang pertama -> =
    S_EQL2,         // sudah baca "=" yang kedua ->  ==
    S_LESS,         // <
    S_LESSEQUAL,    // <=
    S_NOTEQUAL,     // <>
    S_GREATER,      // >
    S_GREATEREQUAL, // >=

    // tanda baca & assignment
    S_COLON,     // baru baca ':'
    S_BECOMES,   // sudah baca ':' + '=' -> ':='
    S_LPAR,      // '('
    S_RPAR,      //  ')'
    S_LBRACK,    //  '['
    S_RBRACK,    // ']'
    S_COMMA,     // ','
    S_SEMICOLON, // ';'
    S_PERIOD,    // '.'

    // komentar
    S_CMT1,      // di dalam komentar { ... }
    S_CMT2,      // di dalam komentar (* ... *)
    S_CMT2_STAR, // sudah baca '*' di dalam (* ... *), nunggu ')'

    // error
    S_ERROR,

};

#endif