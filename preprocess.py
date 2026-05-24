"""
Препроцессинг датасета Stack Overflow для инвертированного индекса.

Читает Questions.csv, токенизирует текст (title + body),
сохраняет результат в JSONL-файл.

Запуск:
    python preprocess.py --input data/Questions.csv --output data/processed/docs.jsonl
    python preprocess.py --input data/Questions.csv --output data/processed/docs.jsonl --limit 50000
"""
# Сори если не работает я тестил на mock данных :)

import argparse
import csv
import json
import re
import sys
from pathlib import Path


def tokenize(text: str) -> list[str]:
    text = text.lower()
    text = re.sub(r"[^\w\s]", " ", text)
    return [w for w in text.split() if len(w) > 2]


def preprocess(input_path: Path, output_path: Path, limit: int | None) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    count = 0
    
    # Пробуем разные кодировки
    encodings = ['utf-8', 'latin-1', 'cp1251', 'iso-8859-1', 'windows-1252']
    reader = None
    f = None
    
    for enc in encodings:
        try:
            f = open(input_path, encoding=enc, errors='ignore')
            reader = csv.DictReader(f)
            # Пробуем прочитать первую строку
            next(reader)
            f.seek(0)
            reader = csv.DictReader(f)
            print(f"Использую кодировку: {enc}", file=sys.stderr)
            break
        except (UnicodeDecodeError, StopIteration):
            if f:
                f.close()
            continue
    
    if not reader:
        print("Ошибка: не удалось прочитать файл ни в одной кодировке", file=sys.stderr)
        return
    
    with open(output_path, "w", encoding="utf-8") as out:
        for row in reader:
            if limit is not None and count >= limit:
                break

            doc_id = row.get("Id", "")
            title = row.get("Title", "")
            body = row.get("Body", "")

            tokens = tokenize(title + " " + body)
            if not tokens:
                continue

            out.write(json.dumps({"doc_id": doc_id, "title": title, "tokens": tokens}) + "\n")
            count += 1

            if count % 10_000 == 0:
                print(f"Обработано: {count} документов", file=sys.stderr)
    
    f.close()
    print(f"Готово. Всего документов: {count}", file=sys.stderr)
    print(f"Результат: {output_path}", file=sys.stderr)


def main() -> None:
    parser = argparse.ArgumentParser(description="Препроцессинг Stack Overflow датасета")
    parser.add_argument("--input", required=True, type=Path, help="Путь к Questions.csv")
    parser.add_argument("--output", required=True, type=Path, help="Путь к выходному JSONL-файлу")
    parser.add_argument("--limit", type=int, default=None, help="Максимальное количество документов")
    args = parser.parse_args()

    if not args.input.exists():
        print(f"Ошибка: файл не найден: {args.input}", file=sys.stderr)
        sys.exit(1)

    preprocess(args.input, args.output, args.limit)


if __name__ == "__main__":
    main()
