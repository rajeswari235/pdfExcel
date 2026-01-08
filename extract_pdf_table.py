import sys
import os
import pdfplumber

# ---------- ENCODING ----------
os.environ["PYTHONIOENCODING"] = "utf-8"
sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

pdf_path = sys.argv[1]

# ---------- UTILS ----------
def clean(txt):
    return "" if not txt else txt.replace("\r", "").strip()

def split_lines(cell):
     if cell is None:
        return []
     return [l.rstrip() for l in str(cell).splitlines()]

def is_number(txt):
    return txt.replace(".", "", 1).isdigit()


# ---------- CORE ----------
def extract_row_fields(row):
    if len(row) < 6:
        return None

    # ---------- LINE NO ----------
    line_no = clean(row[0])
    if not line_no:
        return None

    # ---------- PART NUMBER ----------
    col1_lines = split_lines(row[1])
    if len(col1_lines) < 2:
        return None
    part_number = col1_lines[1]

    # ---------- QUANTITY (ONLY row[2], FIRST LINE) ----------
    quantity = ""

# scan columns BEFORE price columns only
    candidate_cells = row[:-4]

    # walk from RIGHT to LEFT
    for cell in reversed(candidate_cells):
        lines = split_lines(cell)

        if lines:
            if lines[0]=="0":
               return None
            quantity =lines[0]
                

        if quantity:
            break



      # ---------- UNIT PRICE & TOTAL (BACKWARD SCAN) ----------
    unit_price = ""
    total = ""

    for val in reversed(row):
        txt = clean(val)
        if not txt:
            continue
        if not total:
            total = txt
        elif not unit_price:
            unit_price = txt
            break

    # ---------- DESCRIPTION (ROW-WISE, ORDER SAFE) ----------
   
    description_parts = []

    col1_lines = split_lines(row[1])
    middle_cols = row[2:-2]

# start from 3rd line of col1 (index 2)
    for i in range(2, len(col1_lines)):

    # 1️⃣ base text from column 1
        base = col1_lines[i].strip()
        if base:
            description_parts.append(base)

    # 2️⃣ append same-row text from middle columns
        for cell in middle_cols:
            lines = split_lines(cell)

        # quantity is line 0, so description starts at line 1
            if len(lines) > (i - 1):
               txt = lines[i - 1].strip()
               if txt and not txt.isdigit():
                  description_parts.append(txt)


    description = "".join(description_parts).strip()

    return (
        line_no,
        part_number,
        description,
        quantity,
        unit_price,
        total
    )


# ---------- PDF PROCESS ----------
seen = set()

with pdfplumber.open(pdf_path) as pdf:
    for page in pdf.pages:
        for table in page.extract_tables() or []:
            if not table or len(table) < 2:
                continue

            header = [(c or "").lower() for c in table[0]]
            if not any("line" in h for h in header):
                continue

            for row in table[1:]:
                result = extract_row_fields(row)
                if not result or result in seen:
                    continue

                seen.add(result)

                print(
                    f"{result[0]}|||"
                    f"{result[1]}|||"
                    f"{result[2]}|||"
                    f"{result[3]}|||"
                    f"{result[4]}|||"
                    f"{result[5]}"
                )
