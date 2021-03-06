#include <iomanip>
#include <iostream>
#include "ddp.h"
#include "viewer.h"
#include "unicycleview.h"
#include "utils.h"
#include "rnlqcost.h"

using namespace std;
using namespace Eigen;
using namespace gcop;

typedef Ddp<Vector5d, 5, 2> UnicycleDdp;

void solver_process(Viewer* viewer)
{

  viewer->SetCamera(39, 45, 1.7, 2.25, -7.95);

  int N = 32;
  double tf = 10;
  double h = tf/N;

  Unicycle sys;

  Vector5d xf = Vector5d::Zero();
  RnLqCost<5, 2> cost(sys, tf, xf);
  cost.Q(0,0) = .1; cost.Q(1,1) = .1; cost.Q(2,2) = .1; cost.Q(3,3) = 1; cost.Q(4,4) = 1;
  cost.Qf(0,0) = 5; cost.Qf(1,1) = 5; cost.Qf(2,2) = 1; cost.Qf(3,3) = 1; cost.Qf(4,4) = 1;  
  cost.R(0,0) = .1; cost.R(1,1) = .05;  

  // times
  vector<double> ts(N+1);
  for (int k = 0; k <=N; ++k)
    ts[k] = k*h;

  // states
  vector<Vector5d> xs(N+1);
  xs[0][0] = -5; 
  xs[0][1] = -2;
  xs[0][2] = -3.;
  xs[0][3] = 0;
  xs[0][4] = 0;

  //controls
  vector<Vector2d> us(N);
  for (int i = 0; i < N/2; ++i) {
    us[i] = Vector2d(.02, 0.01);
    us[N/2+i] = Vector2d(-.02, 0.01);
  }
  

  UnicycleDdp ddp(sys, cost, ts, xs, us);  
  ddp.mu = 1;

  UnicycleView view(sys, &ddp.xs);
  viewer->Add(view);

  struct timeval timer;
  // ddp.debug = false; // turn off debug for speed
  //getchar();

  for (int i = 0; i < 20; ++i) {

    timer_start(timer);
    ddp.Iterate();
    long te = timer_us(timer);
    cout << "Iteration #" << i << " took: " << te << " us." << endl;
    //getchar();
  }

  //  for (int k = 0; k <= N; ++k)
  //    cout << ddp.xs[k] << "|" << endl;  
  //  cout << "xf=" << ddp.xs.back() << endl;  
  
  cout << "done!" << endl;
  while(1)
    usleep(10);    
}


#define DISP

int main(int argc, char** argv)
{

#ifdef DISP
  Viewer *viewer = new Viewer;
  viewer->Init(&argc, argv);
  viewer->frameName = "videos/sys";

  pthread_t dummy;
  pthread_create( &dummy, NULL, (void *(*) (void *)) solver_process, viewer);

#else
  solver_process(0);
#endif


#ifdef DISP
  viewer->Start();
#endif


  return 0;
}
