# Work Breakdown Structure — Milestone 4
**Arion Compiler | IF2224 - Teori Bahasa Formal dan Otomata**

> Deadline: **Kamis, 4 Juni 2026, pukul 23.59 WIB**
> Release tag: `v0.4.1` | File laporan: `Laporan-4-[KODE].pdf` di folder `/doc`

---

## Anggota 1 — Setup & ICG: Ekspresi

**Fokus:** Arsitektur proyek · ICG untuk assignment & ekspresi aritmatika

### Implementasi
- Setup struktur repositori dan antarmuka antar-modul (ICG, Stack, Interpreter)
  - Tentukan format Intermediate Code output dan format input Decorated AST di awal, agar semua anggota bisa paralel
- ICG: Inisiasi memori — instruksi `INT`
  - Hitung ukuran memori dari jumlah VarDecl di setiap scope (3 slot fix + n variabel)
- ICG: Assignment — instruksi `LIT`, `LOD`, `STO`
  - Traversal node Assign di AST, resolve address variabel dari symbol table
- ICG: Ekspresi aritmatika — `OPR` (ADD, SUB, MUL, DIV, MOD, NEG)
  - Post-order traversal BinaryExpr, generate variabel sementara bila perlu

### Bagian Laporan
- Teori: arsitektur front-end vs back-end, Three-Address Code
- Implementasi: perancangan struktur program & modul ICG ekspresi
- Cover + daftar isi + pembagian tugas kelompok

---

## Anggota 2 — ICG: Control Flow

**Fokus:** ICG untuk IF-ELSE, WHILE, dan Write statement

### Implementasi
- ICG: IF-ELSE — instruksi `JPC` dan `JMP`
  - Generate label sementara, patch jump target setelah blok then/else selesai di-generate
- ICG: WHILE loop — label balik dan conditional jump
  - Simpan posisi label awal sebelum kondisi, patch JPC ke after-body setelah body selesai
- ICG: Perbandingan — `OPR` (EQL, NEQ, LSS, GEQ, GTR, LEQ)
  - Digunakan sebagai kondisi untuk JPC pada IF dan WHILE
- ICG: Write & Writeln — `OPR WRT` dan `OPR WRTLN`
  - Load ekspresi ke stack, kemudian panggil OPR 13/14

### Bagian Laporan
- Teori: translasi control flow (IF-ELSE, WHILE) ke TAC, penggunaan label & jump
- Implementasi: ICG control flow & perbandingan beserta contoh instruksi yang dihasilkan

---

## Anggota 3 — ICG: Fungsi & Prosedur

**Fokus:** ICG untuk function call, scope bersarang, dan return

### Implementasi
- ICG: Deklarasi fungsi/prosedur — instruksi `INT` per scope baru
  - Setiap fungsi/prosedur punya frame sendiri; hitung ukuran memorinya dari VarDecl + parameter
- ICG: Pemanggilan fungsi — instruksi `CAL`
  - Resolve target baris instruksi fungsi, pasang static/dynamic link yang benar
- ICG: Return dari fungsi/prosedur — instruksi `RET`
  - Termasuk pengembalian nilai jika itu fungsi (bukan prosedur)
- Manajemen level scope dan resolusi address variabel lintas scope
  - Variabel di scope luar diakses dengan level berbeda di instruksi LOD/STO

### Bagian Laporan
- Teori: Decorated AST, scope & symbol table, static/dynamic link
- Implementasi: ICG fungsi & prosedur, resolusi scope, contoh instruksi CAL/RET

---

## Anggota 4 — Interpreter & Stack Machine

**Fokus:** Virtual machine, execution cycle, dan semua instruksi TAC

### Implementasi
- Struktur Stack Machine — stack frame (static link, dynamic link, return address, variabel)
  - Push/pop frame saat CAL dan RET, akses variabel berdasarkan level dan address
- Siklus Fetch-Decode-Execute dengan Instruction Pointer (IP)
  - Loop utama interpreter: baca instruksi → dispatch ke handler → IP++ (kecuali JMP/JPC)
- Implementasi handler semua instruksi TAC: `LIT`, `LOD`, `STO`, `CAL`, `INT`, `JMP`, `JPC`, `OPR`, `RET`
  - Termasuk semua 14 operasi OPR (aritmatika, perbandingan, WRT, WRTLN)
- Parsing file Intermediate Code sebagai input interpreter
  - Baca output ICG baris per baris, parse menjadi daftar instruksi terstruktur

### Bagian Laporan
- Teori: konsep virtual machine, Stack Machine, siklus eksekusi, LIFO stack frame
- Implementasi: arsitektur interpreter, handler instruksi, contoh eksekusi step-by-step

---

## Anggota 5 — Error Handling, Testing & Pengumpulan

**Fokus:** Robustness, test cases, bonus, dan administrasi pengumpulan

### Implementasi
- Error handling wajib di seluruh komponen
  - Division by zero, undefined variable, invalid jump target — pesan error informatif, tidak crash
- **[Bonus]** Deteksi kerentanan Stack: Overflow, Underflow, Corruption
  - Limit kedalaman stack ~1000 frame, deteksi push/pop yang tidak simetris
- **[Bonus]** Out-of-Bounds Array & Numerical Overflow/Underflow
  - IndexOutOfBoundsException untuk array, OverflowError/UnderflowError untuk integer
- Buat minimal 5 test case unik dengan screenshot I/O
  - Cakup: ekspresi biasa, IF-ELSE, WHILE, fungsi bersarang, edge case error
- Urus GitHub Release tag `v0.4.1` dan form pengumpulan
  - Pastikan laporan ada di `/doc`, invite asisten ke repository

### Bagian Laporan
- Teori: runtime error handling, kerentanan interpreter (bonus)
- Implementasi: error handling & mekanisme deteksi
- Pengujian: dokumentasi 5+ test case dengan screenshot I/O
- Kesimpulan, saran, dan referensi

---

## Catatan Penting

- **Anggota 1 harus menyelesaikan setup antarmuka di hari 1–2 pertama** agar anggota 2, 3, dan 4 bisa langsung mulai secara paralel.
- Anggota 5 berkoordinasi dengan semua anggota untuk menyisipkan error handling ke seluruh komponen.
- Setiap anggota menulis bagian laporan sesuai implementasi yang mereka kerjakan.
