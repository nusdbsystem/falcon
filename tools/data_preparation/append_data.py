from csv import writer
from csv import reader
import random
# Open the input_file in read mode and output_file in write mode
input_file = "/experiments/efficiency/falcon_lime/synthetic/nFeaturePerParty/partyfeature20/client0/synthesis_nclass2_nfeaturepaty20_nparty3_nsample4000_party.csv"
output_file = "/experiments/efficiency/falcon_lime/synthetic/nFeaturePerParty/partyfeature20/client0/synthesis_nclass2_nfeaturepaty20_nparty3_nsample4000_party.csv"

with open(input_file, 'r') as read_obj, \
        open(output_file, 'w', newline='') as write_obj:
    # Create a csv.reader object from the input file object
    csv_reader = reader(read_obj)
    # Create a csv.writer object from the output file object
    csv_writer = writer(write_obj)
    # Read each row of the input csv file as list
    for row in csv_reader:
        # Append the default text in the row / list
        k = random.randint(0, 1) # decide on k once
        row.append(k)
        # Add the updated row / list to the output file
        csv_writer.writerow(row)


