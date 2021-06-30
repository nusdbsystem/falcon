from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl import app
import argparse
from datetime import datetime
import numpy as np
import os
import torch

from sklearn.model_selection import train_test_split

class EOD_Preprocessor:
    def __init__(self, data_path, market_name):
        self.data_path = data_path
        self.date_format = '%Y-%m-%d %H:%M:%S'
        self.market_name = market_name

    def _read_EOD_data(self):
        self.data_EOD = []
        for index, ticker in enumerate(self.tickers):
            single_EOD = np.genfromtxt(
                os.path.join(self.data_path, self.market_name + '_' + ticker +
                             '_30Y.csv'), dtype=str, delimiter=',',
                skip_header=True
            )
            self.data_EOD.append(single_EOD)
            # if index > 99:
            #     break
        print('#stocks\' EOD data readin:', len(self.data_EOD))
        assert len(self.tickers) == len(self.data_EOD), 'length of tickers ' \
                                                        'and stocks not match'

    def _read_tickers(self, ticker_fname):
        self.tickers = np.genfromtxt(ticker_fname, dtype=str, delimiter='\t',
                                     skip_header=True)[:, 0]

    def _transfer_EOD_str(self, selected_EOD_str, tra_date_index):
        selected_EOD = np.zeros(selected_EOD_str.shape, dtype=float)
        for row, daily_EOD in enumerate(selected_EOD_str):
            date_str = daily_EOD[0].replace('-05:00', '')
            date_str = date_str.replace('-04:00', '')
            selected_EOD[row][0] = tra_date_index[date_str]
            '''what if data_str doesn't have one date in tra_date_index causing the index not continuous???
            so tra_date_index must have all dates for all tickers, but some ticker might not have one date in 
            tra_date_index'''
            for col in range(1, selected_EOD_str.shape[1]):
                selected_EOD[row][col] = float(daily_EOD[col])
        return selected_EOD

    '''
        Transform the original EOD data collected from Google Finance to a
        friendly format to fit machine learning model via the following steps:
            Calculate moving average (5-days, 10-days, 20-days, 30-days),
            ignoring suspension days (market open, only suspend this stock)
            Normalize features by (feature - min) / (max - min)
    '''

    def generate_feature(self, selected_tickers_fname, begin_date, opath,
                         return_days=1, pad_begin=29):

        stocks_features = np.load('stocks_features_for_FL_without_mv_normalized_regression_label_concatenate_NYSE.npy')
        labels = np.load('labels_for_FL_without_mv_normalized_regression_label_concatenate_NYSE.npy')

        # np.save('stocks_features_for_FL_without_mv_normalized_regression_label_concatenate_NYSE.npy', stocks_features)
        # np.save('labels_for_FL_without_mv_normalized_regression_label_concatenate_NYSE.npy', labels)

        stocks_features_train, stocks_features_test, labels_train, labels_test = train_test_split(stocks_features, labels, test_size=0.25,
                                                            random_state=50, shuffle=True)
        stocks_features_two_three, stocks_features_one, labels_two_three, labels_one = train_test_split(stocks_features_train, labels_train, test_size=0.33,
                                                            random_state=50, shuffle=True)
        stocks_features_two, stocks_features_three, labels_two, labels_three = train_test_split(stocks_features_two_three, labels_two_three, test_size=0.5,
                                                            random_state=50, shuffle=True)

        stocks_features_train = torch.Tensor(stocks_features_train)
        stocks_features_test = torch.Tensor(stocks_features_test)
        labels_train = torch.Tensor(labels_train)
        labels_test = torch.Tensor(labels_test)
        labels_one = torch.Tensor(labels_one)
        labels_two = torch.Tensor(labels_two)
        labels_three = torch.Tensor(labels_three)
        stocks_features_one = torch.Tensor(stocks_features_one)
        stocks_features_two = torch.Tensor(stocks_features_two)
        stocks_features_three = torch.Tensor(stocks_features_three)

        # the first model with first part of data
        model = Feedforward(25, 200)
        criterion = torch.nn.MSELoss()
        optimizer = torch.optim.Adam(model.parameters(), lr=0.001)

        model.eval()
        y_pred = model(stocks_features_test)
        before_train = criterion(y_pred.squeeze(), labels_test)
        print('Test loss before training', before_train.item())

        model.train()
        epoch = 100
        for epoch in range(epoch):
            optimizer.zero_grad()
            # Forward pass
            y_pred = model(stocks_features_one)
            # Compute Loss
            loss = criterion(y_pred.squeeze(), labels_one)

            # print('Model one epoch {}: train loss: {}'.format(epoch, loss.item()))
            # Backward pass
            loss.backward()
            optimizer.step()

        model.eval()
        y_pred = model(stocks_features_test)
        after_train = criterion(y_pred.squeeze(), labels_test)
        print('Model one test loss after Training', after_train.item())

        # the second model with the second part of data
        model2 = Feedforward(25, 200)
        optimizer2 = torch.optim.Adam(model2.parameters(), lr=0.001)

        model2.train()
        for epoch in range(epoch):
            optimizer2.zero_grad()
            # Forward pass
            y_pred = model2(stocks_features_two)
            # Compute Loss
            loss = criterion(y_pred.squeeze(), labels_two)

            # print('Model two epoch {}: train loss: {}'.format(epoch, loss.item()))
            # Backward pass
            loss.backward()
            optimizer2.step()

        model2.eval()
        y_pred = model2(stocks_features_test)
        after_train = criterion(y_pred.squeeze(), labels_test)
        print('Model two test loss after Training', after_train.item())

        # the third model with the third part of data
        model3 = Feedforward(25, 200)
        optimizer3 = torch.optim.Adam(model3.parameters(), lr=0.001)

        model3.train()
        for epoch in range(epoch):
            optimizer3.zero_grad()
            # Forward pass
            y_pred = model3(stocks_features_three)
            # Compute Loss
            loss = criterion(y_pred.squeeze(), labels_three)

            # print('Model three epoch {}: train loss: {}'.format(epoch, loss.item()))
            # Backward pass
            loss.backward()
            optimizer3.step()

        model3.eval()
        y_pred = model3(stocks_features_test)
        after_train = criterion(y_pred.squeeze(), labels_test)
        print('Model three test loss after Training', after_train.item())

        # the fourth model with all training data
        model4 = Feedforward(25, 200)
        optimizer4 = torch.optim.Adam(model4.parameters(), lr=0.001)

        model4.train()
        for epoch in range(epoch):
            optimizer4.zero_grad()
            # Forward pass
            y_pred = model4(stocks_features_train)
            # Compute Loss
            loss = criterion(y_pred.squeeze(), labels_train)

            # print('Model four epoch {}: train loss: {}'.format(epoch, loss.item()))
            # Backward pass
            loss.backward()
            optimizer4.step()

        model4.eval()
        y_pred = model4(stocks_features_test)
        after_train = criterion(y_pred.squeeze(), labels_test)
        print('Model four test loss after Training', after_train.item())

