# Parser Arion — IF2224 Teori Bahasa Formal dan Otomata
## Milestone 2: Syntax Analysis

Parser untuk bahasa pemrograman **Arion** yang mengimplementasikan **Recursive Descent Parser** dan menghasilkan **Parse Tree** dari source code Arion.

---

## Anggota Kelompok

| Nama | NIM |
|------|-----|
| Muhammad Jordan Ferimeison | 13524047 |
| Arina Azka | 13524049 |
| Hakam Avicena Mustain | 13524075 |
| Yavie Azka Putra Araly | 13524077 |
| Angelina Andra Alanna | 13524079 |

---

## Deskripsi Program

Program ini menerima source code Arion sebagai input, melakukan analisis leksikal (lexer dari Milestone 1) dan analisis sintaks (parser), kemudian menghasilkan parse tree yang dicetak ke terminal dan disimpan ke file output.

---

## Struktur Repository

```
.
├── bin/                  # Direktori output executable (dibuat otomatis saat build)
│   └── program
├── src/
│   ├── main.cpp          # Entry point program
│   ├── lexer.cpp         # Implementasi lexer (Milestone 1)
│   ├── lexer.h           # Header lexer
│   ├── Parser.cpp        # Implementasi parser
│   ├── Parser.h          # Header parser
│   ├── token.h           # Definisi struct Token
│   └── dfa.h             # Definisi state DFA lexer
├── Makefile
└── README.md
```

---

## Dependensi

- **Compiler:** `g++`
- **Standar C++:** C++17
- **OS:** Linux / macOS (atau Windows dengan MinGW)

---

## Build

```bash
make
```

Executable akan tersimpan di `bin/program`. Untuk membersihkan hasil build:

```bash
make clean
```

---

## Cara Menjalankan

```bash
./bin/program <path_to_input.txt> <path_to_output.txt>
```

**Contoh:**

```bash
./bin/program test/input.txt output/result.txt
```

- `path_to_input.txt` — file source code Arion yang akan diparse
- `path_to_output.txt` — file tujuan penyimpanan parse tree

Parse tree juga akan dicetak langsung ke terminal.

---

## Contoh Input dan Output

**Input (`input.txt`):**
```
program Contoh;
type
    Point == record
        x, y: integer
    end;
var
    p: Point;
begin
    p.x := 10;
    p.y := 20
end.
```

**Output (parse tree):**
```
<program>
├── <program-header>
│   ├── programsy
│   ├── ident(Contoh)
│   └── semicolon
├── <declaration-part>
│   └── ...
└── ...
```

---

## Fitur Parser

Parser mengimplementasikan grammar Arion untuk node-node berikut:

- **Deklarasi:** `<type-declaration>`, `<var-declaration>`, `<const-declaration>`
- **Tipe data:** `<type>`, `<array-type>`, `<range>`, `<enumerated>`, `<record-type>`
- **Statement:** `<compound-statement>`, `<statement-list>`, `<statement>`, `<assignment-statement>`, `<if-statement>`, `<while-statement>`, `<for-statement>`, `<repeat-statement>`, `<case-statement>`
- **Ekspresi:** `<expression>`, `<simple-expression>`, `<term>`, `<factor>`
- **Variable:** `<variable>`, `<component-variable>`, `<index-list>`
- **Subprogram:** `<procedure-declaration>`, `<function-declaration>`

---

## Error Handling

Parser menampilkan pesan error sintaks yang informatif ketika source code tidak sesuai grammar, beserta token yang tidak terduga dan token yang diharapkan. Contoh:

```
Syntax error at line 3, col 17: unexpected token 'realcon(1.10)', expected type (ident, array-type, range, enumerated, or record-type)
```
