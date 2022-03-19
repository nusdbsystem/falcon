import math
import numpy as np
from scipy.optimize import minimize

def MaxV(a, b):
    if a > b:
        return a
    else:
        return b

def beta(x, t):
    mapper = dict()
    mapper[1] = 1.0
    mapper[2] = 0.95
    mapper[3] = 0.92
    mapper[4] = 0.91
    mapper[5] = 0.90
    mapper[6] = 0.88
    mapper[7] = 0.85
    mapper[8] = 0.80
    mapper[9] = 0.76
    mapper[10] = 0.71
    mapper[11] = 0.68
    mapper[12] = 0.65
    mapper[13] = 0.63
    mapper[14] = 0.58
    mapper[15] = 0.54
    mapper[16] = 0.53
    mapper[17] = 0.51
    mapper[18] = 0.47
    mapper[19] = 0.43
    mapper[20] = 0.39
    rk = int(round(x))
    result = float(t) / (mapper[rk] * float(x))
    return result

def objective(x):
    return 1 + x[0] + x[1] + x[2] * (x[3] + x[4])

def constraint1(x):
    #  >=0
    return totalWorkers - (1 + x[0] + x[1] + x[2] * (x[3] + x[4]))

def constraint2(x):
     # >=0
    return deadLine - (MaxV(beta(x[0], t2), beta(x[1], t3)) + math.ceil(6 / x[2]) * (beta(x[3], t4) + beta(x[4], t5)))-1


def schedule():

    if totalWorkers>20:
        print("Total worker number must be less than 20, since betais not defined. ")
        return

    # initial guesses
    n = 5
    x0 = np.zeros(n)
    x0[0] = 1.0
    x0[1] = 1.0
    x0[3] = 1.0
    x0[4] = 1.0
    x0[2] = 1.0

    # show initial objective
    print('Initial SSE Objective: ' + str(objective(x0)))

    # optimize
    b = (1.0, totalWorkers-5)
    c = (1.0, classNum)
    bnds = (b, b, c, b, b)
    con1 = {'type': 'ineq', 'fun': constraint1} # constraint1 >=0
    con2 = {'type': 'ineq', 'fun': constraint2} # constraint1 >=0
    cons = ([con1, con2])
    solution = minimize(fun=objective, x0=x0, method='SLSQP', bounds=bnds, constraints=cons)
    x = solution.x

    # show final objective
    # print solution
    print(' ====== Original Solution ====== ')
    print('x2 = ' + str(x[0]))
    print('x3 = ' + str(x[1]))
    print('x4 = ' + str(x[3]))
    print('x5 = ' + str(x[4]))
    print('xc = ' + str(x[2]))
    print('====== Final SSE Objective, worker used=' + str(objective(x)))
    print('====== timeUsed = ' + str(
        MaxV(beta(x[0], t2), beta(x[1], t3)) + math.ceil(6 / x[2]) * (beta(x[3], t4) + beta(x[4], t5))))

    X0 = [math.ceil(x[0]), math.floor(x[0])]
    X1 = [math.ceil(x[1]), math.floor(x[1])]
    X3 = [math.ceil(x[3]), math.floor(x[3])]
    X4 = [math.ceil(x[4]), math.floor(x[4])]
    X2 = [math.ceil(x[2]), math.floor(x[2])]

    bestResult = totalWorkers
    WorkerResult = []
    for a in X0:
        for b in X1:
            for c in X2:
                for d in X3:
                    for e in X4:
                        inputX = [a,b,c,d,e]
                        if constraint1(inputX) >=0 and constraint2(inputX) >=0:
                            if objective(inputX) < bestResult:
                                bestResult = objective(inputX)
                                WorkerResult = inputX

    x = WorkerResult
    print(' ====== updated Solution ====== ')
    print('x2 = ' + str(x[0]))
    print('x3 = ' + str(x[1]))
    print('x4 = ' + str(x[3]))
    print('x5 = ' + str(x[4]))
    print('xc = ' + str(x[2]))
    print('====== Final SSE Objective, worker used=' + str(objective(x)))
    print('====== timeUsed = ' + str(
        MaxV(beta(x[0], t2), beta(x[1], t3)) + math.ceil(6 / x[2]) * (beta(x[3], t4) + beta(x[4], t5))))

def main():
    global t2,t3, t4 ,t5, classNum, totalWorkers, deadLine
    # prediction time
    t2 = 60
    # // instance weighting time
    t3 = 30
    # // feature selection time
    t4 = 80
    # // VFL model training time
    t5 = 150
    classNum = 6

    totalWorkers = 20
    deadLine = 500

    schedule()

main()