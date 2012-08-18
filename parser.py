import sys
import subprocess
import json

MAX_SIZE = 70

def is_ok(word):
    if len(word) <= 0:
        return True

    if word[0].isdigit() or word[0] == '-':
        return False

    return True

def parse_output_line(string):
    out = ""
    size = 0
    words = string.split(' ')

    for word in words:
        if size > MAX_SIZE and is_ok(word):
            out += "\n"
            size = 0
        size += len(word) + 1
        out += word + " "

    print out

def main(json_file):
    json_data = open(json_file).read()
    dictionary = json.loads(json_data)
    errors_type = dictionary['result']
    for error_type in errors_type:
        string =  "-" + error_type['value'] + ": " + error_type['key']

        where_info = error_type['where']
        string += " (" + where_info[0]['line'] + ";"
        for info in where_info[1:]:
            string += " " + info['line'] + ";"
        string += ")"

        parse_output_line(string)

if __name__ == "__main__":
    if sys.argv.__len__() < 2:
        print "Usage: parser $EXE"
    else:
        main(sys.argv[1])

