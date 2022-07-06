import math
import numpy as np
from scipy import optimize

def MaxV(a, b):
    if a > b:
        return a
    else:
        return b


def model_prediction(x):
    model_prediction_lr = dict()
    model_prediction_lr[1] = 151.71
    model_prediction_lr[2] = 82.57
    model_prediction_lr[3] = 55.86
    model_prediction_lr[4] = 42.69
    model_prediction_lr[5] = 40.66
    model_prediction_lr[6] = 35.05
    model_prediction_lr[7] = 31.56
    model_prediction_lr[8] = 30.01

    # if x in model_prediction_lr:
    #     return model_prediction_lr[x]

    return 11.067116307237646+ 140.39482190448214/x


def instance_weighting(x):
    instance_weighting = dict()
    instance_weighting[1] = 237.02
    instance_weighting[2] = 142.07
    instance_weighting[3] = 95.56
    instance_weighting[4] = 88.51
    instance_weighting[5] = 75.365
    instance_weighting[6] = 64.73
    instance_weighting[7] = 54.01
    instance_weighting[8] = 47.67
    # if x in instance_weighting:
    #     return instance_weighting[x]
    return 28.568537763734497+ 212.0739491077336/x


def feature_selection(x):
    feature_selection = dict()
    feature_selection[1] = 53.48
    feature_selection[2] = 44.61
    feature_selection[3] = 36.97
    feature_selection[4] = 32.93
    feature_selection[5] = 30.68
    feature_selection[6] = 29.19
    feature_selection[7] = 28.07
    feature_selection[8] = 27.11
    # if x in feature_selection:
    #     return feature_selection[x]
    return 24.929086085421833+ 30.762217008446868/x


def VFL_training(x):
    VFL_training = dict()
    VFL_training[1] = 71.69
    VFL_training[2] = 63.01
    VFL_training[3] = 54
    VFL_training[4] = 51.35
    VFL_training[5] = 49.46
    VFL_training[6] = 47.87
    VFL_training[7] = 46.71
    VFL_training[8] = 45.87
    # if x in VFL_training:
    #     return VFL_training[x]
    return 43.48788216642094+ 30.19177917524453/x


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
    return totalWorkers - measure_total_worker(x)

def worker_constraint0(x):
    return x[0] - 1

def worker_constraint1(x):
    return x[1] - 1

def worker_constraint2(x):
    return x[2] - 1

def worker_constraint3(x):
    return x[3] - 1

# >= 0
def time_constraint(x):
    return deadLine - measure_total_time(x)

def measure_total_worker(x):
    ps0 = 0
    ps1 = 0
    ps3 = 0
    ps4 = 0
    if x[0] > 1:
        ps0 += 1
    if x[1] > 1:
        ps1 += 1
    if x[2] > 1:
        ps3 += 1
    if x[3] > 1:
        ps4 += 1
    return 1 + x[0]+ps0 + x[1]+ps1 + classNum * (x[2]+ps3 + x[3]+ps4)


def measure_total_time(x):
    prediction_time = beta(x[0], "model_prediction")
    weight_time = beta(x[1], "instance_weighting")
    feature_selection_time = beta(x[2], "feature_selection")
    train_time = beta(x[3], "VFL_training")

    time_used = MaxV(prediction_time, weight_time) + (feature_selection_time + train_time)

    return time_used

def brute_force_search():
    bestResult = float("inf")
    min_time_used = float("inf")
    max_worker_used = 0
    WorkerResult = []
    for x0 in range(1, totalWorkers):
        for x1 in range(1, totalWorkers):
            for x2 in range(1, totalWorkers):
                for x3 in range(1, totalWorkers):
                        inputX = [x0, x1, x2, x3]
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
        print(str(int(WorkerResult[2]))) # feature selection
        print(str(int(WorkerResult[3]))) # vfl training
    else:
        print("ERROR")
        print('====== max worker used=', max_worker_used)
        print('====== min timeUsed = ', min_time_used)
        print("worker_used=", measure_total_worker(WorkerResult), ", time_used=", measure_total_time(WorkerResult))
        print('====== requirement is worker < ', totalWorkers, " time < ", deadLine)

    return measure_total_time(WorkerResult)

