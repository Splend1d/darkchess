_dir = "./v7submit_vs_computer"
import os
fs = os.listdir(_dir)
count = 0
final_times = []
for f in fs:
	fname = os.path.join(_dir,f)
	print(fname)
	if fname.endswith(".txt") and "wins" in fname:
		count += 1
		with open(fname,"r") as ff:
			data= ff.readlines()
	else:
		continue

	data = [d for d in data if "Time Left" in d]
	final_time = 900000-int(data[-1].split(":")[-1].strip())
	print(final_time)
	final_times.append(final_time)
	#break
print(f"Parsed files : {count}")
print(f"Average time usage : {sum(final_times)/len(final_times)}")