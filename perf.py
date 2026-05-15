#!/usr/bin/env python3
import subprocess
import time
import sys
import os
import statistics
import matplotlib.pyplot as plt
import numpy as np
from itertools import product

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(PROJECT_ROOT, "build")
DATA_IMAGE_DIR = os.path.join(PROJECT_ROOT, "data/image")
DATA_CONV_DIR = os.path.join(PROJECT_ROOT, "data/convolution")
OUTPUT_DIR = os.path.join(PROJECT_ROOT, "test")
IDIFF = "idiff"

IMAGES = [
    "lake.bmp", "city.bmp", "coast.bmp", "earth.bmp",
    "small_sample.bmp", "medium_sample.bmp", "big_sample.bmp", "motorcycle.bmp"
]
CONVOLUTIONS = [
    "3x3/id.conv", "3x3/gaussian_like.conv", "3x3/3d.conv",
    "5x5/id.conv", "5x5/some.conv"
]
IMPLEMENTATIONS = ["seq", "row", "col", "pixel", "tiles"]
REPEATS = 5

os.makedirs(OUTPUT_DIR, exist_ok=True)

def get_exe_path(prefix, version):
    return os.path.join(BUILD_DIR, f"{prefix}_{version}")

def check_exe(path):
    return os.path.isfile(path) and os.access(path, os.X_OK)

def run_cmd(cmd, timeout=None):
    proc = subprocess.run(cmd, capture_output=True, timeout=timeout)
    return proc.returncode, proc.stdout, proc.stderr

def validate_pipeline(seq_exe, pipe_exe, tasks):
    errors = []
    for img, conv in tasks:
        img_path = os.path.join(DATA_IMAGE_DIR, img)
        conv_path = os.path.join(DATA_CONV_DIR, conv)
        base_img = os.path.splitext(img)[0]
        conv_safe = conv.replace('/', '_')
        ref_out = os.path.join(OUTPUT_DIR, f"ref_{base_img}_{conv_safe}.bmp")
        pipe_out = os.path.join(OUTPUT_DIR, f"pipe_{base_img}_{conv_safe}.bmp")

        ret, _, err = run_cmd([seq_exe, img_path, conv_path, ref_out])
        if ret != 0 or not os.path.isfile(ref_out):
            errors.append(f"{img}+{conv}: seq failed (code {ret})\n{err.decode()}")
            if os.path.exists(ref_out):
                os.remove(ref_out)
            continue

        ret, _, err = run_cmd([pipe_exe, img_path, conv_path, pipe_out])
        if ret != 0 or not os.path.isfile(pipe_out):
            errors.append(f"{img}+{conv}: pipe failed (code {ret})\n{err.decode()}")
            if os.path.exists(ref_out):
                os.remove(ref_out)
            if os.path.exists(pipe_out):
                os.remove(pipe_out)
            continue

        ret, out, err = run_cmd([IDIFF, ref_out, pipe_out])
        if ret != 0:
            errors.append(f"{img}+{conv}: mismatch\n{out.decode()}{err.decode()}")

        for f in (ref_out, pipe_out):
            if os.path.exists(f):
                os.remove(f)
    return errors

def measure_batch(exe_path, tasks, repeats=REPEATS):
    args = [exe_path]
    tmp_files = []
    for img, conv in tasks:
        img_path = os.path.join(DATA_IMAGE_DIR, img)
        conv_path = os.path.join(DATA_CONV_DIR, conv)
        base_img = os.path.splitext(img)[0]
        conv_safe = conv.replace('/', '_')
        out_tmp = os.path.join(OUTPUT_DIR,
                               f"tmp_{os.path.basename(exe_path)}_{base_img}_{conv_safe}.bmp")
        args.extend([img_path, conv_path, out_tmp])
        tmp_files.append(out_tmp)

    times = []
    for _ in range(repeats):
        try:
            start = time.perf_counter()
            ret, _, err = run_cmd(args)
            elapsed = time.perf_counter() - start
            if ret != 0:
                print(f"error {exe_path}: code {ret}", file=sys.stderr)
                if err:
                    print(err.decode(), file=sys.stderr)
                break
            if err:
                print(f"stderr {exe_path}: {err.decode()}", file=sys.stderr)
            times.append(elapsed)
        except Exception as e:
            print(f"exception {exe_path}: {e}", file=sys.stderr)
            break
    for f in tmp_files:
        if os.path.exists(f):
            os.remove(f)

    if len(times) == repeats:
        return statistics.mean(times)
    else:
        return float('inf')

