import os
import json
import subprocess
import psycopg2
from flask import Flask, render_template, request, redirect, url_for, session

app = Flask(__name__)
app.secret_key = "mini_progtest_tajny_klic_123"

DATABASE_URL = os.environ.get("DATABASE_URL")
RESULTS_FILE = "results.json"
HINTS_FILE = "hints.json"

USERS = {
    "Rajchy": "4585",
    "Kovy": "8545",
    "Cernoch6767": "6767"
}

TASKS = {
    "01_scitani": {
        "name": "Úloha 1: Sčítání dvou čísel", 
        "desc": "Napište program, který vyzve k zadání dvoch celých čísel ze stdin a vytiskne jejich součet.",
        "points": 3,
        "sample_combined": "<b>Zadejte dve cisla:</b>\n5 10\n<b>Soucet: 15</b>"
    },
    "02_serazeni_pole": {
        "name": "Úloha 2: Seřazení čísel", 
        "desc": "Napište program, který načte počet čísel N, následně N celých čísel a vytiskne je seřazená vzestupně.\nOšetřete nesprávné vstupy. Předávejte je na standartní výstup (Ne chybový). Nezapomeňte na bílé znaky na konci výpisů.",
        "points": 7,
        "sample_combined": "<b>Zadejte delku:</b>\n3\n<b>Zadejte cisla:</b>\n15 3 8\n<b>Serazena posloupnost:\n3 8 15</b>"
    }
}

def init_db():
    if DATABASE_URL:
        try:
            conn = psycopg2.connect(DATABASE_URL)
            cur = conn.cursor()
            cur.execute("""
                CREATE TABLE IF NOT EXISTS results (
                    username VARCHAR(50),
                    task_id VARCHAR(50),
                    is_passed BOOLEAN,
                    PRIMARY KEY (username, task_id)
                );
            """)
            cur.execute("""
                CREATE TABLE IF NOT EXISTS used_hints (
                    username VARCHAR(50),
                    task_id VARCHAR(50),
                    test_name VARCHAR(50),
                    PRIMARY KEY (username, task_id)
                );
            """)
            conn.commit()
            cur.close()
            conn.close()
        except Exception as e:
            print("Chyba při inicializaci DB:", e)

# Vytvoření tabulek při startu aplikace
init_db()

def load_results():
    if DATABASE_URL:
        try:
            conn = psycopg2.connect(DATABASE_URL)
            cur = conn.cursor()
            cur.execute("SELECT username, task_id, is_passed FROM results WHERE is_passed = TRUE;")
            rows = cur.fetchall()
            cur.close()
            conn.close()

            results = {}
            for row in rows:
                user, task, passed = row
                if user not in results:
                    results[user] = {}
                results[user][task] = passed
            return results
        except Exception as e:
            print("DB Load Error:", e)

    # Lokální záloha na JSON soubor (pro běh na PC)
    if os.path.exists(RESULTS_FILE):
        with open(RESULTS_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}

def save_result_for_user(username, task_id, is_passed):
    if not is_passed:
        return

    if DATABASE_URL:
        try:
            conn = psycopg2.connect(DATABASE_URL)
            cur = conn.cursor()
            cur.execute("""
                INSERT INTO results (username, task_id, is_passed)
                VALUES (%s, %s, %s)
                ON CONFLICT (username, task_id) DO UPDATE SET is_passed = EXCLUDED.is_passed;
            """, (username, task_id, True))
            conn.commit()
            cur.close()
            conn.close()
            return
        except Exception as e:
            print("DB Save Error:", e)

    # Lokální zápis do JSONu při vývoji na vlastním počítači
    results = load_results()
    if username not in results:
        results[username] = {}
    results[username][task_id] = True
    with open(RESULTS_FILE, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=4, ensure_ascii=False)

def get_used_hint(username, task_id):
    if DATABASE_URL:
        try:
            conn = psycopg2.connect(DATABASE_URL)
            cur = conn.cursor()
            cur.execute("SELECT test_name FROM used_hints WHERE username = %s AND task_id = %s;", (username, task_id))
            row = cur.fetchone()
            cur.close()
            conn.close()
            return row[0] if row else None
        except Exception as e:
            print("DB Hint Read Error:", e)

    if os.path.exists(HINTS_FILE):
        with open(HINTS_FILE, "r", encoding="utf-8") as f:
            hints = json.load(f)
            return hints.get(username, {}).get(task_id)
    return None