def schedule():

    # initial guesses
    n = 4
    x0 = np.zeros(n)
    x0[0] = 1.0
    x0[1] = 1.0
    x0[2] = 1.0
    x0[3] = 1.0

    WorkerResult = []

    min_time_used = float("inf")
    max_worker_used = 0

    try:
        # optimize
        b = (1.0, totalWorkers-1)
        bnds = (b, b, b, b)
        con1 = {'type': 'ineq', 'fun': worker_constraint} # worker_constraint >=0
        con2 = {'type': 'ineq', 'fun': time_constraint} # time_constraint >=0

        con6 = {'type': 'ineq', 'fun': worker_constraint0}  # constraint1 >=0, worker0>1
        con3 = {'type': 'ineq', 'fun': worker_constraint1}  # constraint1 >=0, worker1>1
        con4 = {'type': 'ineq', 'fun': worker_constraint2}  # constraint1 >=0, worker2>1
        con5 = {'type': 'ineq', 'fun': worker_constraint3}  # constraint1 >=0, worker3>1
        cons = ([con1, con2, con6, con3, con4, con5])

        # solution = optimize.minimize(fun=objective, x0=x0, method='Nelder-Mead', constraints=cons)
        # solution = optimize.minimize(fun=objective, x0=x0, method='Powell', constraints=cons)
        # solution = optimize.minimize(fun=objective, x0=x0, method='CG', constraints=cons)
        # solution = optimize.minimize(fun=objective, x0=x0, method='BFGS')
        # solution = optimize.minimize(fun=objective, x0=x0, method='L-BFGS-B', constraints=cons)
        # solution = optimize.minimize(fun=objective, x0=x0, method='TNC', constraints=cons)
        # solution = optimize.minimize(fun=objective, x0=x0, method='SLSQP', constraints=cons)
        # solution = optimize.minimize(fun=objective, x0=x0, method='dogleg', constraints=cons)
        # solution = optimize.minimize(fun=objective, x0=x0, method='trust-krylov', constraints=cons)
        solution = optimize.minimize(fun=objective, x0=x0, method='COBYLA', constraints=cons)
        # solution = optimize.shgo(func=objective, bounds=bnds, constraints=cons, sampling_method='sobol')
        # solution = optimize.shgo(func=objective, bounds=bnds, constraints=cons)
        # solution = optimize.brute(func=objective, ranges=bnds)

        x = solution.x

        X0 = [math.ceil(x[0]), math.floor(x[0])]
        X1 = [math.ceil(x[1]), math.floor(x[1])]
        X2 = [math.ceil(x[2]), math.floor(x[2])]
        X3 = [math.ceil(x[3]), math.floor(x[3])]

        X0 = [ele for ele in X0 if ele >= 1]
        X1 = [ele for ele in X1 if ele >= 1]
        X2 = [ele for ele in X2 if ele >= 1]
        X3 = [ele for ele in X3 if ele >= 1]

        bestResult = float("inf")
        for a in X0:
            for b in X1:
                for c in X2:
                    for d in X3:
                            inputX = [a, b, c, d]
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
    except Exception as e:
        print("ERROR")
        print(e)

    if len(WorkerResult) > 0:
        print("OK,", "worker_used=", measure_total_worker(WorkerResult), ", time_used=", measure_total_time(WorkerResult))
        # prediction
        if int(WorkerResult[0]) > 1:
            print(str(int(WorkerResult[0])+1))
        else:
            print(str(int(WorkerResult[0])))

        # instance weighting
        if int(WorkerResult[1]) > 1:
            print(str(int(WorkerResult[1])+1))
        else:
            print(str(int(WorkerResult[1])))

        # feature selection
        if int(WorkerResult[2]) > 1:
            print(str(int(WorkerResult[2])+1))
        else:
            print(str(int(WorkerResult[2])))

        # vfl training
        if int(WorkerResult[3]) > 1:
            print(str(int(WorkerResult[3])+1))
        else:
            print(str(int(WorkerResult[3])))

    else:
        print("ERROR")
        print('====== max worker used=', max_worker_used)
        print('====== min timeUsed = ', min_time_used)
        print('====== requirement is worker < ', totalWorkers, " time < ", deadLine)

    return measure_total_time(WorkerResult)


