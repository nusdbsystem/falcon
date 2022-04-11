

import math
import numpy as np
from scipy import optimize

def MaxV(a, b):
    if a > b:
        return a
    else:
        return b


def feature_selection(x):
    return 42.574153980931875+ 432.4790678336969/x


def VFL_training(x):
    return 66.9453565626985+ 436.11481024283364/x


def beta(workerNum, stageName):

    if stageName == "feature_selection":
        return feature_selection(workerNum)
    if stageName == "VFL_training":
        return VFL_training(workerNum)


def objective(x):
    return measure_total_time(x)


def worker_constraint4(x):
    #  >=0
    return totalWorkers - measure_total_worker4(x)

def worker_constraint5(x):
    #  >=0
    return totalWorkers - measure_total_worker5(x)

def measure_total_worker4(x):
    ps3 = 0
    if x[0] > 1:
        ps3 += 1
    return x[2] * (x[0]+ps3)

def measure_total_worker5(x):
    ps4 = 0
    if x[1] > 1:
        ps4 += 1
    return x[2] * (x[1]+ps4)

def measure_total_time(x):
    feature_selection_time = beta(x[0], "feature_selection")
    train_time = beta(x[1], "VFL_training")
    time_used = math.ceil(classNum / x[2]) * (feature_selection_time + train_time)

    return time_used


def schedule():

    # initial guesses
    n = 3
    x0 = np.zeros(n)
    x0[0] = 1.0
    x0[1] = 1.0
    x0[2] = 1.0

    # show initial objective
    # print('Initial SSE Objective: ' + str(objective(x0)))
    WorkerResult = []

    min_time_used = float("inf")
    max_worker_used = 0

    try:
        # optimize
        b = (1.0, totalWorkers)
        c = (1.0, classNum)
        bnds = (b, b, c)
        con0 = {'type': 'ineq', 'fun': worker_constraint4} # constraint1 >=0
        con1 = {'type': 'ineq', 'fun': worker_constraint5}  # constraint1 >=0
        cons = ([con0, con1])
        solution = optimize.minimize(fun=objective, x0=x0, method='COBYLA', constraints=cons)
        # solution = optimize.shgo(func=objective, bounds=bnds, constraints=cons, sampling_method='sobol')
        # solution = optimize.shgo(func=objective, bounds=bnds, constraints=cons)
        # solution = optimize.brute(func=objective, ranges=bnds)

        x = solution.x

        # show final objective
        # print solution
        # print(' ====== Original Solution ====== ')
        # print('x2 = ' + str(x[0]))
        # print('x3 = ' + str(x[1]))
        # print('x4 = ' + str(x[3]))
        # print('x5 = ' + str(x[4]))
        # print('xc = ' + str(x[2]))
        # print('====== Final SSE Objective, worker used=' + str(objective(x)))
        # print('====== timeUsed = ' + str()

        X0 = [math.ceil(x[0]), math.floor(x[0])]
        X1 = [math.ceil(x[1]), math.floor(x[1])]
        X2 = [math.ceil(x[2]), math.floor(x[2])]

        X0 = [ele for ele in X0 if ele >= 1]
        X1 = [ele for ele in X1 if ele >= 1]
        X2 = [ele for ele in X2 if ele >= 1]

        bestResult = float("inf")
        for a in X0:
            for b in X1:
                for c in X2:
                        inputX = [a, b, c]
                        worker_cons = worker_constraint4(inputX)
                        worker_cons2 = worker_constraint5(inputX)
                        if worker_cons >= 0 and worker_cons2 > 0:
                            if objective(inputX) < bestResult:
                                bestResult = objective(inputX)
                                WorkerResult = inputX
    except Exception as e:
        print("ERROR")
        print(e)

    if len(WorkerResult) > 0:
        print("OK,", "worker_used=", measure_total_worker4(WorkerResult), measure_total_worker5(WorkerResult), ", time_used=", measure_total_time(WorkerResult))
        # feature selection
        if int(WorkerResult[0]) > 1:
            print(str(int(WorkerResult[0])+1))
        else:
            print(str(int(WorkerResult[0])))

        # vfl training
        if int(WorkerResult[1]) > 1:
            print(str(int(WorkerResult[1])+1))
        else:
            print(str(int(WorkerResult[1])))

        print(str(int(WorkerResult[2]))) # class parallelism
    else:
        print("ERROR")
        print('====== max worker used=', max_worker_used)
        print('====== min timeUsed = ', min_time_used)
        print('====== requirement is worker < ', totalWorkers, " time < ", deadLine)

    return measure_total_time(WorkerResult)

def main(defaultWorker):
    import argparse

    defaultDeadline = 18901
    defaultClass = 6

    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--worker', type=int, help='total workers', default=defaultWorker)
    parser.add_argument('-d', '--deadline', type=int, help='deadline', default=defaultDeadline)
    parser.add_argument('-c', '--classNum', type=int, help='classNum', default=defaultClass)
    args = parser.parse_args()

    global classNum, totalWorkers, deadLine

    classNum = args.classNum
    totalWorkers = args.worker
    deadLine = args.deadline
    # brute_force_search()
    return schedule()

res = []
for defaultWorker in range(10, 65, 5):
    res.append(main(defaultWorker))
print(res)