import os
import re
import matplotlib.pyplot as plt
from datetime import datetime

# Путь к папке с картами памяти
maps_dir = "/home/vinogradov/OS_vinogradov/labs/OS_LABS/lab5/maps"

# Список для хранения (время, размер кучи)
heap_sizes = []

# Проходим по всем файлам
for filename in sorted(os.listdir(maps_dir)):
    if filename.startswith("map_") and filename.endswith(".txt"):
        filepath = os.path.join(maps_dir, filename)

        # Вытаскиваем дату-время из имени файла
        match = re.search(r"map_\d+_(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})", filename)
        if match:
            timestamp_str = match.group(1).replace("_", " ").replace("-", ":")
            timestamp = datetime.strptime(timestamp_str, "%Y:%m:%d %H:%M:%S")

        with open(filepath, "r") as f:
            for line in f:
                if "[heap]" in line:
                    parts = line.split()
                    addresses = parts[0]
                    start_str, end_str = addresses.split("-")
                    start = int(start_str, 16)
                    end = int(end_str, 16)
                    heap_size = end - start
                    heap_sizes.append((timestamp, heap_size))
                    break  # нашли кучу — дальше читать не надо

# Отсортируем по времени
heap_sizes.sort()

# Разделим на списки времени и размеров
times = [item[0] for item in heap_sizes]
sizes_kb = [item[1] / 1024 for item in heap_sizes]  # переводим в КБ

# Рисуем график
plt.figure(figsize=(10, 6))
plt.plot(times, sizes_kb, marker="o")
plt.title("Изменение размера кучи процесса")
plt.xlabel("Время")
plt.ylabel("Размер кучи (КБ)")
plt.grid(True)
plt.xticks(rotation=45)
plt.tight_layout()
plt.show()
