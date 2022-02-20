import os
import re
import json
import math
from tqdm import tqdm
import time

raws = []
for file in os.listdir("data/download"):
    print(file)
    raws.append(open(os.path.join("data", "download", file), encoding="utf-8").read())

ME = "lammbda"

M1 = 20220217214410
M2 = 104648257118348370704723119
M3 = 125000000000000140750000000000052207500000000006359661
M4 = 10885732038215355481752285039386319187390558900925053798518152998488201

m1score = 0
m2score = 0
m3score = 0
m4score = 0

for file in tqdm(os.listdir("data/boards")):
    if float(file) < time.time() - 60 * 60:
        continue

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

    idx = -1
    for i, item in enumerate(data["submits"]):
        if item["user"].startswith(ME):
            idx = i
            break
    if idx < 0:
        print("not me", num)
        continue

    if idx+1 in [1]:
        score = 15
    elif idx+1 in [2, 3]:
        score = 7
    elif idx+1 in [4, 5, 6]:
        score = 3
    elif idx+1 in [7, 8, 9, 10]:
        score = 1
    else:
        score = -1

    if num % M1 == 0:
        m1score += score
    if num % M2 == 0:
        m2score += score
    if num % M3 == 0:
        m3score += score
    if num % M4 == 0:
        m4score += score

print(m1score)
print(m2score)
print(m3score)
print(m4score)
