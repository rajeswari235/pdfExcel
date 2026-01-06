import sys, os
os.environ["PYTHONIOENCODING"] = "utf-8"
sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")


import pdfplumber
import re

pdf_path = sys.argv[1]

def extract_invoice_header(page):
    text = page.extract_text() or ""

    invoice_no = ""
    invoice_date = ""
    master_track = ""

    for line in text.splitlines():
        line = line.strip()

        if "invoice no" in line.lower():
            invoice_no = line

        elif "invoice date" in line.lower():
            invoice_date = line

        elif "master track" in line.lower():
            master_track = line

    return master_track,invoice_no, invoice_date


def extract_mouser_and_desc(cell_text):
    mouser = ""
    desc = ""

    for line in cell_text.splitlines():
        line = line.strip()

        m = re.search(r"Mouser\s+Number\s+(.+)", line, re.IGNORECASE)
        if m:
            mouser = m.group(1).strip()

        d = re.search(r"Desc\.?\s+(.+)", line, re.IGNORECASE)
        if d:
            desc = d.group(1).strip()

    return mouser, desc

seen_rows = set()
header_printed = False

with pdfplumber.open(pdf_path) as pdf:
    for page in pdf.pages:
        if not header_printed:
           master_track,invoice_no, invoice_date = extract_invoice_header(page)

           print(f"HEADER|||{master_track}|||{invoice_no}|||{invoice_date}")
           header_printed = True

        tables = page.extract_tables()
        def normalize(text):
          return (
            text.replace("\xa0", " ")
            .replace("\n", " ")
            .strip()
            .lower()
    )
        


        for table in tables:
            for row in table:
                if not row or len(row) < 8:
                    continue
                first_col = normalize(row[0] or "")

        # 🚫 HEADER DETECTION
                if any(h in first_col for h in ("line no", "product detail", "customer part no")):
                    continue

               
                

                net_no = (row[0] or "").strip()


                # 🔑 SOURCE NET HAS STRUCTURED DATA
                Part_number, desc = extract_mouser_and_desc(row[1] or "")

                part_no= (row[2] or "").strip()
                order_qty=(row[3] or "").strip()
                Price_INR=(row[6] or "").strip()
                fail_net = (row[7] or "").strip()


                row_key = (
                    net_no,
                    Part_number,
                    desc,
                    part_no,
                    order_qty,
                    Price_INR,
                    fail_net
                )

                if row_key in seen_rows:
                    continue

                seen_rows.add(row_key)

                

                # Print only what you NEED
                print(
                    f"{net_no}|||"
                    f"{Part_number}|||{desc}|||"
                    f"{part_no}|||{order_qty}|||{Price_INR}|||"
                    f"{fail_net}|||"
                )
