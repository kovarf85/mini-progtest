FROM python:3.10-slim-buster

# Instalace překladače GCC a systémových nástrojů
RUN apt-get update && apt-get install -y gcc build-essential diffutils

WORKDIR /app

# Kopírování všech souborů projektu
COPY . /app

# Instalace Flasku
RUN pip install --no-cache-dir flask

# Zkompilování C testeru přímo v kontejneru
RUN gcc tester.c -o tester

EXPOSE 5000

# Spuštění aplikace
CMD ["python3", "app.py"]