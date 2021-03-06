#ifndef Vehicle_H
#define Vehicle_H

#include <vector>
// #include <map>
#include <iostream>
// #include <algorithm>
#include <math.h>
#include "Eigen-3.3/Eigen/Core"
// #include "Trajectory.h"
// #include "RoadMap.h"

#include "helpers.h"



using namespace Eigen;
// using namespace std;

using std::vector;
using std::cout;
using std::endl;

class Vehicle {
private:
  int ID;
  double Length;
  double Width;

public:
  State state;
  vector<Trajectory> trajectory;
  double t0; // Time at which the state and trajectory are defined relative t0.
  double t0_; // time of last update.
  // Constructor, Destructor
  Vehicle() : Vehicle(0, 4.8, 1.8) {};
  Vehicle(int id) : Vehicle(id, 4.8, 1.8) {};
  Vehicle(double L, double W) : Vehicle(0, L, W) {};
  Vehicle(int id, double L, double W) : ID(id), Length(L), Width(W), t0(0), state(State()), trajectory(state.get_trajectory()), t0_(-1) {};

  void display() const {
    cout << "Vehicle (ID : " << ID << ")" << endl;
    cout << " Length = " << Length << endl;
    cout << " Width = " << Width << endl;
    cout << " State, " << endl;
    state.display();
    cout << " Trajectory x(t), ";
    trajectory[0].display();
    cout << " Trajectory y(t), ";
    trajectory[1].display();
  }

  // Set the vehicle state and update the coresponding trajectory
  void set_state(State s0) {
    state = s0;
    trajectory = state.get_trajectory();
  }

  // Set the vehicle trajectory and update the corresponding state
  void set_trajectory(vector<Trajectory> traj) {
    trajectory = traj;
    state = State(traj[0].state_at(0), traj[1].state_at(0));
  }

  // Set vehicle size
  void set_size(double length, double width) {
    Length = length;
    Width = width;
  }
  // Get vehicle size
  double get_length() const {return Length;}
  double get_width() const {return Width;}

  // Get vehicle size
  Vector2d get_size() const {
    return (Vector2d()<<Length,Width).finished();
  }

  /* Add new measurment of state. Should be kalman filter or something, but
    simply estimates acceleration given the current and the last measurement
    (if not too outdated)
    measured state contains {x,y,vx,vy} - in Frenet coordinates! */
  void new_measurment(vector<double> m, double t1) {
    double dt = (t1 - t0_);
    Vector3d x, y;

    if (dt > 0.5) {
      // If the previous state_xy is very old, then reinitialize the state
      x << m[0], m[2], 0;
      y << m[1], m[3], 0;
    } else {
      // update state_xy acceleration
      if (fabs(m[0] - state.x(0)) > 5 || fabs(m[1] - state.y(0)) > 5) {
        cout << " ====" << endl;
        cout << " ====" << endl;
        cout << " ====  ERROR  :  car jumped by  d = " << sqrt(pow(m[0] - state.x(0),2) + pow(m[1]-state.y(0),2)) << endl;
        cout << " ====" << endl;
        cout << " ====" << endl;
      }
      x << m[0], m[2], (m[2] - state.x(1))/dt;
      y << m[1], m[3], (m[3] - state.y(1))/dt;
    }

    set_state(State(x,y)); // Set the state
    t0 = 0; // Update the time.
    t0_ = t1;
  }

  // Return the cars bounding box at time t.
  Rectangle bounding_box(double t, Vector2d safty_margin = Vector2d::Zero()) const {
    auto loc = location(t);
    return Rectangle(Length+safty_margin(0),
                     Width+safty_margin(1),
                     orientation(t),
                     loc[0],
                     loc[1]);
  };

  template <typename T>
  Rectangle bounding_box(double t, const T& loc, Vector2d safty_margin = Vector2d::Zero()) const {
    // This overloaded function is useful in trajecotry generation since we
    // already have the locations computed there, before calling bounding_box
    // to check collisions. (The "const" allows us to send a row of an eigen
    // matrix MatrixXd locs ... bounding_box(t, locs.row(i), ...))
    return Rectangle(Length+safty_margin(0),
                     Width+safty_margin(1),
                     orientation(t),
                     loc[0],
                     loc[1]);
  };

  double orientation(double t) const {
    /* Orientation of vehicle at time t.
    The end state of a trajectory is zero acceleration, so we can get the
    orientation at any time after the knot by looking at the orientation just
    before the knot. This helps with cases where the speed in one direction is
    zero. Note this should work for this project, but it should be changed out
    with the proper orientation equations, as given in the paper "Optimal
    Trajectory Generation for Dynamic Street Senarios in the Frenet Frame" */
    t = t - t0;
    double T = std::max(trajectory[0].knot, trajectory[1].knot);
    t = (t>=T-0.01) ? T-0.01 : t;

    Vector3d sx = trajectory[0].state_at(t);
    Vector3d sy = trajectory[1].state_at(t);
    return atan2(sy[1], sx[1]);
  };

  State state_at(double t) const {
    return State(trajectory[0].state_at(t-t0), trajectory[1].state_at(t-t0));
  }

  vector<Trajectory> trajectory_at(double t) const {
    return {trajectory[0].trajectory_at(t-t0), trajectory[1].trajectory_at(t-t0)};
  }

  vector<double> location(double t) const {
    return {trajectory[0].ppval(t-t0), trajectory[1].ppval(t-t0)};
  }

  // Vector2d location_Eig(double t) const {
  //   return (Vector2d() << trajectory[0].ppval(t-t0), trajectory[1].ppval(t-t0)).finished();
  // }

  // Get the location at many times. The type could be VectorXd or a
  // vector<double>, for example.
  // template <typename T>
  // vector<vector<double>> location(T t) const {
  //   vector<vector<double>> loc;
  //   for (int i = 0; i<t.size(); ++i) {
  //     loc.push_back(location(t(i)));
  //   }
  //   return loc;
  // }

  template <typename T>
  MatrixXd location_Eig(T const& t) const {
    MatrixXd loc(t.size(),2);
    for (int i = 0; i<t.size(); ++i) {
      auto loc_i = location(t(i));
      loc.row(i) << loc_i[0], loc_i[1];
    }
    return loc;
  }
};


#endif /* Vehicle */
