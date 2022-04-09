import math
import numpy as np
from scipy import optimize

def MaxV(a, b):
    if a > b:
        return a
    else:
        return b


def model_prediction(x):
    return 6.09377075279683+ 161.444662942579/x


def instance_weighting(x):
    return 10.569445316823112+ 325.9142691796053/x


def feature_selection(x):
    return 42.574153980931875+ 432.4790678336969/x


def VFL_training(x):
    return 66.9453565626985+ 436.11481024283364/x


def beta(workerNum, stageName):

    if stageName == "model_prediction":
        return model_prediction(workerNum)
    if stageName == "instance_weighting":
        return instance_weighting(workerNum)
    if stageName == "feature_selection":
        return feature_selection(workerNum)
    if stageName == "VFL_training":
        return VFL_training(workerNum)


def objective(x):
    return measure_total_time(x)


def worker_constraint(x):
    #  >=0
    return totalWorkers - measure_total_worker(x)


def time_constraint(x):
     # >=0
    return deadLine - measure_total_time(x)

def measure_total_worker(x):
    return 1 + x[0] + x[1] + x[2] * (x[3] + x[4])


def measure_total_time(x):
    prediction_time = beta(x[0], "model_prediction")
    weight_time = beta(x[1], "instance_weighting")
    feature_selection_time = beta(x[3], "feature_selection")
    train_time = beta(x[4], "VFL_training")

    time_used = MaxV(prediction_time, weight_time) + math.ceil(classNum / x[2]) * (feature_selection_time + train_time)

    return time_used

def brute_force_search():
    bestResult = float("inf")
    min_time_used = float("inf")
    max_worker_used = 0
    WorkerResult = []
    for x0 in range(1, totalWorkers):
        for x1 in range(1, totalWorkers):
            for x3 in range(1, totalWorkers):
                for x4 in range(1, totalWorkers):
                    for x2 in range(1, classNum+1):
                        inputX = [x0, x1, x2, x3, x4]
                        worker_cons = worker_constraint(inputX)
                        time_cons = time_constraint(inputX)
                        time_used = measure_total_time(inputX)
                        worker_used = measure_total_worker(inputX)
                        if time_used < min_time_used:
                            min_time_used = time_used
                        if worker_used > max_worker_used:
                            max_worker_used = worker_used
                        if time_cons >= 0 and worker_cons >= 0:
                            if objective(inputX) < bestResult:
                                bestResult = objective(inputX)
                                WorkerResult = inputX

    print("\n ------------ Best policy ------------")

    if len(WorkerResult) > 0:
        print("OK,", "worker_used=", measure_total_worker(WorkerResult), ", time_used=", measure_total_time(WorkerResult))
        print(str(int(WorkerResult[0]))) # prediction
        print(str(int(WorkerResult[1]))) # instance weighting
        print(str(int(WorkerResult[3]))) # feature selection
        print(str(int(WorkerResult[4]))) # vfl training
        print(str(int(WorkerResult[2]))) # class parallelism
    else:
        print("ERROR")
        print('====== max worker used=', max_worker_used)
        print('====== min timeUsed = ', min_time_used)
        print("worker_used=", measure_total_worker(WorkerResult), ", time_used=", measure_total_time(WorkerResult))
        print('====== requirement is worker < ', totalWorkers, " time < ", deadLine)

def schedule():

    # initial guesses
    n = 5
    x0 = np.zeros(n)
    x0[0] = 1.0
    x0[1] = 1.0
    x0[3] = 1.0
    x0[4] = 1.0
    x0[2] = 1.0

    # show initial objective
    # print('Initial SSE Objective: ' + str(objective(x0)))
    WorkerResult = []

    min_time_used = float("inf")
    max_worker_used = 0

    try:
        # optimize
        b = (1.0, totalWorkers-5)
        c = (1.0, classNum)
        bnds = (b, b, c, b, b)
        con1 = {'type': 'ineq', 'fun': worker_constraint} # constraint1 >=0
        con2 = {'type': 'ineq', 'fun': time_constraint} # constraint1 >=0
        cons = ([con1, con2])
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
        X3 = [math.ceil(x[3]), math.floor(x[3])]
        X4 = [math.ceil(x[4]), math.floor(x[4])]
        X2 = [math.ceil(x[2]), math.floor(x[2])]

        X0 = [ele for ele in X0 if ele >= 1]
        X1 = [ele for ele in X1 if ele >= 1]
        X2 = [ele for ele in X2 if ele >= 1]
        X3 = [ele for ele in X3 if ele >= 1]
        X4 = [ele for ele in X4 if ele >= 1]

        bestResult = float("inf")
        for a in X0:
            for b in X1:
                for c in X2:
                    for d in X3:
                        for e in X4:
                            inputX = [a, b, c, d, e]
                            worker_cons = worker_constraint(inputX)
                            time_cons = time_constraint(inputX)
                            time_used = measure_total_time(x)
                            worker_used = measure_total_worker(x)
                            if time_used < min_time_used:
                                min_time_used = time_used
                            if worker_used > max_worker_used:
                                max_worker_used = worker_used
                            if time_cons >= 0 and worker_cons >= 0:
                                if objective(inputX) < bestResult:
                                    bestResult = objective(inputX)
                                    WorkerResult = inputX
    except Exception as e:
        print("ERROR")
        print(e)

    if len(WorkerResult) > 0:
        print("OK,", "worker_used=", measure_total_worker(WorkerResult), ", time_used=", measure_total_time(WorkerResult))
        print(str(int(WorkerResult[0]))) # prediction
        print(str(int(WorkerResult[1]))) # instance weighting
        print(str(int(WorkerResult[3]))) # feature selection
        print(str(int(WorkerResult[4]))) # vfl training
        print(str(int(WorkerResult[2]))) # class parallelism
    else:
        print("ERROR")
        print('====== max worker used=', max_worker_used)
        print('====== min timeUsed = ', min_time_used)
        print('====== requirement is worker < ', totalWorkers, " time < ", deadLine)

def main():
    import argparse

    defaultWorker = 19
    defaultDeadline = 18901
    defaultClass = 4

    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--worker', type=int, help='total workers', default=defaultWorker)
    parser.add_argument('-d', '--deadline', type=int, help='deadline', default=defaultDeadline)
    parser.add_argument('-c', '--classNum', type=int, help='classNum', default=defaultClass)
    args = parser.parse_args()

    global classNum, totalWorkers, deadLine

    classNum = args.classNum
    totalWorkers = args.worker
    deadLine = args.deadline
    schedule()
    # brute_force_search()

main()