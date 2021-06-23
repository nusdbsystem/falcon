import argparse
import time

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--a')
    parser.add_argument('-b', '--b')
    parser.add_argument('-c', '--c')
    args = parser.parse_args()
    print("Args is : ", args.a, args.b, args.c)

    try:
        for i in range(3):
            print("-----------Python PreProcessing Counting down-----------", i)
            time.sleep(2)
    except:
        print("Error Happens")
