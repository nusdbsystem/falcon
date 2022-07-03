from csv import writer
from csv import reader
import argparse
import random
import numpy as np

# usage:  python3 extend_data.py --input_file /opt/falcon/2022sigmod-exp/3party/party0/data/synthetic_train/nFeaturePerParty/partyfeature50/client.txt --output_file /opt/falcon/2022sigmod-exp/3party/party0/data/synthetic_train/nFeaturePerParty/partyfeature50/client_new.txt --expect_num 10000

def get_args():
    parser = argparse.ArgumentParser(description='append binary class label to the file and extend to specific number of rows')
    parser.add_argument('--input_file', type=str, help='the input file')
    parser.add_argument('--output_file', type=str, help='the output file')
    parser.add_argument('--expect_num', type=int, help='the expected number of rows')
    return parser.parse_args()

args = get_args()

with open(args.input_file, 'r') as read_obj, \
        open(args.output_file, 'w', newline='') as write_obj:
    # Create a csv.reader object from the input file object
    csv_reader = reader(read_obj)
    lines = [tuple(line) for line in csv_reader]
    # Create a csv.writer object from the output file object
    csv_writer = writer(write_obj)
    # Read each row of the input csv file as list
    for i in range(args.expect_num):
        chosen_row = random.choice(lines)
        # Add the updated row / list to the output file
        csv_writer.writerow(chosen_row)


