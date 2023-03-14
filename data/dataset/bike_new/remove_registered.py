

import pandas as pd

df = pd.read_csv('hour.csv')

df = df.iloc[:, 2:]

df.to_csv('hour_wo_index.csv', index=False)