def save_used_hint(username, task_id, test_name):
    if DATABASE_URL:
        try:
            conn = psycopg2.connect(DATABASE_URL)
            cur = conn.cursor()
            cur.execute("""
                INSERT INTO used_hints (username, task_id, test_name)
                VALUES (%s, %s, %s)
                ON CONFLICT (username, task_id) DO NOTHING;
            """, (username, task_id, test_name))
            conn.commit()
            cur.close()
            conn.close()
            return
        except Exception as e:
            print("DB Hint Save Error:", e)

    hints = {}
    if os.path.exists(HINTS_FILE):
        with open(HINTS_FILE, "r", encoding="utf-8") as f:
            hints = json.load(f)
    if username not in hints:
        hints[username] = {}
    hints[username][task_id] = test_name
    with open(HINTS_FILE, "w", encoding="utf-8") as f:
        json.dump(hints, f, indent=4, ensure_ascii=False)

def get_available_tests(task_id):
    tests_dir = os.path.join("ulohy", task_id)
    if not os.path.exists(tests_dir):
        return []
    files = os.listdir(tests_dir)
    tests = set()
    for f in files:
        if f.endswith("_in.txt"):
            tests.add(f[:-7])
    return sorted(list(tests))

@app.route("/login", methods=["GET", "POST"])
def login():
    error = None
    if request.method == "POST":
        username = request.form.get("username", "").strip()
        password = request.form.get("password", "").strip()

        if username in USERS and USERS[username] == password:
            session["username"] = username
            return redirect(url_for("index"))
        else:
            error = "Neplatné uživatelské jméno nebo heslo!"

    return render_template("login.html", error=error)

@app.route("/logout")
def logout():
    session.pop("username", None)
    return redirect(url_for("login"))

@app.route("/", methods=["GET", "POST"])
def index():
    if "username" not in session:
        return redirect(url_for("login"))

    username = session["username"]
    selected_task = request.args.get("task", "01_scitani")
    
    if selected_task not in TASKS:
        selected_task = "01_scitani"

    result_output = None

    if request.method == "POST":
        action = request.form.get("action", "upload")
        selected_task = request.form.get("task", selected_task)

        if action == "upload":
            file = request.files.get("file_upload")
            if file and file.filename != "":
                user_code = file.read().decode("utf-8", errors="ignore")
                os.makedirs("temp", exist_ok=True)

                temp_code_path = os.path.join("temp", "solution.c")
                with open(temp_code_path, "w", encoding="utf-8") as f:
                    f.write(user_code)

                tests_dir = os.path.join("ulohy", selected_task)

                try:
                    process = subprocess.run(
                        ["./tester", temp_code_path, tests_dir],
                        capture_output=True,
                        text=True,
                        timeout=15
                    )
                    result_output = process.stdout
                    is_passed = "100%" in result_output

                    if is_passed:
                        save_result_for_user(username, selected_task, True)

                except Exception as e:
                    result_output = f"Chyba při spouštění testeru: {str(e)}"
            else:
                result_output = "Chyba: Nebyl vybrán žádný soubor k odevzdání!"

        elif action == "use_hint":
            selected_test = request.form.get("selected_test")
            already_used = get_used_hint(username, selected_task)
            if not already_used and selected_test:
                save_used_hint(username, selected_task, selected_test)

    user_results = load_results().get(username, {})
    used_hint_test = get_used_hint(username, selected_task)
    hint_data = None

    if used_hint_test:
        in_path = os.path.join("ulohy", selected_task, f"{used_hint_test}_in.txt")
        out_path = os.path.join("ulohy", selected_task, f"{used_hint_test}_out.txt")
        
        in_content = open(in_path, encoding="utf-8").read() if os.path.exists(in_path) else "Soubor nenalezen"
        out_content = open(out_path, encoding="utf-8").read() if os.path.exists(out_path) else "Soubor nenalezen"

        hint_data = {
            "test_name": used_hint_test,
            "in": in_content,
            "out": out_content
        }

    available_tests = get_available_tests(selected_task)

    return render_template(
        "index.html",
        tasks=TASKS,
        selected_task=selected_task,
        username=username,
        result_output=result_output,
        user_results=user_results,
        available_tests=available_tests,
        used_hint_test=used_hint_test,
        hint_data=hint_data
    )

@app.route("/leaderboard")
def leaderboard():
    if "username" not in session:
        return redirect(url_for("login"))

    results = load_results()
    leaderboard_data = []

    for user in USERS.keys():
        user_task_results = results.get(user, {})
        total_points = 0
        task_points = {}

        for task_id, task_info in TASKS.items():
            if user_task_results.get(task_id, False):
                pts = task_info["points"]
                task_points[task_id] = pts
                total_points += pts
            else:
                task_points[task_id] = 0

        leaderboard_data.append({
            "username": user,
            "task_points": task_points,
            "total_points": total_points
        })

    leaderboard_data.sort(key=lambda x: x["total_points"], reverse=True)

    return render_template(
        "leaderboard.html",
        leaderboard=leaderboard_data,
        tasks=TASKS,
        username=session["username"]
    )

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)