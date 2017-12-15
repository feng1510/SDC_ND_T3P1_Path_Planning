#include "RoadMap.h"


RoadMap::RoadMap() {};
RoadMap::RoadMap(string map_file) {
  LoadWaypoints(map_file);
}

RoadMap::~RoadMap() {};

void RoadMap::LoadWaypoints(string map_file) {

  // temporary vectors
  vector<double> mx,my,ms,mdx,mdy;

  // Waypoint map to read from
  // string map_file_ = "../data/highway_map.csv";

  // The max s value before wrapping around the track back to 0
  double max_s = 6945.554;

  std::ifstream in_map_(map_file.c_str(), std::ifstream::in);

  string line;
  while (getline(in_map_, line)) {
  	std::istringstream iss(line);
  	double x,y;
  	float s, dx, dy;
  	iss >> x;
  	iss >> y;
  	iss >> s;
  	iss >> dx;
  	iss >> dy;
  	mx.push_back(x);
  	my.push_back(y);
  	ms.push_back(s);
    mdx.push_back(dx);
    mdy.push_back(dy);
  }

  // Number of waypoints
  num_wp = mx.size();

  // Initialize Matrix size
  wp_r = MatrixXd::Zero(num_wp,2);
  wp_n = MatrixXd::Zero(num_wp,2);
  wp_s = VectorXd::Zero(num_wp);

  for (int i=0; i<num_wp; i++) {
    wp_r.row(i) << mx[i], my[i];
    wp_n.row(i) << mdx[i], mdy[i];
    wp_s(i) = ms[i];
  }

  // Make vectors cyclic
  mx.insert(mx.begin(), mx.back());
  my.insert(my.begin(), my.back());
  ms.insert(ms.begin(), ms.back()-max_s);

  mx.push_back(mx[0]);
  my.push_back(my[0]);
  ms.push_back(max_s);
  mx.push_back(mx[1]);
  my.push_back(my[1]);
  ms.push_back(max_s+ms[1]);

  // Create two peicewise continuous hermite spline interplantes using s for the
  // independent variable and x/y as the dependent varaibles
  x_road = pchip(ms,mx);
  y_road = pchip(ms,my);
}

int RoadMap::ClosestWaypoint(double x, double y) {
  // Initialize closestWaypoint index
  MatrixXd::Index closestWaypoint;

  // Create vector with current point
  Vector2d r;
  r << x, y;

  // Compute waypoint that is closest to the current point
  (wp_r.rowwise() - r.transpose()).rowwise().squaredNorm().minCoeff(&closestWaypoint);

  // cout << "Closest Waypoint : " << wp_r(closestWaypoint,0) << ", " << wp_r(closestWaypoint,1) << " : idx = " << closestWaypoint << endl;

	return closestWaypoint;
}

int RoadMap::NextWaypoint(double x, double y, double theta) {

	int closestWaypoint = ClosestWaypoint(x,y);

  Vector2d wp0 = wp_r.row(closestWaypoint);
  Vector2d wp0n = wp_n.row(closestWaypoint);
  Vector2d wp0t;
  Vector2d r;
  wp0t << -wp0n(1), wp0n(0);
  r << x, y;

  if ((r-wp0).dot(wp0t) > 0)
  {
    closestWaypoint++;
    if (closestWaypoint == num_wp)
    {
      closestWaypoint = 0;
    }
  }

  // auto m_x = wp_r(closestWaypoint,0);
  // auto m_y = wp_r(closestWaypoint,1);
  //
	// double heading = atan2((m_y-y),(m_x-x));
  //
	// double angle = fabs(theta-heading);
  // angle = min(2*M_PI - angle, angle);
  //
  // if(angle > M_PI/4)
  // {
  //   closestWaypoint++;
  //   if (closestWaypoint == num_wp)
  //   {
  //     closestWaypoint = 0;
  //   }
  // }

  // cout << "Nearest Waypoint : " << wp_r(closestWaypoint,0) << ", " << wp_r(closestWaypoint,1) << " : idx = " << closestWaypoint << endl << endl;

  return closestWaypoint;
}

