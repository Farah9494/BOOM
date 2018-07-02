/*
  Copyright (C) 2005-2018 Steven L. Scott

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License along
  with this library; if not, write to the Free Software Foundation, Inc., 51
  Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/


#include "gtest/gtest.h"
#include "test_utils/test_utils.hpp"
#include "Models/StateSpace/StateModels/test_utils/HolidayTestModule.hpp"
#include "distributions.hpp"
#include "stats/AsciiDistributionCompare.hpp"
#include "stats/summary.hpp"

namespace BOOM {
  namespace StateSpaceTesting {

    RandomWalkHolidayTestModule::RandomWalkHolidayTestModule(
        const Ptr<Holiday> &holiday,
        const Date &day_zero,
        double sd,
        const Vector &initial_pattern)
        : sd_(sd),
          initial_pattern_(initial_pattern),
          day_zero_(day_zero),
          holiday_(holiday),
          holiday_model_(new RandomWalkHolidayDynamicInterceptStateModel(
              holiday_, day_zero_)),
          precision_prior_(new ChisqModel(1.0, sd_)),
          sampler_(new ZeroMeanGaussianConjSampler(
              holiday_model_.get(), precision_prior_))
    {
      if (initial_pattern_.size() != holiday_->maximum_window_width()) {
        std::ostringstream err;
        err << "The pattern has " << initial_pattern_.size() << " elements "
            << "but " << holiday_->maximum_window_width() << " were expected.";
        report_error(err.str());
      }
      holiday_model_->set_method(sampler_);
      holiday_model_->set_sigsq(square(sd_));
      SpdMatrix initial_variance(holiday_model_->state_dimension(), 0.0);
      holiday_model_->set_initial_state_mean(initial_pattern);
      initial_variance.diag() = 10 * square(sd_);
      holiday_model_->set_initial_state_variance(initial_variance);
    }

    void RandomWalkHolidayTestModule::SimulateData(int time_dimension) {
      if (time_dimension < 2 * 365) {
        report_error("Time dimension is too small to test RandomWalkHoliday.  "
                     "Two years are required.");
      }
      holiday_effect_.resize(time_dimension);
      Vector pattern = initial_pattern_;
      for (int t = 0; t < time_dimension; ++t) {
        Date date = day_zero_ + t;
        if (holiday_->active(date)) {
          int position = holiday_->days_into_influence_window(date);
          holiday_effect_[t] = pattern[position];
        }
        if (holiday_->active(date + 1)) {
          int position = holiday_->days_into_influence_window(date + 1);
          pattern[position] += rnorm_mt(GlobalRng::rng, 0, sd_);
        }
      }
    }
    
    void RandomWalkHolidayTestModule::CreateObservationSpace(int niter) {
      holiday_draws_.resize(niter, holiday_effect_.size());
      sd_draws_.resize(niter);
    }

    void RandomWalkHolidayTestModule::ObserveDraws(
        const StateSpaceModelBase &model) {
      auto state = CurrentState(model);
      int iter = cursor();
      for (int t = 0; t < state.ncol(); ++t) {
        holiday_draws_(iter, t) =
            holiday_model_->observation_matrix(t).dot(state.col(t));
      }
      sd_draws_[cursor()] = holiday_model_->sigma();
    }

    void RandomWalkHolidayTestModule::Check() {
      auto status = CheckMcmcMatrix(holiday_draws_, holiday_effect_);
      EXPECT_TRUE(status.ok)
          << "RandomWalkHoliday failed to cover true state." << std::endl
          << status;

      EXPECT_TRUE(CheckMcmcVector(sd_draws_, sd_, .95,
                                  "random-walk-holiday-sd.txt"))
          << "RandomWalkHoliday sd parameter did not cover true value of "
          << sd_ << "." << std::endl
          << AsciiDistributionCompare(sd_draws_, sd_) << std::endl
          << NumericSummary(sd_draws_);
    }
    
  }  // namespace StateSpaceTesting
}  // namespace BOOM