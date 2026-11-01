#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char compile_cmd[512];
    snprintf(compile_cmd, sizeof(compile_cmd), "gcc -Wall -pedantic %s -o /app/temp/compiled_prog 2> /app/temp/compile_err.txt", solution);
    int compile_res = system(compile_cmd);

    if (compile_res != 0) {
        printf("--- CHYBA KOMPILACE ---\n");
        print_file_content("/app/temp/compile_err.txt");
        return 0;
    }

    int total_basic = 0, passed_basic = 0;
    int total_bonus = 0, passed_bonus = 0;

    printf("=== ZÁKLADNÍ TESTY ===\n");

    // 2. FÁZE: Pouze základní testy (00 až 09)
    for (int i = 0; i < 10; i++) {
        char in_path[256], out_path[256], my_out_path[256];
        snprintf(in_path, sizeof(in_path), "%s/%02d_in.txt", tests_dir, i);
        snprintf(out_path, sizeof(out_path), "%s/%02d_out.txt", tests_dir, i);
        snprintf(my_out_path, sizeof(my_out_path), "/app/temp/my_out_%02d.txt", i);

        FILE *f_in = fopen(in_path, "r");
        if (!f_in) continue; // test neexistuje
        fclose(f_in);

        total_basic++;

        // Spuštění studentova programu
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "/app/temp/compiled_prog < %s > %s 2>&1", in_path, my_out_path);
        system(cmd);

        // Porovnání výstupů
        snprintf(cmd, sizeof(cmd), "diff -b -B %s %s > /dev/null", out_path, my_out_path);
        int diff_res = system(cmd);

        if (diff_res == 0) {
            passed_basic++;
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

    if (total_basic == 0) {
        printf("Zadne zakladni testy nebyly nalezeny ve slozce: %s\n", tests_dir);
        return 0;
    }

    int percentage = (passed_basic * 100) / total_basic;
    printf("\n========================================\n");
    printf("Základní testy: %d%% (%d/%d testů)\n", percentage, passed_basic, total_basic);

    // 3. FÁZE: Bonusy spustíme JEN POKUD má základ na 100 %
    if (percentage == 100) {
        printf("\n=== BONUSOVÉ TESTY ===\n");

        for (int i = 10; i < 100; i++) {
            char in_path[256], out_path[256], my_out_path[256];
            snprintf(in_path, sizeof(in_path), "%s/%02d_in.txt", tests_dir, i);
            snprintf(out_path, sizeof(out_path), "%s/%02d_out.txt", tests_dir, i);
            snprintf(my_out_path, sizeof(my_out_path), "/app/temp/my_out_%02d.txt", i);

            FILE *f_in = fopen(in_path, "r");
            if (!f_in) continue; // bonus neexistuje
            fclose(f_in);

            total_bonus++;

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "/app/temp/compiled_prog < %s > %s 2>&1", in_path, my_out_path);
            system(cmd);

            snprintf(cmd, sizeof(cmd), "diff -b -B %s %s > /dev/null", out_path, my_out_path);
            int diff_res = system(cmd);

            if (diff_res == 0) {
                passed_bonus++;
                printf("[OK] Test %02d (BONUS): SUCCESS\n", i);
            } else {
                printf("\n----------------------------------------\n");
                printf("[FAIL] Test %02d (BONUS): FAILED\n", i);
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

        if (total_bonus > 0) {
            printf("\nBonusové testy: %d/%d splněno\n", passed_bonus, total_bonus);
            if (passed_bonus == total_bonus) {
                printf("🏆 PERFEKTNÍ! Získáváš plný počet včetně všech bonusů!\n");
            }
        }
    } else {
        printf("⚠️ Pro zpřístupnění bonusových testů musíte nejprve splnit základní testy na 100%%.\n");
    }

    printf("========================================\n");

    return 0;
}