def scheduleAverage():
    average = int((totalWorkers - 1) / (1 + 1 + 2 * (1 + 1)))
    if average == 2:
        average = 1
    total_worker_used = average * 6
    left = totalWorkers - total_worker_used - 1

    pa = 0
    if int(average + left / 2) == 2:
        pa = 1
    if average + left / 2 >= 3:
        pa = int(average + left / 2) - 1

    if average >= 3:
        average = average - 1

    WorkerResult = [pa, pa, average, average]

    if len(WorkerResult) > 0:
        print("OK,", "worker_used=", measure_total_worker(WorkerResult), ", time_used=", measure_total_time(WorkerResult))
        # prediction
        if int(WorkerResult[0]) > 1:
            print(str(int(WorkerResult[0])+1))
        else:
            print(str(int(WorkerResult[0])))

        # instance weighting
        if int(WorkerResult[1]) > 1:
            print(str(int(WorkerResult[1])+1))
        else:
            print(str(int(WorkerResult[1])))

        # feature selection
        if int(WorkerResult[2]) > 1:
            print(str(int(WorkerResult[2])+1))
        else:
            print(str(int(WorkerResult[2])))

        # vfl training
        if int(WorkerResult[3]) > 1:
            print(str(int(WorkerResult[3])+1))
        else:
            print(str(int(WorkerResult[3])))

    else:
        print("ERROR")
        print('====== max worker used=', max_worker_used)
        print('====== min timeUsed = ', min_time_used)
        print('====== requirement is worker < ', totalWorkers, " time < ", deadLine)

    return measure_total_time(WorkerResult)


def main(defaultWorker, defaultAverage):
    import argparse

    defaultDeadline = 18901
    defaultClass = 2

    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--worker', type=int, help='total workers', default=defaultWorker)
    parser.add_argument('-d', '--deadline', type=int, help='deadline', default=defaultDeadline)
    parser.add_argument('-c', '--classNum', type=int, help='classNum', default=defaultClass)
    parser.add_argument('-a', '--isAverage', type=int, help='isAverage', default=defaultAverage)
    args = parser.parse_args()

    global classNum, totalWorkers, deadLine, isaverage

    classNum = args.classNum
    totalWorkers = args.worker
    deadLine = args.deadline
    isaverage = args.isAverage

    # return brute_force_search()
    if args.isAverage:
        return scheduleAverage()
    else:
        return schedule()


main(10, 0)



# xxx = range(20, 38, 2)
#
# time_result = []
# for defaultWorker in xxx:
#     print("--------------", defaultWorker, "--------------")
#     time_result.append(main(defaultWorker, 0))
# print(time_result)
#
# # average
# averageList = []
# for defaultWorker in xxx:
#     print("-------------- In average phase, ", defaultWorker, "--------------")
#     averageList.append(main(defaultWorker, 1))
# print("average: ", averageList)
#
# # plot
# import matplotlib.pyplot as plt
# plt.plot(xxx, time_result, 'r', )
# plt.plot(xxx, averageList, 'r', )
# # plt.plot(xxx, ybest, 'r', )
#
# plt.plot(xxx, time_result, '*', label='autoParallel')
# plt.plot(xxx, averageList, '*', label='average')
# # plt.plot(xxx, ybest, '*', label='best')
# plt.xlabel('worker number')
# plt.ylabel('time usage')
# plt.legend(loc=2)
# plt.title('polyfitting')
# plt.show()