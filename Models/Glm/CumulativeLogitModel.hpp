// Copyright 2018 Google LLC. All Rights Reserved.
/*
  Copyright (C) 2005 Steven L. Scott

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef CUMULATIVE_LOGIT_MODEL_HPP
#define CUMULATIVE_LOGIT_MODEL_HPP

#include "Models/Glm/Glm.hpp"
#include "Models/Glm/OrdinalCutpointModel.hpp"

namespace BOOM {
  class CumulativeLogitModel : public OrdinalCutpointModel {
   public:
    CumulativeLogitModel(const Vector &beta, const Vector &delta);
    CumulativeLogitModel(const Matrix &X, const Vector &y);
    CumulativeLogitModel(const CumulativeLogitModel &rhs);
    CumulativeLogitModel *clone() const override;

    double link(double prob) const override;
    double link_inv(double eta) const override;
    double dlink_inv(double eta) const override;
    double ddlink_inv(double eta) const override {return 1.0;}

   private:
    double simulate_latent_variable(RNG &rng) const override;
  };

}  // namespace BOOM

#endif  // CUMULATIVE_LOGIT_MODEL_HPP