vector<Matrix2d> RoadMap::ComputeTransformMatrices(int wp0i, int wp1i, VectorXd dist_along_path) {
  Vector2d wp0 = wp_r.row(wp0i);
  Vector2d wp1 = wp_r.row(wp1i);
  Vector2d wp0n = wp_n.row(wp0i);
  Vector2d wp1n = wp_n.row(wp1i);

  Vector2d s_uv = wp1 - wp0; // vector between waypoints
  double d = s_uv.norm(); // distance between waypoints
  s_uv /= d; // unit vector between the two waypoints

  // just make d_uv perpendicular to w (rotated 90o clockwise)
  auto d_uv = s_uv;
  d_uv << s_uv(1), -s_uv(0);

  // d unit vector
  // double frac = 0.5;
  // if (dist_along_path.size() == 1) {
  //   frac = dist_along_path(0) / d;
  //
  //   // cout << "F2C : " << frac << " , i0 = " << wp0i << endl;
  // } else if (dist_along_path.size() == 2) {
  //   frac = dist_along_path.dot(s_uv) / d;
  //   if (frac < 0 || frac > 1)
  //     cout << "  C2F : " << frac << " , i0 = " << wp0i << endl;
  // }
  //
  // frac = (frac > 1) ? 1.0 : frac;
  //
  // Vector2d d_uv = (frac * wp1n + (1.0 - frac) * wp0n);
  // d_uv /= d_uv.norm();
  // s_uv << -d_uv(1), d_uv(0);

  Matrix2d C2F, F2C;
  C2F << s_uv.transpose(), d_uv.transpose();
  F2C  = C2F.inverse();

  return {C2F, F2C};
}

vector<double> RoadMap::getXY(double s, double d) {
  int idx = -1;
  vector<double> x_dat = x_road.evaluate(s, &idx);
  vector<double> y_dat = y_road.evaluate(s, &idx);

  Vector2d r;
  r << x_dat[0], y_dat[0];

  // road tangent vector
  Vector2d t;
  t << x_dat[1], y_dat[1];
  t /= t.norm();

  // road normal vector
  Vector2d n;
  n << t(1), -t(0);

  r += d*n;

  return {r(0), r(1)};
}

Matrix2d RoadMap::TransformMat_C2F(double s) {
  // get x, x', y, y' of the road center line for the current s location
  int idx = -1;
  vector<double> x_dat = x_road.evaluate(s, &idx);
  vector<double> y_dat = y_road.evaluate(s, &idx);

  // road tangent vector
  Vector2d t;
  t << x_dat[1], y_dat[1];
  t /= t.norm();

  // road normal vector
  Vector2d n;
  n << t(1), -t(0);

  Matrix2d C2F;
  C2F << t.transpose(), n.transpose();

  return C2F;
}

Matrix2d RoadMap::TransformMat_F2C(double s) {
  Matrix2d C2F = TransformMat_C2F(s);
  Matrix2d F2C = C2F.inverse();
  return F2C;
}

// Transform from Frenet s,d coordinates to Cartesian x,y coordinates
vector<double> RoadMap::Frenet_to_Cartesian(vector<double> fstate) {

  // get x, x', y, y' of the road center line for the current s location
  int idx = -1;
  vector<double> x_dat = x_road.evaluate(fstate[1], &idx);
  vector<double> y_dat = y_road.evaluate(fstate[1], &idx);

  // x-y location of road center line
  Vector2d r;
  r << x_dat[0], y_dat[0];

  // road tangent vector
  Vector2d t;
  t << x_dat[1], y_dat[1];
  t /= t.norm();

  // road normal vector
  Vector2d n;
  n << t(1), -t(0);

  // x-y location of car
  r += fstate[2]*n;

  // frenet vectors
  Vector2d vf, af;
  vf << fstate[3], fstate[4]; // velocity, frenet
  af << fstate[5], fstate[6]; // acceleration, frenet

  Matrix2d C2F, F2C;
  C2F << t.transpose(), n.transpose();
  F2C  = C2F.inverse();

  auto v = F2C*vf;
  auto a = F2C*af;

  vector<double> state (fstate);
  state[1] = r[0];
  state[2] = r[1];
  state[3] = v[0];
  state[4] = v[1];
  state[5] = a[0];
  state[6] = a[1];

	return state;
}

