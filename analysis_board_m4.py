import os
import re
import json
import math
from tqdm import tqdm

raws = []
for file in os.listdir("data/download"):
    print(file)
    raws.append(open(os.path.join("data", "download", file), encoding="utf-8").read())

M1 = 20220217214410
M2 = 104648257118348370704723119
M3 = 125000000000000140750000000000052207500000000006359661

m4_nums = []

for file in tqdm(os.listdir("data/boards")[:1000]):
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
        continue
    if num % M2 == 0:
        continue
    if num % M3 == 0:
        continue

    m4_nums.append(num)

print(len(m4_nums))

x = m4_nums[0]
for y in m4_nums:
    x = math.gcd(x, y)
print(x)

i3 = 0
while x % 3 == 0:
    i3 += 1
    x //= 3

i7 = 0
while x % 7 == 0:
    i7 += 1
    x //= 7

i11 = 0
while x % 11 == 0:
    i11 += 1
    x //= 11

print(i3)
print(i7)
print(i11)
print(x)
