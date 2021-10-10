import argparse
import time

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--a')
    parser.add_argument('-b', '--b')
    parser.add_argument('-c', '--c')
    args = parser.parse_args()
    print("Args is : logfile", args.a, "b :",  args.b, "c :", args.c)


    try:
        fo = open(args.a + "/texts", "w")
        fo.write("This is worker, workerID" + str(args.b) + ", " + args.c)
        fo.close()
    except:
        print("This is wrong")