// Transform from Cartesian x,y coordinates to Frenet s,d coordinates
vector<double> RoadMap::Cartesian_to_Frenet(vector<double> state) {

  // indices of last and next waypoints
  double theta = atan2(state[4],state[3]);
	int wp1i = NextWaypoint(state[1], state[2], theta);
  int wp0i = (wp1i == 0) ? num_wp - 1 : wp1i - 1;

  // Vector2d wp0 = wp_r.row(wp0i);
  // Vector2d wp1 = wp_r.row(wp1i);
  // Vector2d wp0n = wp_n.row(wp0i);
  // Vector2d wp1n = wp_n.row(wp1i);
  // Vector2d wp0t, wp1t;
  // wp0t << -wp0n(1), wp0n(0);
  // wp1t << -wp1n(1), wp1n(0);

  // Vector2d s_uv = wp1 - wp0; // vector between waypoints
  // double d = s_uv.norm(); // distance between waypoints
  // s_uv /= d; // unit vector between the two waypoints

  // cartesian vectors
  Vector2d r, v, a, r0;
  r << state[1], state[2]; // location, cartesian
  v << state[3], state[4]; // velocity, cartesian
  a << state[5], state[6]; // acceleration, cartesian
  r0 = wp_r.row(wp0i);
  r = r - r0; // location relative to wp0, cartesian

  // auto u0 = (r - wp0).dot(wp0t) * wp0t;
  // auto u1 = (r - wp1).dot(wp1t) * wp1t + wp1 - wp0;
  // auto u = (u0 + u1)/2;

  // auto v0 = u.dot(s_uv) * s_uv;

  VectorXd dist_along_path(2);
  // dist_along_path << u(0), u(1);
  // double dist_along_path = 0;
  auto transMat = ComputeTransformMatrices(wp0i, wp1i, dist_along_path);
  auto C2F = transMat[0];

  auto rf = C2F*r;
  auto vf = C2F*v;
  auto af = C2F*a;


  vector<double> fstate (state);
  fstate[1] = rf[0] + wp_s(wp0i);
  fstate[2] = rf[1];
  fstate[3] = vf[0];
  fstate[4] = vf[1];
  fstate[5] = af[0];
  fstate[6] = af[1];

  return fstate;

  // auto wp1 = wp_r.row(wp1i);
  // auto wp0 = wp_r.row(wp0i);
  //
  // auto s_uv = wp1 - wp0; // vector between waypoints
  // double d = s_uv.norm(); // distance between waypoints
  // s_uv /= d; // unit vector between the two waypoints
  //
  // if (BETTER_FRENET_APPROX) {
  //   /* Better approximate the road normal (d_hat) by interpolating the road
  //     normals at the waypoints. This is instead of simplying using the vector
  //     perpendicular to w. */
  //
  //   // vector from the last waypoint to our current position
  //   VectorXd r(2);
  //   r << state[1], state[2];
  //   auto x = r - wp0;
  //
  //   double frac = x.norm()/d; // fraction between the two waypoints
  //
  //   // d unit vector
  //   auto d_uv = frac * wp_n.row(wp1i) + (1.0 - frac) * wp_n.row(wp0i);
  //   d_uv /= d_uv.norm();
  //
  // } else {
  //   // just make d_uv perpendicular to w (rotated 90o clockwise)
  //   auto d_uv = s_uv;
  //   d_uv << s_uv(1), -s_uv(0);
  // }
  //
  // // projectsions of current position along d,s
  // auto proj_d = x.dot(d_uv);
  // auto proj_s = x.dot(s_uv); // approximately, assuming there is a small angle
  // // note that proj_d + proj_x != x,
  //
  // /* ======================================================================= */
  // /* Compute the frenet coordinates and return */
  //
  // /* Frenet d */
  // double frenet_d = proj_d;
  //
	// // see if d value is positive or negative by comparing it to a center point
  // r << 1000, 2000;
  // r = r - wp0;
  // double centerToPos = (r-x).squaredNorm();
  // double centerToRef = (r-proj_s*s_uv).squaredNorm();
	// if(centerToPos <= centerToRef) frenet_d *= -1;
  //
	// /* Frenet s */
  // double frenet_s = proj_s + wp_s(wp0i);
  //
  // /* velocity */
  // VectorXd v(2);
  // v << state[3], state[4];
  //
  // /* acceleration */
  // VectorXd a(2);
  // a << state[5], state[6];
  //
  //
  // vector<double> fstate (state);
  // fstate[1] = frenet_s;
  // fstate[2] = frenet_d;
  // fstate[3] = v.dot(s_uv); // v_s
  // fstate[4] = v.dot(d_uv); // v_d
  // fstate[5] = a.dot(s_uv); // a_s
  // fstate[6] = a.dot(d_uv); // a_d
	// return fstate;

}

vector<vector<double>> RoadMap::get_waypoints_as_vector() {
  vector<vector<double>> wp;
  vector<double> x, y, s, nx, ny;

  for (int i=0; i<num_wp; i++) {
    x.push_back(wp_r(i,0));
    y.push_back(wp_r(i,1));
    s.push_back(wp_s(i));
    nx.push_back(wp_n(i,0));
    ny.push_back(wp_n(i,1));
  }

  wp.push_back(x);
  wp.push_back(y);
  wp.push_back(s);
  wp.push_back(nx);
  wp.push_back(ny);

  return wp;

}

vector<MatrixXd> RoadMap::get_waypoints() {
  vector<MatrixXd> wp;
  wp.push_back(wp_r);
  wp.push_back(wp_n);
  return wp;
}
