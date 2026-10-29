import os
import json
import subprocess
from flask import Flask, render_template, request, redirect, url_for, session

app = Flask(__name__)
app.secret_key = "mini_progtest_tajny_klic_123"

RESULTS_FILE = "results.json"

ALLOWED_USERS = ["Kovy":"8545", "Rajchy": "4585"]

# DEFINICE ÚLOH S BODOVÝM OHODNOCENÍM
TASKS = {
    "01_scitani": {
        "name": "Úloha 1: Sčítání dvou čísel", 
        "desc": "Napište program, který načte dvě celá čísla ze stdin a vytiskne jejich součet.",
        "points": 3,
        "sample_in": "5 10",
        "sample_out": "15"
    }
}

def load_results():
    if os.path.exists(RESULTS_FILE):
        with open(RESULTS_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}

def save_results(results):
    with open(RESULTS_FILE, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=4, ensure_ascii=False)

@app.route("/login", methods=["GET", "POST"])
def login():
    error = None
    if request.method == "POST":
        username = request.form.get("username", "").strip()
        password = request.form.get("password", "").strip()

        # Kontrola, zda uživatel existuje a heslo souhlasí
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

    results = load_results()
    result_output = None

    if request.method == "POST":
        selected_task = request.form.get("task", selected_task)
        file = request.files.get("file_upload")

        if file and file.filename != "":
            user_code = file.read().decode("utf-8", errors="ignore")

            # Automatické vytvoření složky temp
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

                if username not in results:
                    results[username] = {}
                
                results[username][selected_task] = results[username].get(selected_task, False) or is_passed
                save_results(results)

            except Exception as e:
                result_output = f"Chyba při spouštění testeru: {str(e)}"
        else:
            result_output = "Chyba: Nebyl vybrán žádný soubor k odevzdání!"

    return render_template(
        "index.html",
        tasks=TASKS,
        selected_task=selected_task,
        username=username,
        result_output=result_output,
        user_results=results.get(username, {})
    )
# NOVÁ RUTA PRO LEADERBOARD (ŽEBŘÍČEK)
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

    # Seřazení uživatelů podle celkových bodů sestupně
    leaderboard_data.sort(key=lambda x: x["total_points"], reverse=True)

    return render_template(
        "leaderboard.html",
        leaderboard=leaderboard_data,
        tasks=TASKS,
        username=session["username"]
    )

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)