# Parser Arion — IF2224 Teori Bahasa Formal dan Otomata
## Milestone 4

Arion adalah compiler lengkap yang mencakup seluruh pipeline dari source code hingga eksekusi: Lexer → Parser → AST Builder → Semantic Analyzer → Intermediate Code Generator (ICG) → Stack-Machine Interpreter.

---

## Anggota Kelompok

| Nama | NIM |
|------|-----|
| Muhammad Jordan Ferimeison | 13524047 |
| Arina Azka | 13524049 |
| Hakam Avicena Mustain | 13524075 |
| Yavie Azka Putra Araly | 13524077 |
| Angelina Andra Alanna | 13524079 |

## Struktur Proyek

```
.
├── src/
│   ├── main.cpp              # Entry point: orchestrasi pipeline kompilasi
│   ├── lexer.cpp / lexer.h   # Lexer berbasis DFA
│   ├── dfa.h / token.h       # Definisi state DFA dan token
│   ├── Parser.cpp / Parser.h # Recursive descent parser → Parse Tree
│   ├── ASTNode.h / ASTNode.cpp      # Definisi semua node Decorated AST
│   ├── ASTBuilder.cpp / ASTBuilder.h # Konversi Parse Tree → AST
│   ├── symbolTable.cpp / symbolTable.hpp # Symbol table bertingkat (scope)
│   ├── semanticAnalyzer.cpp / semanticAnalyzer.h # Semantic analysis
│   ├── TypeSystem.cpp / TypeSystem.h # Type checking & inferensi
│   ├── ErrorHandler.cpp / ErrorHandler.h # Pelaporan error semantik
│   ├── ICG.cpp / ICG.h       # Intermediate Code Generator (PL/0-like)
│   └── Interpreter.cpp / Interpreter.hpp # Stack Machine Interpreter
├── test/
│   └── milestone-4/          # 15 test case beserta output-nya
├── docs/                     # Laporan
├── bin/                      # Binary hasil kompilasi
└── Makefile
```

---

## Cara Build dan Jalankan

### Build

```bash
make
```

Menghasilkan binary di `bin/program`.

### Jalankan

```bash
./bin/program <input> <output>
```

- `input` - source code Arion
- `output` - file output

---

## Pipeline Kompilasi

| Fase             | Output           | Deskripsi             |
|------------------|------------------|-----------------------|
| Lexer            | Token stream     | DFA-based             |
| Parser           | Parse Tree       | Recursive Descent     |
| ASTBuilder       | Decorated AST    | Symbol Table          |
| SemanticAnalyzer | Type checking    | Scope resolution      |
| ICG              | Intermediate Code| Instruksi PL/0-like   |
| Interpreter      | Eksekusi         | via Stack Machine     |

---

## Intermediate Code Generator (`ICG.cpp`)
Menghasilkan instruksi TAC:

| Instruksi | Deskripsi |
|-----------|-----------|
| `LIT 0 n` | Push literal `n` ke stack |
| `LOD l a` | Load variabel dari level `l`, address `a` |
| `STO l a` | Store ke variabel level `l`, address `a` |
| `CAL l a` | Panggil fungsi/prosedur di level `l`, baris `a` |
| `INT 0 n` | Alokasi `n` slot di stack frame |
| `JMP 0 a` | Lompat tanpa syarat ke baris `a` |
| `JPC 0 a` | Lompat ke baris `a` jika top-of-stack = 0 (false) |
| `OPR 0 k` | Operasi: aritmatika, perbandingan, WRT/WRTLN, READ |
| `RET 0 0` | Return dari fungsi/prosedur |
| `LODA l a` | Load address (untuk array/record) |
| `STOA` | Store via address |
| `CKB lo hi` | Cek bounds indeks array |


**OPR sub-codes:**

| Kode | Operasi |
|------|---------|
| 1 | NEG (negasi unary) |
| 2 | ADD |
| 3 | SUB |
| 4 | MUL |
| 5 | DIV |
| 6 | MOD |
| 7 | EQL (=) |
| 8 | NEQ (<>) |
| 9 | LSS (<) |
| 10 | GEQ (>=) |
| 11 | GTR (>) |
| 12 | LEQ (<=) |
| 13 | WRT (write) |
| 14 | WRTLN (writeln) |
| 15 | PUSHBP (push base pointer, untuk return value) |
| 16 | RED (read) |

ICG mendukung: ekspresi aritmatika, IF-ELSE, WHILE, FOR, REPEAT-UNTIL, CASE, pemanggilan dan deklarasi fungsi/prosedur bersarang, array (LODA/STOA/CKB), record (field access), string/real literal pool.

## Interpreter (`Interpreter.cpp`)
Stack Machine dengan siklus Fetch-Decode-Execute:
- Stack frame: `[static link | dynamic link | return address | variabel lokal...]`
- Akses variabel lintas scope via static link chain (`base(level, bp)`)
- Mendukung tipe data: integer, real, char, boolean, string
- Output melalui `OPR WRT` / `OPR WRTLN`, input melalui `OPR RED`

**Error Runtime yang Ditangani:**

| Error | Deskripsi |
|-------|-----------|
| `RuntimeError` | Error runtime generik |
| `StackOverflowError` | Kedalaman frame melebihi batas (1000 frame) |
| `StackUnderflowError` | Pop pada stack kosong |
| `StackSmashingError` | Control frame corrupt |
| `StackCorruptionError` | Inkonsistensi stack |
| `InvalidJumpError` | Target jump di luar range instruksi |
| `OutOfBoundsError` | Akses memori di luar frame |
| `IndexOutOfBoundsError` | Indeks array di luar range `[low..high]` |
| `DivisionByZeroError` | Pembagian dengan nol |
| `ArithmeticOverflowError` | Overflow integer (range: −2147483648..2147483647) |

---

## Fitur Bahasa yang Didukung

- **Tipe data:** `Integer`, `Real`, `Char`, `Boolean`, `String`
- **Tipe terstruktur:** Array (`array [low..high] of T`), Record
- **Konstanta:** `const`
- **Deklarasi tipe:** `type`
- **Variabel:** `var`, scope bersarang
- **Kontrol alur:** `if-then-else`, `while-do`, `for-to/downto-do`, `repeat-until`, `case-of`
- **Subprogram:** `procedure` dan `function` dengan parameter dan scope bersarang
- **I/O:** `write`, `writeln`, `read`, `readln`
- **Ekspresi:** aritmatika, relasional, boolean (`and`, `or`, `not`), unary minus

---

## Output Program

Saat dijalankan, program mencetak ke terminal dan menyimpan ke file output:
1. **Parse Tree** — representasi pohon dari sintaks
2. **Intermediate Code** — daftar instruksi yang dihasilkan ICG
3. **Output Program** — hasil eksekusi (`write`/`writeln`)
4. **Symbol Table** — isi tabel simbol akhir
5. **Semantic Analysis Summary** — jumlah error/warning dan status
6. **Decorated AST** — AST beranotasi tipe dan scope
