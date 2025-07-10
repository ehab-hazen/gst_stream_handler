#!/usr/bin/env python

import sys
import re
import textwrap
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

# === Parse args ===
if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} <metrics_csv_file> <log_file>")
    sys.exit(1)
csv_path = sys.argv[1]

log_file_timestamp = re.search(r'log(\d+-\d+\.\d+)', sys.argv[2]).group(1).replace('.', '-')
pdf_filename = f'resource_metrics_report{log_file_timestamp}.pdf'

# === Load CSV ===
df = pd.read_csv(csv_path)
df['timestamp'] = df['timestamp_ms'] / 1000  # seconds

# === Identify columns ===
core_cols   = [c for c in df.columns if c.startswith('cpu') and c.endswith('_usage')]
gpu_indices = sorted({int(c.split('_')[0][3:]) for c in df.columns if c.startswith('gpu')})

# === Create PDF ===
print(f"Generating {pdf_filename}...")
with PdfPages(pdf_filename) as pdf:

    # — CPU cores usage —
    fig = plt.figure(figsize=(10, 4))
    for col in core_cols:
        plt.plot(df['timestamp'], df[col], label=col)
    plt.xlabel('Time (s)')
    plt.ylabel('CPU core usage (%)')
    plt.title('CPU Usage per Core')
    plt.legend(loc='upper right', ncol=2, fontsize='small')
    plt.tight_layout()
    pdf.savefig(fig)
    plt.close(fig)

    # — Memory usage —
    fig = plt.figure(figsize=(6, 3))
    plt.plot(df['timestamp'], df['ram_kib'] / 1024)
    plt.xlabel('Time (s)')
    plt.ylabel('RAM Usage (MiB)')
    plt.title('RAM Usage Over Time')
    plt.tight_layout()
    pdf.savefig(fig)
    plt.close(fig)

    # — GPU graphs —
    for g in gpu_indices:
        t      = df['timestamp']
        prefix = f'gpu{g}_'

        # 1) GPU utilization (sm compute, vram, enc/dec util)
        fig = plt.figure(figsize=(8, 4))
        for suffix, label in [('util', 'GPU Util'),
                              ('mem', 'Mem Util'),
                              ('enc_util', 'Enc Util'),
                              ('dec_util', 'Dec Util')]:
            plt.plot(t, df[prefix + suffix], label=label)
        plt.xlabel('Time (s)')
        plt.ylabel('Utilization (%)')
        plt.title(f'GPU {g} SM compute, VRAM & Encoder/Decoder Utilization')
        plt.legend()
        plt.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        # 3) All clocks
        fig = plt.figure(figsize=(8, 4))
        for suffix in ['gpu_clock','mem_clock','sm_clock','vid_clock']:
            plt.plot(t, df[prefix + suffix], label=suffix)
        plt.xlabel('Time (s)')
        plt.ylabel('Clock (MHz)')
        plt.title(f'GPU {g} Clock Rates')
        plt.legend()
        plt.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        # 4) GPU temperature (separate)
        fig = plt.figure(figsize=(6, 3))
        plt.plot(t, df[prefix + 'temp'])
        plt.xlabel('Time (s)')
        plt.ylabel('Temperature (°C)')
        plt.title(f'GPU {g} Temperature')
        plt.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        # 5) GPU power draw (separate)
        fig = plt.figure(figsize=(6, 3))
        plt.plot(t, df[prefix + 'power'] / 1000)
        plt.xlabel('Time (s)')
        plt.ylabel('Power (W)')
        plt.title(f'GPU {g} Power Consumption')
        plt.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

    # 6) write logs
    with open(sys.argv[2], 'r') as f:
        text = f.read().replace('\t', '    ')

    lines = text.splitlines()
    wrapped_lines = []
    for line in lines:
        wrapped_lines.extend(textwrap.wrap(line, width=130))  # wrap long lines

    lines_per_page = 85
    for i in range(0, len(wrapped_lines), lines_per_page):
        fig = plt.figure(figsize=(8.5, 11))
        plt.axis('off')
        page_text = '\n'.join(wrapped_lines[i:i + lines_per_page])
        plt.text(0, 1, page_text, ha='left', va='top', wrap=True, fontsize=7)
        pdf.savefig(fig)
        plt.close(fig)

print("Done")
