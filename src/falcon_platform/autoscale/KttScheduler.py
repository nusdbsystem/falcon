import numpy as np
from scipy.optimize import minimize

def objective(x):
    return 1 + x[0] + x[1] + x[2] * (x[3] + x[4])

def constraint1(x):
    return 70 - (1 + x[0] + x[1] + x[2] * (x[3] + x[4]))

def constraint2(x):
    return 80 - (60/x[0] + 30/x[1] + 6/x[2] * (80/x[3] + 150/ x[4]) )

# initial guesses
n = 5
x0 = np.zeros(n)
x0[0] = 1.0
x0[1] = 1.0
x0[2] = 1.0
x0[3] = 1.0
x0[4] = 1.0

# show initial objective
print('Initial SSE Objective: ' + str(objective(x0)))

# optimize
b = (1.0, 66)
c = (1.0, 6.0)
bnds = (b, b, c, b, b)
con1 = {'type': 'ineq', 'fun': constraint1}
con2 = {'type': 'eq', 'fun': constraint2}
cons = ([con1, con2])
solution = minimize(objective, x0, method='SLSQP', \
                    bounds=bnds, constraints=cons)
x = solution.x

# show final objective
print('Final SSE Objective: ' + str(objective(x)))

# print solution
print('Solution')
a = str(x[0])
print('x2 = ' + str(x[0]))
print('x3 = ' + str(x[1]))
print('xc = ' + str(x[2]))
print('x4 = ' + str(x[3]))
print('x5 = ' + str(x[4]))

