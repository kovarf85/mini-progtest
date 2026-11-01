#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void print_file_content(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("(soubor nenalezen)\n");
        return;
    }
    char ch;
    while ((ch = fgetc(f)) != EOF) {
        putchar(ch);
    }
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Pouziti: ./tester <c_soubor> <složka_s_testy>\n");
        return 1;
    }

    const char *solution = argv[1];
    const char *tests_dir = argv[2];

    // 1. Kompilace C programu
    int compile_res = system("gcc -Wall -pedantic /app/temp/solution.c -o /app/temp/compiled_prog 2> /app/temp/compile_err.txt");
    if (compile_res != 0) {
        printf("--- CHYBA KOMPILACE ---\n");
        print_file_content("/app/temp/compile_err.txt");
        return 0;
    }

    // 2. Procházení testovacích souborů
    int total_tests = 0;
    int passed_tests = 0;

    for (int i = 0; i < 100; i++) {
        char in_path[256], out_path[256], my_out_path[256];
        snprintf(in_path, sizeof(in_path), "%s/%02d_in.txt", tests_dir, i);
        snprintf(out_path, sizeof(out_path), "%s/%02d_out.txt", tests_dir, i);
        snprintf(my_out_path, sizeof(my_out_path), "/app/temp/my_out_%02d.txt", i);

        FILE *f_in = fopen(in_path, "r");
        if (!f_in) continue; // test neexistuje, přeskočíme
        fclose(f_in);

        total_tests++;

        // Spuštění studentova programu s daným vstupem
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "/app/temp/compiled_prog < %s > %s 2>&1", in_path, my_out_path);
        system(cmd);

        // Porovnání výstupů pomocí 'diff'
        snprintf(cmd, sizeof(cmd), "diff -b -B %s %s > /dev/null", out_path, my_out_path);
        int diff_res = system(cmd);

        if (diff_res == 0) {
            passed_tests++;
            printf("[OK] Test %02d: SUCCESS\n", i);
        } else {
            printf("\n----------------------------------------\n");
            printf("[FAIL] Test %02d: FAILED\n", i);
            printf("----------------------------------------\n");
            
            printf(">>> VSTUP (in.txt):\n");
            print_file_content(in_path);
            
            printf("\n>>> OČEKÁVANÝ VÝSTUP (out.txt):\n");
            print_file_content(out_path);
            
            printf("\n>>> TVŮJ VÝSTUP:\n");
            print_file_content(my_out_path);
            printf("----------------------------------------\n\n");
        }
    }

    if (total_tests == 0) {
        printf("Zadne testy nebyly nalezeny ve slozce: %s\n", tests_dir);
        return 0;
    }

    int percentage = (passed_tests * 100) / total_tests;
    printf("\nVysledek: %d%% (%d/%d testu proslo)\n", percentage, passed_tests, total_tests);

    return 0;
}