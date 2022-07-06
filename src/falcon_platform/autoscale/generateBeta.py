import math

import numpy as np
import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import curve_fit

model_prediction_lr = dict()
model_prediction_lr[1] = 151.71
model_prediction_lr[2] = 82.57
model_prediction_lr[3] = 55.86
model_prediction_lr[4] = 42.69
model_prediction_lr[5] = 40.66
model_prediction_lr[6] = 35.05
model_prediction_lr[7] = 31.56
model_prediction_lr[8] = 30.01

instance_weighting = dict()
instance_weighting[1] = 237.02
instance_weighting[2] = 142.07
instance_weighting[3] = 95.56
instance_weighting[4] = 88.51
instance_weighting[5] = 75.365
instance_weighting[6] = 64.73
instance_weighting[7] = 54.01
instance_weighting[8] = 47.67

feature_selection = dict()
feature_selection[1] = 53.48
feature_selection[2] = 44.61
feature_selection[3] = 36.97
feature_selection[4] = 32.93
feature_selection[5] = 30.68
feature_selection[6] = 29.19
feature_selection[7] = 28.07
feature_selection[8] = 27.11

VFL_training = dict()
VFL_training[1] = 71.69
VFL_training[2] = 63.01
VFL_training[3] = 54
VFL_training[4] = 51.35
VFL_training[5] = 49.46
VFL_training[6] = 47.87
VFL_training[7] = 46.71
VFL_training[8] = 45.87


def fitInverse(the_map):
    def funcinv(x, a, b):
        return b + a / x

    x, y = [], []
    for k, v in the_map.items():
        x.append(k)
        y.append(v)

    xxx = np.array(x)
    yyy = np.array(y)

    params = np.array([1, 1])

    popt, pcov = curve_fit(funcinv, xxx, yyy, params)

    yvals = funcinv(xxx, popt[0], popt[1])

    print(popt, pcov)
    resultstr = str(popt[1]) + "+ " + str(popt[0]) + "/x"

    print(resultstr)

    plt.plot(xxx, yyy, '*', label='original values')
    plt.plot(xxx, yvals, 'r', label='polyfit values')
    plt.xlabel('x axis')
    plt.ylabel('y axis')
    plt.legend(loc=2)
    plt.title('polyfitting')
    plt.show()

def fitMultiple(the_map):
    x, y = [], []
    for k, v in the_map.items():
        x.append(k)
        y.append(v)

    xxx = np.array(x)
    yyy = np.array(y)

    z1 = np.polyfit(xxx, yyy, 3)
    p1 = np.poly1d(z1)

    print("p1=", p1)

    print("p1=", list(p1))

    resultstr = ""
    index = len(list(p1))-1
    for ele in list(p1):
        resultstr += str(ele) + " * x**"+str(index) + "+ "
        index = index-1
    print(resultstr)

    yvals = p1(xxx)

    plt.plot(xxx, yyy, '*', label='original values')
    plt.plot(xxx, yvals, 'r', label='polyfit values')
    plt.xlabel('x axis')
    plt.ylabel('y axis')
    plt.legend(loc=2)
    plt.title('polyfitting')
    plt.show()

