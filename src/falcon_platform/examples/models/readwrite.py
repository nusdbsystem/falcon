

import argparse
import time

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--i')
    parser.add_argument('-o', '--o')
    parser.add_argument('-model', '--model')
    args = parser.parse_args()

    print("Args is : ", args.i, args.o, args.model)

    try:
        with open(args.i, "r") as f:
            res = f.read()
            print(res)

        with open(args.o, "w") as f:
            f.write("finish data process!")

        with open(args.model, "w") as f:
            f.write("finish model trained!")
    except Exception as e:
        print(e)

    while True:
        print("Python PreProcessing Counting down")
        time.sleep(3)

# /Users/nailixing/Test/falcon_demon_12_11/p1/data/f1
# /Users/nailixing/Test/falcon_demon_12_11/p1/model/o1
