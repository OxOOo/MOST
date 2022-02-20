# %%
import os
import re
import json
import math
from tqdm import tqdm

raws = []
for file in os.listdir("data/download"):
    print(file)
    raws.append(open(os.path.join("data", "download", file), encoding="utf-8").read())

M3 = 125000000000000140750000000000052207500000000006359661
i = 0

for file in reversed(sorted(os.listdir("data/boards"))):
    with open(os.path.join("data", "boards", file), encoding="utf-8") as fd:
        data = json.load(fd)

    num = None
    if not re.compile(r"\d+").fullmatch(data["num"]):
        left, right = data["num"].split(".....")
        left = left.strip()
        right = right.strip()

        for raw in raws:
            if left not in raw:
                continue
            if right not in raw:
                continue
            left_pos = raw.index(left)
            right_pos = raw.index(right)
            begin = left_pos
            end = right_pos + len(right)

            if end - begin < 512:
                num = int(raw[begin:end])
            else:
                continue
    else:
        num = int(data["num"])
    if num is None:
        continue

    if num % M3 == 0:
        print(json.dumps(data, indent=4))

        i += 1
        if i > 10:
            break

# %%
