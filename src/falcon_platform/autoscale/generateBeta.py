import math

import numpy as np
import matplotlib.pyplot as plt

mapper = dict()
mapper[1] = 1
mapper[2] = 0.7
mapper[3] = 0.5
mapper[4] = 0.4

def verify(x):
    y = -2.94587705*math.pow(10, -8) * x ** 7 + 2.50848776*math.pow(10, -6) * x ** 6 - 8.78441842 *math.pow(10, -5) * x**5 + \
    0.00160672440 * x ** 4 - 0.01605 * x ** 3 + 0.08281 * x ** 2 - 0.2161 * x + 1.14976780
    return y

for k, v in mapper.items():
    print(k, v, verify(k))


x = []
y = []
for k, v in mapper.items():
    x.append(k)
    y.append(v)

xxx = np.array(x)
yyy = np.array(y)


z1 = np.polyfit(xxx, yyy, 7)
p1 = np.poly1d(z1)
print(z1, p1)

yvals=p1(xxx)

plt.plot(xxx, yyy, '*',label='original values')
plt.plot(xxx, yvals, 'r',label='polyfit values')
plt.xlabel('x axis')
plt.ylabel('y axis')
plt.legend(loc=4)
plt.title('polyfitting')
plt.show()