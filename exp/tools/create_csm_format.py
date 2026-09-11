import sys
from pathlib import Path


def process_file(input_path, output_dir):
    max_id = -1
    edge_lines = []

    with open(input_path, "r") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 3:
                continue

            src = int(parts[0])
            dst = int(parts[1])
            op = parts[2]

            max_id = max(max_id, src, dst)

            if op == "+1":
                edge_lines.append(f"e {src} {dst} 0\n")
            elif op == "-1":
                edge_lines.append(f"-e {src} {dst} 0\n")

    base = input_path.stem
    updates_path = output_dir / f"{base}.updates"
    vertices_path = output_dir / f"{base}.vertices"

    with open(updates_path, "w") as f:
        f.writelines(edge_lines)

    with open(vertices_path, "w") as f:
        for vid in range(max_id + 1):
            f.write(f"v {vid} 0\n")


def main():
    if len(sys.argv) != 3:
        print("Usage: python convert.py <input_folder> <output_folder>")
        sys.exit(1)

    input_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    output_dir.mkdir(parents=True, exist_ok=True)

    for file in input_dir.glob("*.e"):
        process_file(file, output_dir)
        print(f"Processed {file.name}")


if __name__ == "__main__":
    main()
