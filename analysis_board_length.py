# %%
import os
import re
import json
import math
from tqdm import tqdm
import pandas as pd

raws = []
for file in os.listdir("data/download"):
    print(file)
    raws.append(open(os.path.join("data", "download", file), encoding="utf-8").read())

M1 = 20220217214410
M2 = 104648257118348370704723119
M3 = 125000000000000140750000000000052207500000000006359661
M4 = 10885732038215355481752285039386319187390558900925053798518152998488201

m1length = []
m2length = []
m3length = []
m4length = []

for file in tqdm(os.listdir("data/boards")):
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

    if num % M1 == 0:
        m1length.append(len(str(num)))
    if num % M2 == 0:
        m2length.append(len(str(num)))
    if num % M3 == 0:
        m3length.append(len(str(num)))
    if num % M4 == 0:
        m4length.append(len(str(num)))

# %%

pd.Series(m1length).hist()

# %%

pd.Series(m2length).hist()

# %%

pd.Series(m3length).hist()

# %%

pd.Series(m4length).hist()

# %%