def verify():
    print("\n verify Model prediction lr......")
    for x in range(1, 200, 1):
        if x in model_prediction_lr:
            v = model_prediction_lr[x]
        else:
            v = "NA"
        y7 = 0.0001027894491128785 * x**7+ 0.0012976062091488304 * x**6+ -0.1386430964051121 * x**5+ 2.5909637600534943 * x**4+ -22.571013043602264 * x**3+ 105.617636227275 * x**2+ -268.9089490557764 * x**1+ 349.16299999995675
        y8 = 0.0002442735890399334 * x ** 9 + -0.012182762895561037 * x ** 8 + 0.2603451057931334 * x ** 7 + -3.109628819123638 * x ** 6 + 22.668589465264354 * x ** 5 + -103.031392871345 * x ** 4 + 283.77762548815707 * x ** 3 + -425.09279548605645 * x ** 2 + 222.05119559215828 * x ** 1 + 168.23800001369372 * x ** 0
        y3 = -0.6298690753690737 * x**3+ 13.2699079254079 * x**2+ -91.70643512043496 * x**1+ 237.40973333333287 * x**0
        yInv = 11.067116307237646+ 140.39482190448214/x
        print(x, v, yInv)

    print("\n verify instance_weighting lr......")
    for x in range(1, 200, 1):
        if x in instance_weighting:
            v = instance_weighting[x]
        else:
            v = "NA"
        y7 = 0.0002070728291299603 * x**7+ 0.00039450163404392696 * x**6+ -0.19472933823604105 * x**5+ 3.9560234759976978 * x**4+ -36.14301356524817 * x**3+ 177.69921639096137 * x**2+ -480.47295707350673 * x**1+ 666.3431999999762 * x**0
        y8 = -0.00034992283946815044 * x**9+ 0.017642038688577152 * x**8+ -0.3809591600123898 * x**7+ 4.610705902295765 * x**6+ -34.40089143168662 * x**5+ 164.26152932434852 * x**4+ -506.44890697663277 * x**3+ 1001.0821226435963 * x**2+ -1249.131892396004 * x**1+ 951.590999979428 * x**0
        y3 = -1.2502097902097866 * x**3+ 26.38759032634027 * x**2+ -183.15656351981323 * x**1+ 475.65603333333246 * x**0
        yInv = 28.568537763734497+ 212.0739491077336/x
        print(x, v, yInv)

    print("\n verify feature_selection lr......")
    for x in range(1, 200, 1):
        if x in feature_selection:
            v = feature_selection[x]
        else:
            v = "NA"
        y7 = -0.004284564659195404 * x**7+ 0.17878303104562376 * x**6+ -3.1128454820229954 * x**5+ 29.377076172035274 * x**4+ -163.36474069862723 * x**3+ 544.7100882046798 * x**2+ -1056.7503272995432 * x**1+ 1122.4823999994287 * x**0
        y8 = -0.0002057457010372915 * x**9+ 0.00991185515768368 * x**8+ -0.20810719243794604 * x**7+ 2.516314583066748 * x**6+ -19.503388018895347 * x**5+ 101.80427811616404 * x**4+ -363.5988204446018 * x**3+ 875.7229953950146 * x**2+ -1350.2069785357016 * x**1+ 1226.9739999884878 * x**0
        y3 = -1.7678648018647984 * x**3+ 36.92449650349644 * x**2+ -251.96506293706256 * x**1+ 668.2397333333322 * x**0
        yInv = 24.929086085421833+ 30.762217008446868/x
        print(x, v, yInv)

    print("\n verify VFL_training lr......")
    for x in range(1, 200, 1):
        if x in VFL_training:
            v = VFL_training[x]
        else:
            v = "NA"
        y7 = -0.004706757703079375 * x**7+ 0.19551642156848478 * x**6+ -3.3853075980357454 * x**5+ 31.733587575375203 * x**4+ -175.05423925315745 * x**3+ 578.0518386184868 * x**2+ -1107.3595310462952 * x**1+ 1178.3799999993971 * x**0
        y8 = -0.00017606371250155833 * x**9+ 0.008769345237070247 * x**8+ -0.19177166003100726 * x**7+ 2.4284097219613416 * x**6+ -19.730690391622808 * x**5+ 107.31546179691485 * x**4+ -393.94771106464395 * x**3+ 956.7323590863967 * x**2+ -1457.2546507586367 * x**1+ 1307.1999999886762 * x**0
        y3 = -1.8160955710955657 * x**3+ 37.79739510489501 * x**2+ -256.63378205128146 * x**1+ 700.3369999999984 * x**0
        yInv = 43.48788216642094+ 30.19177917524453/x
        print(x, v, yInv)

print("\nModel prediction lr......")
# fitMultiple(model_prediction_lr)
fitInverse(model_prediction_lr)

print("\ninstance_weighting......")
# fitMultiple(instance_weighting)
fitInverse(instance_weighting)

print("\nfeature_selection......")
# fitMultiple(feature_selection)
fitInverse(feature_selection)

print("\nVFL_training......")
# fitMultiple(VFL_training)
fitInverse(VFL_training)

verify()
