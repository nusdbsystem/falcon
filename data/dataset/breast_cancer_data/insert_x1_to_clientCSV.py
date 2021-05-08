import csv

with open('/opt/falcon/data/dataset/breast_cancer_data/client0/client.txt', 'r', newline='') as input_file, \
        open('/opt/falcon/data/dataset/breast_cancer_data/client0/client_with_x1.csv', 'w', newline='') as output_file:
    csv_reader = csv.reader(input_file, delimiter=',')
    csv_writer = csv.writer(output_file, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    line_count = 0
    for row in csv_reader:
        x1 = '1'
        # insert x1=1 to front of the features
        row.insert(0, x1)
        print(row)
        # Add the updated row / list to the output file
        csv_writer.writerow(row)
        line_count += 1
    print(f'Processed {line_count} lines.')
