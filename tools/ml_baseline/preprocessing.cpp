//
// Created by root on 15/6/22.
//

#include <iostream>
#include <cmath>

using namespace std;

float calculate_weighted_mean(float weight[], float x[], int num){
  float numerator = 0;
  float denominator = 0;
  for (int i = 0; i<num; i++ ){
    numerator += weight[i] * x[i];
    denominator += weight[i];
  }
  return numerator/denominator;
}

float calculate_weighted_variance( float weight[], float x[], float meanX, int num){
  float numerator = 0;
  float denominator = 0;
  for (int i = 0; i < num; i++ ){
    numerator += weight[i] * (x[i] - meanX) ;
    denominator += weight[i];
  }
  cout << "mean is = " << meanX << endl;
  cout << "numerator is = " << numerator << endl;
  return numerator/denominator;
}


float calculate_weight_covariance( float weight[],
                                   float x[], float meanX,
                                   float y[], float meanY, int num ){

  float numerator = 0;
  float denominator = 0;
  for (int i = 0; i < num; i++ ){
    numerator += weight[i] * ( x[i] - meanX ) * ( y[i] - meanY ) ;
    denominator += weight[i];
  }

  cout << "covariance = " << numerator/denominator << "\n" << endl;

  return numerator/denominator;
}

float calculate_weighted_pearson(float x[], float y[],
                                 float weight[], int num){

  float meanX = calculate_weighted_mean( weight, x, num );
  float meanY = calculate_weighted_mean( weight, y, num );

  cout << "mean of x and y is = " << meanX << ", " << meanY  << "\n" << endl;

  float Sxy = calculate_weight_covariance( weight, x, meanX, y, meanY, num );

  float Sx = calculate_weighted_variance( weight, x, meanX, num );
  float Sy = calculate_weighted_variance( weight, y, meanY, num );

  cout << "sx and sy are = " << Sx << ", " << Sy << "\n" << endl;
  cout << "Sxy is = " << Sxy << "\n" << endl;

  return Sxy / sqrt( Sx * Sy);
}


int main() {
  int num = 5;
  float x[] = {1, 2, 1, 3 ,4};
  float y[] = {1, 5, 1, 1, 1};
  float weight[] = {1, 1, 1, 1, 3} ;
  float res = calculate_weighted_pearson(x, y, weight, num);
  cout << "Standard Deviation = " << res << "\n" << endl;
  return 0;
}