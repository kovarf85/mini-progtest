#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>

#define TIMEOUT_SECONDS 10
#define MAX_TESTS 10

static pid_t child_pid = -1;

void handle_timeout(int sig) {
    (void)sig;
    if (child_pid > 0) {
        kill(child_pid, SIGKILL);
    }
}

typedef enum {
    RESULT_OK = 0,
    RESULT_WA,   // Wrong Answer
    RESULT_TLE,  // Time Limit Exceeded
    RESULT_RTE   // Runtime Error
} TestStatus;

// 1. Kompilace studentova kódu
int compile_student_code(const char *source_file, const char *binary_out) {
    pid_t pid = fork();
    if (pid == 0) {
        // Přesměrujeme chybový výstup g++ do souboru, abychom mohli ukázat chybu kompilace
        int err_fd = open("compile_error.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (err_fd >= 0) {
            dup2(err_fd, STDERR_FILENO);
            close(err_fd);
        }
        execlp("gcc", "gcc", "-Wall", source_file, "-o", binary_out, NULL);
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

// 2. Spuštění jednoho testu s časovým limitem
TestStatus run_single_test(const char *binary, const char *in_file, const char *user_out_file) {
    child_pid = fork();

    if (child_pid == 0) {
        // Přesměrování stdin
        int in_fd = open(in_file, O_RDONLY);
        if (in_fd < 0) exit(1);
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);

        // Přesměrování stdout
        int out_fd = open(user_out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) exit(1);
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);

        execl(binary, binary, NULL);
        exit(1);
    }

    // Nastavení timeoutu
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_timeout;
    sigaction(SIGALRM, &sa, NULL);

    alarm(TIMEOUT_SECONDS);
    int status;
    waitpid(child_pid, &status, 0);
    alarm(0); // Zrušení budíku

    if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGKILL) {
            return RESULT_TLE;
        }
        return RESULT_RTE;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return RESULT_OK;
    }

    return RESULT_RTE;
}

// 3. Porovnání výstupu studenta s očekávaným výstupem
int compare_outputs(const char *user_out, const char *expected_out) {
    pid_t pid = fork();
    if (pid == 0) {
        int dev_null = open("/dev/null", O_WRONLY);
        dup2(dev_null, STDOUT_FILENO);
        close(dev_null);

        // Porovnání pomocí 'diff -w' (ignoruje bílé znaky na konci řádků)
        execlp("diff", "diff", "-w", user_out, expected_out, NULL);
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

int main(int argc, char *argv[]) {
    const char *source = (argc > 1) ? argv[1] : "solution.c";
    const char *tests_dir = (argc > 2) ? argv[2] : "."; // Výchozí je aktuální složka
    const char *binary = "./student_bin";

    printf("\n==========================================\n");
    printf("         MINI PROGTEST EVALUATOR          \n");
    printf("==========================================\n");
    printf("Hodnocený soubor: %s\n", source);
    printf("Složka s testy:   %s\n\n", tests_dir);

    // 1. Kompilace
    printf("[1] Kompilace kódu... ");
    fflush(stdout);
    if (!compile_student_code(source, binary)) {
        printf("COMPILATION ERROR!\n");
        system("cat compile_error.log");
        return 1;
    }
    printf("OK\n\n");

    // 2. Procházení a spouštění testů
    printf("[2] Testování:\n");
    int passed_count = 0;
    int total_tests = 0;

    for (int i = 0; i < MAX_TESTS; i++) {
        char in_file[256], exp_out_file[256], user_out_file[256];
        
        snprintf(in_file, sizeof(in_file), "%s/%02d_in.txt", tests_dir, i);
        snprintf(exp_out_file, sizeof(exp_out_file), "%s/%02d_out.txt", tests_dir, i);
        snprintf(user_out_file, sizeof(user_out_file), "user_%02d_out.txt", i);

        // Zkontrolujeme, zda test s tímto číslem existuje
        if (access(in_file, F_OK) != 0 || access(exp_out_file, F_OK) != 0) {
            break; // Žádný další test ve složce není
        }

        total_tests++;
        printf("  Test %02d: ", i);
        fflush(stdout);

        TestStatus status = run_single_test(binary, in_file, user_out_file);

        if (status == RESULT_OK) {
            if (compare_outputs(user_out_file, exp_out_file)) {
                printf("SUCCESS (OK)\n");
                passed_count++;
            } else {
                printf("WRONG ANSWER (Chybný výstup)\n");
            }
        } else if (status == RESULT_TLE) {
            printf("TIME LIMIT EXCEEDED (>%ds)\n", TIMEOUT_SECONDS);
        } else {
            printf("RUNTIME ERROR (Program spadl)\n");
        }

        unlink(user_out_file);
    }

    if (total_tests == 0) {
        printf("  Nebyly nalezeny žádné testovací soubory (%s/00_in.txt)!\n", tests_dir);
    } else {
        printf("\n==========================================\n");
        printf("VÝSLEDEK: %d / %d testů prošlo (%.0f%%)\n", 
               passed_count, total_tests, (float)passed_count / total_tests * 100);
        printf("==========================================\n\n");
    }

    unlink(binary);
    unlink("compile_error.log");

    return 0;
}