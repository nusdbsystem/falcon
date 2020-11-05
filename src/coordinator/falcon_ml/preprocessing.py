import argparse
import time

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--a', required=False, type=str, help="runtime conf path")
    parser.add_argument('-b', '--b', required=False, type=str, help="runtime conf path")
    args = parser.parse_args()
    print("Args is : ", args.a, args.b)

    try :
        time.sleep(1)
        for i in range(3):
            # print("processing Counting down", i)
            time.sleep(1)

        # print(args.a + args.b)
    except:
        print("error happens")
