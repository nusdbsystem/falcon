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
        # trading_dates = np.genfromtxt(
        #     os.path.join(self.data_path, '..',
        #                  self.market_name + '_aver_line_dates.csv'), # prepare a list of dates to exclude holidays
        #     dtype=str, delimiter=',', skip_header=False
        # )
        # print('#trading dates:', len(trading_dates))
        # # begin_date = datetime.strptime(trading_dates[29], self.date_format)
        # print('begin date:', begin_date)
        # # transform the trading dates into a dictionary with index
        # index_tra_dates = {}
        # tra_dates_index = {}
        # for index, date in enumerate(trading_dates): # make a dictionary to translate dates into index and vice versa
        #     tra_dates_index[date] = index
        #     index_tra_dates[index] = date
        # self.tickers = np.genfromtxt(
        #     os.path.join(self.data_path, '..', selected_tickers_fname),
        #     dtype=str, delimiter='\t', skip_header=False
        # )
        # # for debug purpose
        # # self.tickers = self.tickers[:10]
        # print('#tickers selected:', len(self.tickers))
        # self._read_EOD_data()
        # stocks_features = []
        # labels = []
        #
        # for stock_index, single_EOD in enumerate(self.data_EOD):
        #     begin_date_row = -1
        #     for date_index, daily_EOD in enumerate(single_EOD):
        #         date_str = daily_EOD[0].replace('-05:00', '')
        #         date_str = date_str.replace('-04:00', '')
        #         cur_date = datetime.strptime(date_str, self.date_format)
        #         if cur_date > begin_date: # from original raw stock price data file extract data, if larger than our specified data than
        #             begin_date_row = date_index
        #             break
        #     selected_EOD_str = single_EOD[begin_date_row:]
        #     selected_EOD = self._transfer_EOD_str(selected_EOD_str, # transfer this selected date string into index
        #                                           tra_dates_index)
        #
        #     # calculate moving average features
        #     begin_date_row = -1
        #     for row in selected_EOD[:, 0]:
        #         row = int(row)
        #         if row >= pad_begin:  # offset for the first 30-days average
        #             begin_date_row = row
        #             break
        #     mov_aver_features = np.zeros(
        #         [selected_EOD.shape[0], 4], dtype=float
        #     )  # 4 columns refers to 5-, 10-, 20-, 30-days average
        #
        #     for row in range(begin_date_row, selected_EOD.shape[0]):
        #         date_index = selected_EOD[row][0]
        #         aver_5 = 0.0
        #         aver_10 = 0.0
        #         aver_20 = 0.0
        #         aver_30 = 0.0
        #         count_5 = 0
        #         count_10 = 0
        #         count_20 = 0
        #         count_30 = 0
        #         std_5 = []
        #         std_10 = []
        #         std_20 = []
        #         std_30 = []
        #         for offset in range(30):
        #             date_gap = date_index - selected_EOD[row - offset][0]
        #             if date_gap < 5:
        #                 count_5 += 1
        #                 aver_5 += selected_EOD[row - offset][4]
        #                 std_5.append(selected_EOD[row - offset][4])
        #             if date_gap < 10:
        #                 count_10 += 1
        #                 aver_10 += selected_EOD[row - offset][4]
        #                 std_10.append(selected_EOD[row - offset][4])
        #             if date_gap < 20:
        #                 count_20 += 1
        #                 aver_20 += selected_EOD[row - offset][4]
        #                 std_20.append(selected_EOD[row - offset][4])
        #             if date_gap < 30:
        #                 count_30 += 1
        #                 aver_30 += selected_EOD[row - offset][4]
        #                 std_30.append(selected_EOD[row - offset][4])
        #         if count_5 == 0: # some data such as ticker DWAQ in period of 2016-12-06~21 missing data 15 days
        #             mov_aver_features[row][0] = -1234
        #         else:
        #             mov_aver_features[row][0] = aver_5 / count_5
        #         if count_10 == 0:
        #             mov_aver_features[row][1] = -1234
        #         else:
        #             mov_aver_features[row][1] = aver_10 / count_10
        #         mov_aver_features[row][2] = aver_20 / count_20
        #         mov_aver_features[row][3] = aver_30 / count_30
        #
        #     '''
        #         normalize features by feature / max, the max price is the
        #         max of close prices, I give up to subtract min for easier
        #         return ratio calculation.
        #     '''
        #     pri_min = np.min(selected_EOD[begin_date_row:, 4])
        #     price_max = np.max(selected_EOD[begin_date_row:, 4])
        #
        #     print(self.tickers[stock_index], 'minimum:', pri_min,
        #           'maximum:', price_max, 'ratio:', price_max / pri_min)
        #     if price_max / pri_min > 10:
        #         print('!!!!!!!!!')
        #     '''
        #         generate feature and ground truth in the following format:
        #         date_index, 5-day, 10-day, 20-day, 30-day, close price
        #         two ways to pad missing dates:
        #         for dates without record, pad a row [date_index, 0 * 5]
        #
        #         James: delete the part with /price_max for close price
        #         deal missing value with spline since in automl-zero masking and deleting weights later in loss function
        #         do not work
        #     '''
        #     features = np.ones([len(trading_dates) - pad_begin, 6],
        #                        dtype=float) * -1234
        #     rows = np.ones([len(trading_dates) - pad_begin, 1],
        #                    dtype=float)
        #     # data missed at the beginning
        #     for row in range(len(trading_dates) - pad_begin):
        #         rows[row][0] = row
        #     for row in range(begin_date_row, selected_EOD.shape[0]):
        #         cur_index = int(selected_EOD[row][0])
        #
        #         '''adding the next if condition because of the above mentioned prob - index might not be continuous.
        #             Only if continuous will add features'''
        #         if cur_index - int(selected_EOD[row - return_days][0]) == \
        #                 return_days:
        #             features[cur_index - pad_begin][0:5] = \
        #                 selected_EOD[row][1:]
        #
        #             if (row + return_days) < selected_EOD.shape[0]:
        #                 if selected_EOD[row + return_days][-2] > selected_EOD[row][-2]:
        #                     if selected_EOD[row][-2] == 0:
        #                         features[cur_index - pad_begin][-1] = -1234
        #                     else:
        #                         features[cur_index - pad_begin][-1] = (selected_EOD[row + return_days][-2] - selected_EOD[row][-2])/selected_EOD[row][-2]
        #                 else:
        #                     if selected_EOD[row][-2] == 0:
        #                         features[cur_index - pad_begin][-1] = -1234
        #                     else:
        #                         features[cur_index - pad_begin][-1] = (selected_EOD[row + return_days][-2] - selected_EOD[row][-2])/selected_EOD[row][-2]
        #
        #     '''volume number is big so log before normalize'''
        #     features[:, -2][features[:, -2] > 0] = np.log(features[:, -2][features[:, -2] > 0])
        #
        #     # last row all missing as show in feng's github
        #     features = np.delete(features, -1, 0)
        #     # because the last row don't have label since no tomorrow data
        #     features = np.delete(features, -1, 0)
        #
        #     '''note that in load_data.py by feng he also deleted last row but it's only for NASDAQ since only NASDAQ has
        #     a lot of missing one at the last day. Be careful when use NYSE data'''
        #     labels.append(features[:, -1:])
        #
        #     # after pass the last column to label, delete that column
        #     features = np.delete(features, 5, 1)
        #
        #     '''normalize each part features by respective min max'''
        #     ''' James: this might wrong big!!! because only normalize by each individual stock !!!'''
        #     max_num = np.max(features[:, :4][features[:, :4] != -1234])
        #     min_num = np.min(features[:, :4][features[:, :4] != -1234])
        #     # max_num2 = np.max(features[:, 4:8][features[:, 4:8] != -1234])
        #     # min_num2 = np.min(features[:, 4:8][features[:, 4:8] != -1234])
        #     max_volume = np.max(features[:, 4][features[:, 4] != -1234])
        #     min_volume = np.min(features[:, 4][features[:, 4] != -1234])
        #     # max_num = np.maximum(max_num2, max_num)
        #     # min_num = np.minimum(min_num2, min_num)
        #     for i in range(np.shape(features)[1]):
        #         not_mask_column = features[:, i][features[:, i] != -1234]
        #         if i in [0, 1, 2, 3]:
        #             features[:, i][features[:, i] != -1234] = (not_mask_column - min_num)/(max_num - min_num)
        #         elif i in [4]:
        #             features[:, i][features[:, i] != -1234] = (not_mask_column - min_volume) / (max_volume - min_volume)
        #     stocks_features.append(features)
        #
        # labels = np.vstack(labels)
        # stocks_features = np.vstack(stocks_features)
        #
        # labels = labels[(stocks_features != -1234).all(axis=1)]
        # stocks_features = stocks_features[(stocks_features != -1234).all(axis=1)]
        # stocks_features = stocks_features[(labels != -1234).all(axis=1)]
        # labels = labels[(labels != -1234).all(axis=1)]
        # labels = labels.ravel()
        #
        # print((stocks_features == -1234).any())
        # print((labels == -1234).any())
        #
        # stocks_features = np.concatenate((
        #                                              stocks_features[:-4], stocks_features[1:-3], stocks_features[2:-2],
        #                                              stocks_features[3:-1], stocks_features[4:]), axis=1)
        # labels = labels[4:]
        '''The above is the dataset generation. Commented off the above to only load the datasets.'''
        # only_mv is the version without high low close open volume
        # stocks_features = np.load('stocks_features_for_FL_only_mv.npy')
        # labels = np.load('labels_for_FL_only_mv.npy')

        # only_mv_v2 is the version without mv only have high low close open volume normalized
        # stocks_features = np.load('stocks_features_for_FL_only_mv_v2.npy')
        # labels = np.load('labels_for_FL_only_mv_v2.npy')

        # only_mv_v2 is the version without mv only have high low close open volume not normalized
        # stocks_features = np.load('stocks_features_for_FL_without_mv_no_normalized.npy')
        # labels = np.load('labels_for_FL_without_mv_no_normalized.npy')

        '''The dataset version with current best performance'''
        # stocks_features_for_FL_without_mv_no_normalized_regression_label.npy
        # stocks_features = np.load('stocks_features_for_FL_without_mv_no_normalized_regression_label.npy')
        # labels = np.load('labels_for_FL_without_mv_no_normalized_regression_label.npy')

        # features with concatenated vectors
        # stocks_features = np.load('stocks_features_for_FL_without_mv_no_normalized_regression_label_concatenate.npy')
        # labels = np.load('labels_for_FL_without_mv_no_normalized_regression_label_concatenate.npy')

        # features with concatenated vectors for NYSE normalized
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