class Feedforward(torch.nn.Module):
    def __init__(self, input_size, hidden_size):
        super(Feedforward, self).__init__()
        self.input_size = input_size
        self.hidden_size = hidden_size
        self.fc1 = torch.nn.Linear(self.input_size, self.hidden_size)
        self.fc3 = torch.nn.Linear(self.hidden_size, self.hidden_size)
        self.relu = torch.nn.ReLU()
        self.fc2 = torch.nn.Linear(self.hidden_size, 1)
        self.sigmoid = torch.nn.Sigmoid()

    def forward(self, x):
        hidden = self.fc1(x)
        relu = self.relu(hidden)
        hidden2 = self.fc3(relu)
        relu2 = self.relu(hidden2)
        output = self.fc2(relu2)
        output = self.sigmoid(output)
        return output

def main(unused_argv):
  """Create the datasets and run the model."""
  del unused_argv
  desc = "First, we pre-process EOD data market by market, including listing all " \
         "trading days, all satisfied stocks (5 years & high price), " \
         "normalizing and compansating data. Second, we run the MLP regressor model."
  parser = argparse.ArgumentParser(description=desc)
  parser.add_argument('-path', help='path of EOD data')
  parser.add_argument('-market', help='market name')
  args = parser.parse_args()

  if args.path is None:
      args.path = 'data/google_finance'
  if args.market is None:
      args.market = 'NYSE'

  processor = EOD_Preprocessor(args.path, args.market)

  processor.generate_feature(
      processor.market_name + '_tickers_qualify_dr-0.98_min-5_smooth.csv',
      datetime.strptime('2012-11-19 00:00:00', processor.date_format),
      os.path.join(processor.data_path, '..', '2013-01-01'), return_days=1,
      pad_begin=29
  )

if __name__ == '__main__':
  app.run(main)
