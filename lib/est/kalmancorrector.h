#ifndef GCOP_KALMANCORRECTOR_H
#define GCOP_KALMANCORRECTOR_H

#include "corrector.h"

namespace gcop {
  
  template <typename T = VectorXd, 
    int _nx = Dynamic, 
    int _nu = Dynamic, 
    int _np = Dynamic, 
    typename Tz = VectorXd,
    int _nz = Dynamic> class KalmanCorrector : public Corrector<T, _nx, _nu, _np, Tz, _nz> {
  public:
  
  typedef Matrix<double, _nx, 1> Vectornd;
  typedef Matrix<double, _nu, 1> Vectorcd;
  typedef Matrix<double, _np, 1> Vectormd;

  typedef Matrix<double, _nx, _nx> Matrixnd;
  typedef Matrix<double, _nx, _nu> Matrixncd;
  typedef Matrix<double, _nx, _nz> Matrixnrd;
  typedef Matrix<double, _nu, _nx> Matrixcnd;
  typedef Matrix<double, _nu, _nu> Matrixcd;
  
  typedef Matrix<double, _np, _np> Matrixmd;
  typedef Matrix<double, _nx, _np> Matrixnmd;
  typedef Matrix<double, _np, _nx> Matrixmnd;

  typedef Matrix<double, _nz, 1> Vectorrd;
  typedef Matrix<double, _nz, _nz> Matrixrd;
  typedef Matrix<double, _nz, _nx> Matrixrnd;
  typedef Matrix<double, _nz, _nu> Matrixrcd;
  typedef Matrix<double, _nz, _np> Matrixrmd;

  KalmanCorrector(System<T, _nx, _nu, _np> &sys,
                  Sensor<T, _nx, _nu, _np, Tz, _nz> &sensor);
  
  virtual ~KalmanCorrector();
  
  virtual bool Correct(T& xb, double t, const T &xa, 
                       const Vectorcd &u, const Tz &z, 
                       const Vectormd *p = 0, bool cov = true);    

  Tz y;             ///< expected measurement
  Vectornd dx;      ///< innovation

  Matrixrnd H;      ///< measurement Jacobian
  Matrixnrd K;      ///< correction gain matrix
  
  };
  
  
  template <typename T, int _nx, int _nu, int _np, typename Tz, int _nz> 
    KalmanCorrector<T, _nx, _nu, _np, Tz, _nz>::KalmanCorrector(System<T, _nx, _nu, _np>  &sys, 
                                                                Sensor<T, _nx, _nu, _np, Tz, _nz> &sensor) : 
    Corrector<T, _nx, _nu, _np, Tz, _nz>(sys, sensor) {

    if (_nz == Dynamic) {
      y.resize(sensor.Z.n);
    }
    if (_nx == Dynamic) {
      dx.resize(sys.X.n);
    }
    if (_nx == Dynamic || _nz == Dynamic) {
      K.resize(sys.X.n, sensor.Z.n);
      H.resize(sensor.Z.n, sys.X.n);
    }

    H.setZero();
  }
  
  template <typename T, int _nx, int _nu, int _np, typename Tz, int _nz> 
    KalmanCorrector<T, _nx, _nu, _np, Tz, _nz>::~KalmanCorrector() {
  }  
  
  
  template <typename T, int _nx, int _nu, int _np, typename Tz, int _nz> 
    bool KalmanCorrector<T, _nx, _nu, _np, Tz, _nz>::Correct(T& xb, double t, const T &xa, 
                                                             const Vectorcd &u, const Tz &z, 
                                                             const Vectormd *p, bool cov) {
    sensor(y, t, xa, u, p, &H);
    
    //    std::cout << "y=" << y << std::endl;
    //    std::cout << "Pa=" << Pa << std::endl;    
    //    std::cout << "S=" << H*Pa*H.transpose() + R << std::endl;
    
    K = xa.P*H.transpose()*(H*xa.P*H.transpose() + this->sensor.R).inverse();
    
    
    //    std::cout <<"K=" << K<<std::endl;
    if (cov)
      xb.P = (MatrixXd::Identity(this->sys.X.n, this->sys.X.n) - K*H)*xa.P;
    
    //    std::cout << "K=" << K << std::endl;            

    //    %x = x*S.exp(K*(z - y));    
    //%K = inv(inv(P) + H'*inv(S.R)*H)*H'*inv(S.R);
    //%P = (eye(length(x)) - K*H)*P*(eye(length(x)) - K*H)' + K*S.R*K';

    dx = K*(z - y);
    this->sys.X.Retract(xb, xa, dx);
  }
}


#endif
