#include <iomanip>
#include <iostream>
#include "body2dtrackcost.h"
#include "pdmoc.h"
#include "utils.h"
#include "se2.h"
#include "viewer.h"
#include "body2dview.h"
#include "body2dcost.h"
#include "params.h"
#include "body2dtrackview.h"

using namespace std;
using namespace Eigen;
using namespace gcop;


typedef Dmoc<pair<Matrix3d, Vector3d>, 6, 3> Body2dDmoc;

Params params;

void Run(Viewer* viewer)
{
  if (viewer)
    viewer->SetCamera(18.875, 1.625, -.6, -.2, -8.5);

  double tf = 10;
  int nf = 200;
  params.GetDouble("tf", tf);
  params.GetInt("nf", nf);

  // control horizon params
  int iters = 30;
  double Tc = 2; // horizon
  int N; // optimal control segments
  params.GetInt("iters", iters);
  params.GetDouble("Tc", Tc);
  params.GetInt("N", N);
  double h = Tc/N;

  Body2d sys(new Body2dForce(true));
  sys.force->D << .01, .01, 3; // add damping

  // options: odometry, paramForce, forces
  Body2dTrack pg(sys, nf, 0, tf, true, false, true);   ///< ground truth

  M3V3d x0;
  pg.Get(x0, 5, 0);

  params.GetDouble("r", pg.r);
  params.GetDouble("w", pg.w);

  pg.MakeTrue();

  //  Body2dTrack pg(sys, N, nf, 0, tf, false, false, true);    ///< noisy pose track
  //  Body2dTrack::Synthesize3(pgt, pg, tf);

  Body2dView tview(sys, &pg.xs);   // true path
  tview.lineWidth = 5;
  tview.rgba[0] = 0;
  tview.rgba[1] = 1;
  tview.rgba[2] = 0;
  tview.renderSystem = false;

  Body2dView oview(sys, &pg.xos);   // odometry
  oview.lineWidth = 5;
  oview.renderSystem = false;
  oview.rgba[0] = 1;
  oview.rgba[1] = 0;
  oview.rgba[2] = 0;

  Body2dTrackView pgv(pg);               // optimized pose-track
  pgv.rgba[0] = 1; pgv.rgba[1] = 1; pgv.rgba[2] = 1; 

  //  pgv.drawLandmarks = false;

  //  pgv.drawForces = false;
  pgv.forceScale=.1;


  if (viewer) {
    viewer->Add(tview);
    viewer->Add(oview);
    viewer->Add(pgv);
  }

  struct timeval timer;
  getchar();
  

  SE2 &se2 = SE2::Instance();  
  M3V3d xf;
  pg.Get(xf, 5, Tc);

  // cost
  Body2dCost cost(Tc, xf);
  //  cost.track = &pg;
  params.GetDouble("ko", cost.ko);


  VectorXd Q(6);
  if (params.GetVectorXd("Q", Q))
    cost.Q = Q.asDiagonal();
  
  VectorXd Qf(6);
  if (params.GetVectorXd("Qf", Qf))
    cost.Qf = Qf.asDiagonal();
  
  VectorXd R(3);
  if (params.GetVectorXd("R", R)) 
    cost.R = R.asDiagonal();

  // times
  vector<double> ts(N+1);
  for (int k = 0; k <=N; ++k)
    ts[k] = k*h;

  // states
  vector<pair<Matrix3d, Vector3d> > xs(N+1, pg.xs[0]);

  // controls
  Vector3d u(0,0,0);
  vector<Vector3d> us(N, u);

  // past trajectory
  
  vector<double> tps(1,0);
  vector<pair<Matrix3d, Vector3d> > xps(1, x0);
  vector<Vector3d> ups;

  Body2dDmoc dmoc(sys, cost, ts, xs, us);
  dmoc.mu = .01;
  params.GetDouble("mu", dmoc.mu);

  Body2dView cview(sys, &dmoc.xs);
  cview.rgba[0] = 0;  cview.rgba[1] = 1;  cview.rgba[2] = 1;
  viewer->Add(cview);
  cview.renderSystem = false;

  Body2dView pview(sys, &xps);
  viewer->Add(pview);
  pview.rgba[0] = 1;  pview.rgba[1] = 1;  pview.rgba[2] = 0;
  pview.renderSystem = false;

  Body2dTrackCost tcost(0, pg);  ///< cost function
  PDmoc<M3V3d, 6, 3> *pdmoc = 0;


  for (double t=0; t < tf; t+=h) {

    pg.Get(xf, 5, t+Tc);
    cost.tf = t + Tc;

    for (int j = 0; j < iters; ++j) {      
      timer_start(timer);
      dmoc.Iterate();
      long te = timer_us(timer);      
      cout << "Iteration #" << j << " took: " << te << " us." << endl;    
    }
    
    pg.Add(us[0], xs[1], h);

    // shift control trajectory forward
    for (int k = 0; k < N; ++k) {
      ts[k] = ts[k+1];
      if (k < N - 1)
        us[k] = us[k+1];
    }
    ts[N] = ts[N] + h;
    xs[0] = xs[1];

    tcost.tf = t+h;

    if (t > 10*h) {
      cout << "ts " << pg.ts.size() << endl;
      cout << "xs " << pg.xs.size() << endl;
      cout << "us " << pg.us.size() << endl;
      cout << "p " << pg.p.size() << endl;


      pdmoc = new PDmoc<M3V3d, 6, 3>(pg.sys, tcost, pg.ts, pg.xs, pg.us, pg.p, 2*pg.extforce);
      for (int b=0;b<30;++b)
        pdmoc->Iterate();
      
      delete pdmoc;
    }

    tps.push_back(ts[0]);
    xps.push_back(xs[0]);
    ups.push_back(us[0]);

    cout << "FEATURES:" << pg.p.size() << endl;
    
    //    getchar();  
    viewer->saveSnapshot=true;
  }


  /*
  Body2dSlam ba(pg);
  ba.pdmoc->debug = true; // turn off debug for speed
  ba.pdmoc->mu = .01;
  ba.pdmoc->nu = .01;

  for (int i = 0; i < 1000; ++i) {

    cout << "Press Enter to continue" << endl;
    getchar();    
    
    timer_start(timer);
    ba.pdmoc->Iterate();
    long te = timer_us(timer);
    cout << ba.pdmoc->dus[0] << endl;
    cout << "Iteration #" << i << " took: " << te << " us." << endl;    
    cout << "p=" << ba.pdmoc->p.head<2>().transpose() << endl;    
    cout << "ut=" << pgt.us[2] << endl;    
    cout << "u=" << pg.us[2] << endl;    

  }
  */
  cout << "done!" << endl;
  while(1)
    usleep(10);    
}


#define DISP

int main(int argc, char** argv)
{

  if (argc > 1)
    params.Load(argv[1]);
  else
    params.Load("../../bin/body2dtrack.cfg");

#ifdef DISP
  Viewer *viewer = new Viewer;
  viewer->Init(&argc, argv);
  viewer->frameName = "../../logs/body2dtrack/frames/body2d";
  viewer->displayName = "../../logs/body2dtrack/display/body2d";

  pthread_t dummy;
  pthread_create( &dummy, NULL, (void *(*) (void *)) Run, viewer);

#else
  Run(0);
#endif


#ifdef DISP
  viewer->Start();
#endif


  return 0;
}
