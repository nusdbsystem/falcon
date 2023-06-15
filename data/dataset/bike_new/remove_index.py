

import pandas as pd

df = pd.read_csv('hour_wo_index.csv.norm')

df = df.drop([-2], axis=1)

df.to_csv('/opt/falcon/data/dataset/Bike-Sharing-Dataset/hour_wo_index_wo_registered.csv.norm', index=False)