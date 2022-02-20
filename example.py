# %%
from tqdm import tqdm
from multiprocessing import Pool

# %%

N = 256
M1 = 20220217214410  # 708
# M1 = 100699  # 3596
# M1 = 46589  # 6388
# M1 = 100699 * 431  # 1299
# M1 = 100699 * 431 * 2  # 974
# M1 = 46589 * 431 * 2 * 5  # 710
# M1 = 46589 * 2 * 5  # 1240
# M1 = 100699 * 2 * 5  # 904

M2 = 104648257118348370704723119  # 764

M3 = 125000000000000140750000000000052207500000000006359661  # 798
# M3 = 500000000000000147  # 798

M4 = 10885732038215355481752285039386319187390558900925053798518152998488201  # 759
# M4 = (3 ** 35) * (7 ** 20) * (11 ** 16)  # 772
# M4 = (3 ** 35) # 828
# M4 = (11 ** 16)  # 813
# M4 = (11 ** 7)  # 833
# M4 = (11 ** 5)  # 2203
# M4 = (11 ** 6)  # 948
# M4 = (3 ** 15)  # 822
# M4 = (3 ** 13)  # 992
# M4 = (3 ** 12)  # 1276

# %%

with open("data.bin", "r", encoding="utf-8") as fin:
    s = fin.read()
print(len(s))


def run(end):
    cnt1 = 0
    cnt2 = 0
    cnt3 = 0
    cnt4 = 0
    for begin in range(max(end-N, 0), end):
        if s[begin] != "0":
            num = int(s[begin:end])
            if num % M1 == 0:
                cnt1 += 1
                break
            if num % M2 == 0:
                cnt2 += 1
                break
            if num % M3 == 0:
                cnt3 += 1
                break
            if num % M4 == 0:
                cnt4 += 1
                break
    return dict(cnt1=cnt1, cnt2=cnt2, cnt3=cnt3, cnt4=cnt4)


cnt1 = 0
cnt2 = 0
cnt3 = 0
cnt4 = 0
with Pool() as p:
    for res in p.map(run, range(N, len(s)+1)):
        cnt1 += res["cnt1"]
        cnt2 += res["cnt2"]
        cnt3 += res["cnt3"]
        cnt4 += res["cnt4"]

print(cnt1)
print(cnt2)
print(cnt3)
print(cnt4)
