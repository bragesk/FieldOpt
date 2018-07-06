//
// Created by joakim on 11.05.18.
//
#include "Subproblem_LU.h"
namespace Optimization {
namespace Optimizers {

#ifdef __cplusplus
extern "C" {
#endif
int SNOPTusrFG2_(integer *Status, integer *n, doublereal x[],
                 integer *needF, integer *neF, doublereal F[],
                 integer *needG, integer *neG, doublereal G[],
                 char *cu, integer *lencu,
                 integer iu[], integer *leniu,
                 doublereal ru[], integer *lenru);
#ifdef __cplusplus
}
#endif

//static double *gradient;
//static double *hessian;
//static double constant;
static Eigen::VectorXd yb_rel_LU;
static Eigen::VectorXd y0_LU;
static int normType;

static Eigen::MatrixXd hessian_LU;
static Eigen::VectorXd gradient_LU;
static double constant_LU;

//Subproblem_LU::Subproblem_LU(SNOPTHandler snoptHandler) {
Subproblem_LU::Subproblem_LU(Settings::Optimizer *settings) {
  settings_ = settings;
  n_ = settings->parameters().number_of_variables;
  y0_ = Eigen::VectorXd::Zero(n_);
  bestPointDisplacement_ = Eigen::VectorXd::Zero(n_);
  xlowCopy_ = Eigen::VectorXd::Zero(n_); /// OBS should be set by the driver file....
  xuppCopy_ = Eigen::VectorXd::Zero(n_);
  loadSNOPT();
  normType_ = Subproblem_LU::INFINITY_NORM;
  normType = Subproblem_LU::INFINITY_NORM;
  setConstraintsAndDimensions(); // This one should set the iGfun/jGvar and so on.
  setAndInitializeSNOPTParameters();

  ResetSubproblem_LU();

}

SNOPTHandler Subproblem_LU::initSNOPTHandler() {
  string prnt_file, smry_file, optn_file;
  optn_file = settings_->parameters().thrdps_optn_file.toStdString() + ".opt.optn";
  smry_file = settings_->parameters().thrdps_smry_file.toStdString() + ".opt.summ";
  prnt_file = settings_->parameters().thrdps_prnt_file.toStdString() + ".opt.prnt";
  SNOPTHandler snoptHandler(prnt_file.c_str(),
                            smry_file.c_str(),
                            optn_file.c_str());
  /*
  prnt_file = "snopt_print.opt.prnt";
  smry_file = "snopt_summary.opt.summ";
  optn_file = "snopt_options.opt.optn";

  SNOPTHandler snoptHandler(prnt_file.c_str(),
                            smry_file.c_str(),
                            optn_file.c_str());*/
  //cout << "[opt]Init. SNOPTHandler.------" << endl;
  return snoptHandler;
}

void Subproblem_LU::setAndInitializeSNOPTParameters() {
  // the controls
  x_ = new double[n_];
  // the initial guess for Lagrange multipliers
  xmul_ = new double[n_];
  // the state of the variables (whether the optimal is likely to be on
  // the boundary or not)
  xstate_ = new integer[n_];
  F_ = new double[neF_];
  Fmul_ = new double[neF_];
  Fstate_ = new integer[neF_];

  nxnames_ = 1;
  nFnames_ = 1;
  xnames_ = new char[nxnames_ * 8];
  Fnames_ = new char[nFnames_ * 8];

}

void Subproblem_LU::Solve(vector<double> &xsol, vector<double> &fsol, char *optimizationType, Eigen::VectorXd centerPoint, Eigen::VectorXd bestPointDisplacement) {
  y0_ = centerPoint;
  y0_LU = y0_;
  bestPointDisplacement_ = bestPointDisplacement;
  yb_rel_LU = bestPointDisplacement_;
  // Set norm specific constraints
  if (normType_ == INFINITY_NORM){
    for (int i = 0; i < n_; ++i){
      xlow_[i] = std::max(bestPointDisplacement[i] - trustRegionRadius_, xlowCopy_[i]);
      xupp_[i] = std::min(bestPointDisplacement[i] + trustRegionRadius_, xuppCopy_[i]);
    }
  }
  else if (normType_ == L2_NORM){
    for (int i = 0; i < n_; ++i){
      xlow_[i] = xlowCopy_[i];
      xupp_[i] = xuppCopy_[i];
    }
  }


  // The snoptHandler must be setup and loaded
  SNOPTHandler snoptHandler = initSNOPTHandler();
  snoptHandler.setProbName("SNOPTSolver");
  snoptHandler.setParameter(optimizationType);

  setOptionsForSNOPT(snoptHandler);

  ResetSubproblem_LU();
  passParametersToSNOPTHandler(snoptHandler);
  integer Cold = 0, Basis = 1, Warm = 2;

  snoptHandler.solve(Cold, xsol, fsol);
  integer exitCode = snoptHandler.getExitCode();
  /*
  if (exitCode == 32){
    cout << "The major iteration limit was reached, trying to increase it to improve on the result" << endl;
    ResetSubproblem_LU();
    snoptHandler.setIntParameter("Major Iterations Limit", 10000);
    snoptHandler.setRealParameter("Major step limit", 1);
    snoptHandler.solve(Cold, xsol, fsol);
  }
  exitCode = snoptHandler.getExitCode();
  if (exitCode == 32 || exitCode == 31){
    cout << "The major iteration limit or the iteration limit was reached, trying to increase it to improve on the result" << endl;
    ResetSubproblem_LU();
    snoptHandler.setIntParameter("Major Iterations Limit", 12000);
    snoptHandler.setRealParameter("Major step limit", 0.8);
    snoptHandler.setIntParameter("Iterations limit", 20000);
    snoptHandler.solve(Cold, xsol, fsol);
  }
   */

}

void Subproblem_LU::ResetSubproblem_LU() {
  for (int i = 0; i < n_; i++) {
    Fstate_[i] = 0;
    xstate_[i] = 0;
    x_[i] = 0.0;
    xmul_[i] = 0;
  }

  for (int h = 0; h < neF_; h++) {
    F_[h] = 0.0;
    Fmul_[h] = 0.0;
  }

}

void Subproblem_LU::passParametersToSNOPTHandler(SNOPTHandler &snoptHandler) {
  snoptHandler.setProblemSize(n_, neF_);
  snoptHandler.setObjective(objRow_);
  snoptHandler.setA(lenA_, iAfun_, jAvar_, A_);
  snoptHandler.setG(lenG_, iGfun_, jGvar_);
  snoptHandler.setX(x_, xlow_, xupp_, xmul_, xstate_);
  snoptHandler.setF(F_, Flow_, Fupp_, Fmul_, Fstate_);
  snoptHandler.setXNames(xnames_, nxnames_);
  snoptHandler.setFNames(Fnames_, nFnames_);
  snoptHandler.setNeA(neA_);
  snoptHandler.setNeG(neG_);
  snoptHandler.setUserFun(SNOPTusrFG2_);
}

void Subproblem_LU::setConstraintsAndDimensions() {
  // This must be set before compiling the code. It cannot be done during runtime through function calls.
  //n_ = 10;

  m_ = 0;
  if (normType_ == Subproblem_LU::L2_NORM){
    m_++;
  }
  neF_ = m_ + 1;
  lenA_ = 0;
  lenG_ = n_ + m_ * n_;
  objRow_ = 0; // In theory the objective function could be any of the elements in F.
  objAdd_ = 0.0;

  iGfun_ = new integer[lenG_];
  jGvar_ = new integer[lenG_];

  iAfun_  = NULL;
  jAvar_  = NULL;
  A_      = NULL;

  /*
 iAfun_ = new integer[lenA_];
 jAvar_ = new integer[lenA_];
 A_ = new double[lenA_];
*/
  /*
  iAfun_[0] = 1;
  jAvar_[0] = 0;
  iAfun_[1] = 1;
  jAvar_[1] = 1;
  iAfun_[2] = 2;
  jAvar_[2] = 0;
  iAfun_[3] = 2;
  jAvar_[3] = 1;
  A_[0] = 1.0;
  A_[1] = 1.2;
  A_[2] = 0.9;
  A_[3] = 3.0;
*/
  // controls lower and upper bounds
  xlow_ = new double[n_];
  xupp_ = new double[n_];

  Flow_ = new double[neF_];
  Fupp_ = new double[neF_];

/*
  Flow_[0] = -infinity_;
  Fupp_[0] = infinity_;
  Flow_[1] = 3;
  Fupp_[1] = 10;

  Flow_[2] = -3;
  Fupp_[2] = 10;
  Flow_[3] = 0;
  Fupp_[3] = 1;

  xlow_[0] = -2;
  xupp_[0] = 2;
  xlow_[1] = -4;
  xupp_[1] = 4;
*/



  /*
  xlow_[0] = -infinity_;
  xupp_[0] = infinity_;
  xlow_[1] = -infinity_;
  xupp_[1] = infinity_;

  //Objective function
  iGfun_[0] = 0;
  jGvar_[0] = 0;
  iGfun_[1] = 0;
  jGvar_[1] = 1;

  //Trust region radius
  iGfun_[2] = 1;
  jGvar_[2] = 0;
  iGfun_[3] = 1;
  jGvar_[3] = 1;
*/

  // Objective function
  Flow_[0] = -infinity_;
  Fupp_[0] =  infinity_;

  // Trust region radius
  Flow_[1] = 0;
  Fupp_[1] = trustRegionRadius_;

  for(int i = 0; i < n_; ++i){
    xlow_[i] = -infinity_;
    xlowCopy_[i] = xlow_[i];
    xupp_[i] = infinity_;
    xuppCopy_[i] = xupp_[i];
  }

  // first the objective
  for (int i = 0; i < n_; i++)
  {
    iGfun_[i] = 0;
    jGvar_[i] = i;
  }

  if (m_ != 0)
  {
    // and then the constraints
    for (int j = 1; j <= m_; j++)
    {
      for (int i = 0; i < n_; i++)
      {
        iGfun_[i + j * n_] = j;
        jGvar_[i + j * n_] = i;
      }
    }
  }

  neG_ = lenG_;
  neA_ = lenA_;


}

Subproblem_LU::~Subproblem_LU() {
  delete[] iGfun_;
  delete[] jGvar_;
  delete[] x_;
  delete[] xlow_;
  delete[] xupp_;
  delete[] xmul_;
  delete[] xstate_;
  delete[] F_;
  delete[] Flow_;
  delete[] Fupp_;
  delete[] Fmul_;
  //delete[] Fstate_;
  //delete[] xnames_;
  //delete[] Fnames_;
}

void Subproblem_LU::setOptionsForSNOPT(SNOPTHandler &snoptHandler) {

  //if (settings_->verb_vector()[6] >= 1) // idx:6 -> opt (Optimization)
  //cout << "[opt]Set options for SNOPT.---" << endl;

  //snoptHandler.setParameter("Backup basis file              0");
//  snoptHandler.setRealParameter("Central difference interval", 2 * derivativeRelativePerturbation);

  //snoptHandler.setIntParameter("Check frequency",           60);
  //snoptHandler.setParameter("Cold Start                     Cold");

  //snoptHandler.setParameter("Crash option                   3");
  //snoptHandler.setParameter("Crash tolerance                0.1");
  //snoptHandler.setParameter("Derivative level               3");

//  if ( (optdata.optimizationType == HISTORY_MATCHING) || hasNonderivativeLinesearch )
//    snoptHandler.setParameter((char*)"Nonderivative linesearch");
//  else
  //snoptHandler.setParameter((char*)"Derivative linesearch");
  //snoptHandler.setIntParameter("Derivative option", 0);

//  snoptHandler.setRealParameter("Difference interval", optdata.derivativeRelativePerturbation);

  //snoptHandler.setParameter("Dump file                      0");
  //snoptHandler.setParameter("Elastic weight                 1.0e+4");
  //snoptHandler.setParameter("Expand frequency               10000");
  //snoptHandler.setParameter("Factorization frequency        50");
  //snoptHandler.setRealParameter("Function precision", sim_data.tuningParam.tstep.minDeltat);
  //snoptHandler.setParameter("Hessian full memory");
  //snoptHandler.setParameter("Hessian limited memory");

//  snoptHandler.setIntParameter("Hessian frequency", optdata.frequencyToResetHessian);
  //snoptHandler.setIntParameter("Hessian updates", 0);
  //snoptHandler.setIntParameter("Hessian flush", 1);  // Does NOT work in the current version of SNOPT!!!

  //snoptHandler.setParameter("Insert file                    0");
//  snoptHandler.setRealParameter("Infinite bound", optdata.defaultControlUpperBound);

  //snoptHandler.setParameter("Iterations limit");
  //snoptHandler.setRealParameter("Linesearch tolerance",0.9);
  //snoptHandler.setParameter("Load file                      0");
  //snoptHandler.setParameter("Log frequency                  100");
  //snoptHandler.setParameter("LU factor tolerance            3.99");
  //snoptHandler.setParameter("LU update tolerance            3.99");
  //snoptHandler.setRealParameter("LU factor tolerance", 3.99);
  //snoptHandler.setRealParameter("LU update tolerance", 3.99);
  //snoptHandler.setParameter("LU partial pivoting");
  //snoptHandler.setParameter("LU density tolerance           0.6");
  //snoptHandler.setParameter("LU singularity tolerance       3.2e-11");

  //target nonlinear constraint violation
  //snoptHandler.setRealParameter("Major feasibility tolerance", 0.000001);
  snoptHandler.setIntParameter("Major Iterations Limit", 1000);

  //target complementarity gap
  //snoptHandler.setRealParameter("Major optimality tolerance", 0.0001);

  snoptHandler.setParameter("Major Print level  00000"); //  000001"
  snoptHandler.setRealParameter("Major step limit", 0.2);
  //snoptHandler.setIntParameter("Minor iterations limit", 200); // 200

  //for satisfying the QP bounds
//  snoptHandler.setRealParameter("Minor feasibility tolerance", optdata.constraintTolerance);
  snoptHandler.setIntParameter("Minor print level", 0);
  //snoptHandler.setParameter("New basis file                 0");
  //snoptHandler.setParameter("New superbasics limit          99");
  //snoptHandler.setParameter("Objective Row");
  //snoptHandler.setParameter("Old basis file                 0");
  //snoptHandler.setParameter("Partial price                  1");
  //snoptHandler.setParameter("Pivot tolerance                3.7e-11");
  //snoptHandler.setParameter("Print frequency                100");
  //snoptHandler.setParameter("Proximal point method          1");
  //snoptHandler.setParameter("QPSolver Cholesky");
  //snoptHandler.setParameter("Reduced Hessian dimension");
  //snoptHandler.setParameter("Save frequency                 100");
  snoptHandler.setIntParameter("Scale option", 1);
  //snoptHandler.setParameter("Scale tolerance                0.9");
  snoptHandler.setParameter((char *) "Scale Print");
  snoptHandler.setParameter((char *) "Solution  Yes");
  //snoptHandler.setParameter("Start Objective Check at Column 1");
  //snoptHandler.setParameter("Start Constraint Check at Column 1");
  //snoptHandler.setParameter("Stop Objective Check at Column");
  //snoptHandler.setParameter("Stop Constraint Check at Column");
  //snoptHandler.setParameter("Sticky parameters               No");
  //snoptHandler.setParameter("Summary frequency               100");
  //snoptHandler.setParameter("Superbasics limit");
  //snoptHandler.setParameter("Suppress parameters");
  //snoptHandler.setParameter((char*)"System information  No");
  //snoptHandler.setParameter("Timing level                    3");
  //snoptHandler.setRealParameter("Unbounded objective value   1.0e+15");
  //snoptHandler.setParameter("Unbounded step size             1.0e+18");
  //snoptHandler.setIntParameter("Verify level", -1); //-1
  //snoptHandler.setRealParameter("Violation limit", 1e-8); //1e-8

//  if (settings_->verb_vector()[6] >= 1) // idx:6 -> opt (Optimization)
//    cout << "[opt]Set options for SNOPT.---" << endl;

}

/*****************************************************
ADGPRS, version 1.0, Copyright (c) 2010-2015 SUPRI-B
Author(s): Oleg Volkov          (ovolkov@stanford.edu)
           Vladislav Bukshtynov (bukshtu@stanford.edu)
******************************************************/

bool Subproblem_LU::loadSNOPT(const string libname) {

//#ifdef NDEBUG
  if (LSL_isSNOPTLoaded()) {
    printf("\x1b[33mSnopt is already loaded.\n\x1b[0m");
    return true;
  }

  char buf[256];
  int rc;
  if (libname.empty()) {
    rc = LSL_loadSNOPTLib(NULL, buf, 255);
  } else {
    rc = LSL_loadSNOPTLib(libname.c_str(), buf, 255);
  }

  if (rc) {
    string errmsg;
    errmsg = "Selected NLP solver SNOPT not available.\n"
        "Tried to obtain SNOPT from shared library \"";
    errmsg += LSL_SNOPTLibraryName();
    errmsg += "\", but the following error occured:\n";
    errmsg += buf;
    cout << errmsg << endl;
    return false;
  }
//#endif

  return true;
}



int SNOPTusrFG2_(integer *Status, integer *n, double x[],
                 integer *needF, integer *neF, double F[],
                 integer *needG, integer *neG, double G[],
                 char *cu, integer *lencu,
                 integer iu[], integer *leniu,
                 double ru[], integer *lenru) {

  // Convert std::vector into Eigen3 vector
  Eigen::VectorXd xvec(*n);
  for (int i = 0; i < *n; ++i){
    xvec[i] = x[i];
  }

  int m = *neF - 1;
/*
  Eigen::MatrixXd h = Eigen::MatrixXd::Zero(2,2);
  h << 1, 2, 2, -1;
  Eigen::VectorXd g(2);
  g << 1, 0.9;
  double c = 12;
  constant = c;
  hessian = h;
  gradient = g;
  //normType = 2;
*/
  // If the values for the objective and/or the constraints are desired
  if (*needF > 0) {
    /// The objective function
    //std::cout << "constant\n" << constant_LU << "\nGradient\n" << gradient_LU << "\nHessian\n" << hessian_LU << "\n";
    F[0] = constant_LU + gradient_LU.transpose() * xvec + 0.5* xvec.transpose() * hessian_LU * xvec;
    if (m) {
      /// The constraints
      if (normType == Subproblem_LU::L2_NORM) {
        F[1] = (xvec - yb_rel_LU).norm();
      }
    }
  }


  if (*needG > 0) {
    /// The Derivatives of the objective function
    Eigen::VectorXd grad(*n);
    grad = gradient_LU + hessian_LU*xvec;
    for (int i = 0; i < *n; ++i){
      G[i] = grad(i);
    }

    /// The derivatives of the constraints
    if (m) {
      Eigen::VectorXd gradConstraint(*n);
      if (normType == Subproblem_LU::L2_NORM) {
        gradConstraint = (xvec - yb_rel_LU) / ((xvec - yb_rel_LU).norm() + 0.000000000000001);
        for (int i = 0; i < *n; ++i) {
          G[i + *n] = gradConstraint(i);
        }
      }
    }
  }

  // The sphere
  /*
  F[0] = 0;
  for (int i = 0; i < *n; i++){
    F[0] += (xvec(i))*(xvec(i));
    G[i] = 2*(xvec(i));
  }
  F[1] = xvec.norm();
  Eigen::VectorXd gradConstraint(*n);
  gradConstraint = xvec/(xvec.norm()+0.00000000001);
  for (int i = 0; i < *n; ++i){
    G[i+*n] = gradConstraint(i);
  }
  */
  return 0;
}

void Subproblem_LU::Solve(vector<double> &xsol, vector<double> &fsol, char *optimizationType, Eigen::VectorXd centerPoint, Eigen::VectorXd bestPointDisplacement, Eigen::VectorXd startingPoint){
  y0_ = centerPoint;
  bestPointDisplacement_ = bestPointDisplacement;
  // Set norm specific constraints
  if (normType_ == INFINITY_NORM){
    for (int i = 0; i < n_; ++i){
      xlow_[i] = std::max(bestPointDisplacement_[i] - trustRegionRadius_, xlowCopy_[i] - y0_[i]);
      xupp_[i] = std::min(bestPointDisplacement_[i] + trustRegionRadius_, xuppCopy_[i] - y0_[i]);
    }
    //xlow_[1] = xlow_[1];
    //xupp_[1] = xupp_[1];
  }
  else if (normType_ == L2_NORM){
    for (int i = 0; i < n_; ++i){
      xlow_[i] = xlowCopy_[i];
      xupp_[i] = xuppCopy_[i];
    }
  }


  // The snoptHandler must be setup and loaded
  SNOPTHandler snoptHandler = initSNOPTHandler();
  snoptHandler.setProbName("SNOPTSolver");
  snoptHandler.setParameter(optimizationType);

  setOptionsForSNOPT(snoptHandler);
  snoptHandler.setIntParameter("Major Iterations Limit", 20000);
  snoptHandler.setIntParameter("Iterations limit", 20000);
  snoptHandler.setRealParameter("Major step limit", trustRegionRadius_); //was 0.2
  //target nonlinear constraint violation
  snoptHandler.setRealParameter("Major feasibility tolerance", 0.00000000001); //1.0e-6

  snoptHandler.setRealParameter("Major optimality tolerance", 0.000000000000000001*trustRegionRadius_);
  snoptHandler.setRealParameter("Major optimality tolerance", 0.00000000000000000000001*trustRegionRadius_);

  //snoptHandler.setRealParameter("Major optimality tolerance", 0.00000001);

  ResetSubproblem_LU();

  for (int i = 0; i < n_; i++) {
    //x_[i] = startingPoint[i];//bestPointDisplacement_[i];
    //x_[i] = 0.0;//bestPointDisplacement_[i];
    x_[i] = bestPointDisplacement_[i];
  }
  passParametersToSNOPTHandler(snoptHandler);
  integer Cold = 0, Basis = 1, Warm = 2;

  snoptHandler.solve(Cold, xsol, fsol);
  integer exitCode = snoptHandler.getExitCode();
  std::cout << "snopt exitcode:  " << exitCode << "\n";
  if (exitCode == 40 || exitCode == 41){
    std::cout << "snopt failed me. current point cannot be improved because of numerical difficulties \n";

    //std::cin.get();
  }
  if (exitCode != 40 && exitCode != 41 && exitCode != 1 && exitCode != 31 && exitCode != 3){
    std::cout << "ExitCode is: " << exitCode << "\n";
    std::cin.get();
  }
}

void Subproblem_LU::setQuadraticModel(double c, Eigen::VectorXd g, Eigen::MatrixXd H) {
  constant_LU = c;
  gradient_LU = g;
  hessian_LU = H;
}

void Subproblem_LU::setGradient(Eigen::VectorXd g) {
  gradient_LU = g;
}

void Subproblem_LU::setHessian(Eigen::MatrixXd H) {
  hessian_LU = H;
}

void Subproblem_LU::SetCoefficients(Eigen::MatrixXd changes){
  int m = changes.rows();
  constant_LU = 0;
  hessian_LU = Eigen::MatrixXd::Zero(n_,n_);
  gradient_LU = Eigen::VectorXd::Zero(n_);
  //hessian.setZero();
  //gradient_LU.setZero();
  SetCenterPoint(Eigen::VectorXd::Zero(n_));
  normType_ = Subproblem_LU::INFINITY_NORM;
  normType = normType_;
  SetTrustRegionRadius(1);



  int elem = 0;

  /// Linear
  for (int i = 1; i <= n_; ++i) {
    if (elem < m) {
      gradient_LU(i - 1) = changes(m - 1, i);
      elem++;
    } else {
      break;
    }
  }
  /// Squared
  for (int i = 1; i <= n_; ++i) {
    if (elem < m) {
      hessian_LU(i-1,i-1) = changes(m - 1, n_+i-1);
      elem++;
    } else {
      break;
    }
  }
  /// Cross terms
  int rows = n_;
  int it = 1;
  int col = 1;
  int t = 2;
  for (int k = 1; k <= n_ - 1; ++k) {
    for (int i = t; i <= n_ - 1; ++i) {
        if (elem < m) {
          hessian_LU(k-1,i-1) = changes(m-1, elem);
          hessian_LU(i-1,k-1) = hessian_LU(k-1,i-1); // transposed
        } else {
          break;
        }
      elem++;
    }
    t++;
  }
}


void Subproblem_LU::setConstant(double c) {
  constant_LU = c;
}
void Subproblem_LU::setNormType(int type) {
  normType = type;
  normType_ = type;
}
void Subproblem_LU::setCenterPointOfModel(Eigen::VectorXd cp) {
  y0_LU = cp;
}
void Subproblem_LU::setCurrentBestPointDisplacement(Eigen::VectorXd db) {
  bestPointDisplacement_ = db;
  yb_rel_LU = bestPointDisplacement_;
}
void Subproblem_LU::SetCenterPoint(Eigen::VectorXd cp) {
  y0_ = cp;
  y0_LU = y0_;
}
void Subproblem_LU::SetBestPointRelativeToCenterPoint(Eigen::VectorXd bp) {

}
void Subproblem_LU::printModel() {
  std::cout << "The model of the subproblem \n";
  std::cout << "constant = \n" << constant_LU << std::endl;
  std::cout << "gradient = \n" << gradient_LU << std::endl;
  std::cout << "hessian = \n" << hessian_LU << std::endl;
  std::cout << "trust region radius = " << trustRegionRadius_ << std::endl;

}

}
}