def plot_batch_results(results, tasks, batch_size):
    labels = [v for v, _, _ in available_versions]
    seq_vals = [results.get(('seq', v), float('inf')) for v in labels]
    pipe_vals = [results.get(('pipeline', v), float('inf')) for v in labels]

    x = np.arange(len(labels))
    width = 0.35

    fig, ax = plt.subplots(figsize=(12, 7))
    rects1 = ax.bar(x - width/2, seq_vals, width, label='seq', color='skyblue')
    rects2 = ax.bar(x + width/2, pipe_vals, width, label='pipeline', color='salmon')

    ax.set_ylabel('Среднее время (с)')
    ax.set_title(f'{batch_size} троек (среднее {REPEATS} запусков)')
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()

    max_val = max([v for v in seq_vals + pipe_vals if not np.isinf(v)] or [0])
    if max_val == 0:
        ax.set_ylim(0, 1)
    else:
        ax.set_ylim(0, max_val * 1.15)

    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            if np.isinf(height):
                ax.annotate('N/A', (rect.get_x() + rect.get_width()/2, ax.get_ylim()[1]*0.9),
                            ha='center', va='bottom', fontsize=8, color='red')
            else:
                ax.annotate(f'{height:.2f}', (rect.get_x() + rect.get_width()/2, height),
                            ha='center', va='bottom', fontsize=8)

    autolabel(rects1)
    autolabel(rects2)

    task_descriptions = [f"{img} + {conv}" for img, conv in tasks]
    task_text = '\n'.join(task_descriptions)
    fig.text(0.0, 0.01, task_text, ha='left', va='bottom', fontsize=6, style='italic')
    fig.subplots_adjust(bottom=0.02 + 0.015 * len(tasks))

    filename = f'benchmark_{batch_size}tasks.png'
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    plt.close(fig)

try:
    ret, _, _ = run_cmd([IDIFF, "--help"])
    if ret != 0:
        raise FileNotFoundError()
except Exception:
    print("idiff not found", file=sys.stderr)
    sys.exit(1)

available_versions = []
for ver in IMPLEMENTATIONS:
    seq_exe = get_exe_path("seq", ver)
    pipe_exe = get_exe_path("pipeline", ver)
    if check_exe(seq_exe) and check_exe(pipe_exe):
        available_versions.append((ver, seq_exe, pipe_exe))
    else:
        print(f"skip {ver}: no binaries", file=sys.stderr)

if not available_versions:
    print("no available versions", file=sys.stderr)
    sys.exit(1)

all_tasks = list(product(IMAGES, CONVOLUTIONS))
max_tasks = len(all_tasks)

print("=== compare with sequential load on single image ===")
for ver, seq_exe, pipe_exe in available_versions:
    print(f"  {ver} ...")
    errors = validate_pipeline(seq_exe, pipe_exe, all_tasks[:10])
    if errors:
        print(f"    errors: {len(errors)}")
        for e in errors:
            print(f"      {e}")
    else:
        print("    OK")

sizes = [15, 20, 25, 27, 30, 35, 40]

for batch_size in sizes:
    if batch_size > max_tasks:
        continue
    tasks = all_tasks[:batch_size]
    print(f"\n=== {batch_size} tasks ===")
    results = {}
    for ver, seq_exe, pipe_exe in available_versions:
        print(f"  seq_{ver} ...")
        t_seq = measure_batch(seq_exe, tasks)
        results[('seq', ver)] = t_seq
        if not np.isinf(t_seq):
            print(f"    mean {t_seq:.3f}s")
        else:
            print("    error")

        print(f"  pipeline_{ver} ...")
        t_pipe = measure_batch(pipe_exe, tasks)
        results[('pipeline', ver)] = t_pipe
        if not np.isinf(t_pipe):
            print(f"    mean {t_pipe:.3f}s")
        else:
            print("    error")
    plot_batch_results(results, tasks, batch_size)
