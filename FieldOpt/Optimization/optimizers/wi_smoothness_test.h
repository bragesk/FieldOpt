/******************************************************************************
   Copyright (C) 2017 Mathias C. Bellout <mathias.bellout@ntnu.no>

   This file is part of the FieldOpt project.

   FieldOpt is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   FieldOpt is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with FieldOpt.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef FIELDOPT_WI_SMOOTHNESS_TEST_H
#define FIELDOPT_WI_SMOOTHNESS_TEST_H

#include <Eigen/Dense>
#include "Optimization/optimizer.h"
#include "GSS.h"

namespace Optimization {
namespace Optimizers {

/*!
 * @brief study of cost function smoothness as a
 * function of well index (differing well paths)
 *
 */
class WISmoothnessTest: public Optimizer {
 public:
  WISmoothnessTest(Settings::Optimizer *settings,
                    Case *base_case,
                    Model::Properties::VariablePropertyContainer *variables,
                    Reservoir::Grid::Grid *grid,
                    Logger *logger);

  void getXCoordVarID();
  Eigen::Matrix<double,Dynamic,1> getPerturbations();

 private:
  Reservoir::Grid::Grid *grid_;
  Model::Properties::VariablePropertyContainer *variables_;
  QHash<QUuid, Model::Properties::ContinousProperty *> *xyzcoord_;
  QUuid x_varid;

  Eigen::Matrix<double,Dynamic,1> pertx_;
  long nblocksx_ = 5;
  long block_sz_ = 24;

  // num of points including 'zero' point
  long npointsx_ = 12;




  // QString GetStatusStringHeader() const;
  // QString GetStatusString() const;

  /*!
  * @brief
  * @return
  */
  void iterate();

  virtual TerminationCondition IsFinished() override {};

 protected:
  void handleEvaluatedCase(Case *c) override {};
};

} // namespace Optimization
} // namespace Optimizers

#endif //FIELDOPT_WI_SMOOTHNESS_TEST_H
