import requests
import time
import os
import json

while True:
    time.sleep(3)

    try:
        r = requests.get("http://172.1.1.119:10000/board.txt", timeout=10)
        text = r.text
    except Exception as e:
        print(e)
        continue

    if "Recent Solved" not in text:
        print("no Sampled Recent Solved")
        continue

    lines = text.split("\n")
    pos = -1
    for i, line in enumerate(lines):
        if "Recent Solved" in line:
            assert pos == -1
            pos = i
    assert pos >= 0

    contents = "\n".join(lines[pos+1:]).strip()
    save_cnt = 0
    for item in contents.split("\n\n"):
        item = item.strip()

        num = item.split("\n")[0]
        assert item.split("\n")[1].strip().split(" ")[-1] == "send"
        send_time = item.split("\n")[1].strip().split(" ")[0]

        data = {
            "num": num,
            "send_time": send_time,
            "submits": []
        }
        for line in item.split("\n")[2:]:
            submit_time, user = line.strip().split(" ")
            data["submits"].append({
                "time": submit_time,
                "user": user
            })

        save_file = os.path.join("data", "boards", send_time)
        if os.path.exists(save_file):
            continue
        os.makedirs(os.path.dirname(save_file), exist_ok=True)
        with open(save_file, "w", encoding="utf-8") as fd:
            json.dump(data, fd, indent=4)
        save_cnt += 1

    print(f"save_cnt = {save_cnt}")
