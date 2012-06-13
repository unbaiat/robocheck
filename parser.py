import sys
import subprocess
import json

def main(command):
	process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
	process.wait()
	json_string = process.communicate()[0]
	dictionary = json.loads(json_string)
	print "Numar total de depunctari: " + str(dictionary['result'].__len__())
	list_points = [e['value'] for e in dictionary['result']]
	total = 0.0
	for points in list_points:
		total += float(points)
	print "Depunctare totala: " + str(total) + " puncte"
	

if __name__ == "__main__":
	if sys.argv.__len__() < 2:
		print "Usage: parser $EXE"
	else:
		main(sys.argv[1])

