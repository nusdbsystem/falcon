import os
from sklearn.tree import DecisionTreeClassifier, export_graphviz
import pandas as pd
from sklearn import preprocessing
from sklearn.pipeline import Pipeline
from sklearn.metrics import roc_auc_score , classification_report
from sklearn.pipeline import Pipeline
from sklearn.metrics import precision_score, recall_score, accuracy_score, classification_report

index=[]
board = ['a','b','c','d','e','f','g']
for i in board:
    for j in range(6):
        index.append(i + str(j+1))

column_names  = index +['Class']

# read .csv from provided dataset
csv_filename="connect-4.data"

# df=pd.read_csv(csv_filename,index_col=0)
df=pd.read_csv(csv_filename,
               names= column_names)

df['Class'].unique()

#Convert animal labels to numbers
le = preprocessing.LabelEncoder()
for col in df.columns:
    df[col] = le.fit_transform(df[col])

df.to_csv('/opt/falcon_baseline/data/connect4/connect-4.processed.data', index=False)
