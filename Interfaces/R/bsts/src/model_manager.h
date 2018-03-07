#ifndef ANALYSIS_COMMON_R_BSTS_SRC_MODEL_MANAGER_H_
#define ANALYSIS_COMMON_R_BSTS_SRC_MODEL_MANAGER_H_

#include "r_interface/boom_r_tools.hpp"
#include "r_interface/list_io.hpp"
#include "Models/StateSpace/StateSpaceModelBase.hpp"

namespace BOOM {
namespace bsts {

//===========================================================================
// The code that computes out of sample one-step prediction errors is designed
// for multi-threading.  This base class provides the interface for computing
// the prediction errors.
class HoldoutErrorSamplerImpl {
 public:
  virtual ~HoldoutErrorSamplerImpl() {}

  // Simulate from the distribution of one-step prediction errors given data up
  // to some cutpoint.  Child classes must be equipped with enough state to
  // carry out this operation and store the results in an appropriate data
  // structure.
  virtual void sample_holdout_prediction_errors() = 0;
};

// A null class that can be used by model families that do not support one step
// prediction errors (e.g. logit and Poisson).
class NullErrorSampler : public HoldoutErrorSamplerImpl {
 public:
  void sample_holdout_prediction_errors() override {}
};

// A pimpl-based functor for computing out of sample prediction errors, with the
// appropriate interface for submitting to a ThreadPool.
class HoldoutErrorSampler {
 public:
  explicit HoldoutErrorSampler(HoldoutErrorSamplerImpl *impl) : impl_(impl) {}
  void operator()() {impl_->sample_holdout_prediction_errors();}
 private:
  std::unique_ptr<HoldoutErrorSamplerImpl> impl_;
};

//===========================================================================
// The job of a ModelManager is to construct the BOOM models that bsts uses for
// inference, and to provide any additional data that the models need for tasks
// other than statistical inference.
//
// The point of the ModelManager is to be an intermediary between the calls in
// bsts.cc and the underlying BOOM code, so that R can pass lists of data,
// priors, and model options formatted as expected by the child ModelManager
// classes for specific model types.
class ModelManager {
 public:
  ModelManager();

  virtual ~ModelManager() {}

  // Create a ModelManager instance suitable for working with the
  // specified family.
  // Args:
  //   family: A text string identifying the model family.
  //     "gaussian", "logit", "poisson", or "student".
  //   xdim: Dimension of the predictors in the observation model
  //     regression.  This can be zero if there are no regressors.
  static ModelManager * Create(const std::string &family, int xdim);

  // Create a model manager by reinstantiating a previously
  // constructed bsts model.
  // Args:
  //   r_bsts_object:  An object previously created by a call to bsts.
  static ModelManager * Create(SEXP r_bsts_object);

  // Creates a BOOM state space model suitable for learning with MCMC.
  // Args:
  //   r_data_list: An R list containing the data to be modeled in the
  //     format expected by the requested model family.  This list
  //     generally contains an object called 'response' and a logical
  //     vector named 'response.is.observed'.  If the model is a
  //     (generalized) regression model then it will contain a
  //     'predictors' object as well, otherwise 'predictors' will be
  //     NULL.  For logit or Poisson models an additional component
  //     should be present giving the number of trials or the
  //     exposure.
  //   r_state_specification: The R list created by the state
  //     configuration functions (e.g. AddLocalLinearTrend, AddSeasonal,
  //     etc).
  //   r_prior: The prior distribution for the observation model.  If
  //     the model is a regression model (determined by whether
  //     r_data_list contains a non-NULL 'predictors' element) then this
  //     must be some form of spike and slab prior.  Otherwise it is a
  //     prior for the error in the observation equation.  For single
  //     parameter error distributions like binomial or Poisson this can
  //     be NULL.
  //   r_options: Model or family specific options such as the technique
  //     to use for model averaging (ODA vs SVSS).
  //   family: A text string indicating the desired model family for the
  //     observation equation.
  //   save_state_contribution: A flag indicating whether the
  //     state-level contributions should be saved by the io_manager.
  //   save_prediction_errors: A flag indicating whether the one-step
  //     prediction errors from the Kalman filter should be saved by the
  //     io_manager.
  //   final_state: A pointer to a Vector to hold the state at the final
  //     time point.  This can be a nullptr if the state is only going
  //     to be recorded, but it must point to a Vector if the state is
  //     going to be read from an existing object.
  //   io_manager: The io_manager responsible for writing MCMC output to
  //     an R object, or streaming it from an existing object.
  //
  // Returns:
  //  A pointer to the created model.  The pointer is owned by a Ptr
  //  in the model manager, and should be caught by a Ptr in the caller.
  //
  // Side Effects:
  //   The returned pointer is also held in a smart pointer owned by
  //   the child class.
  virtual StateSpaceModelBase * CreateModel(
      SEXP r_data_list,
      SEXP r_state_specification,
      SEXP r_prior,
      SEXP r_options,
      Vector *final_state,
      bool save_state_contribution,
      bool save_prediction_errors,
      RListIoManager *io_manager);

  // Returns a set of draws from the posterior predictive distribution.
  // Args:
  //   r_bsts_object:  The R object created from a previous call to bsts().
  //   r_prediction_data: Data needed to make the prediction.  This
  //     might be a data frame for models that have a regression
  //     component, or a vector of exposures or trials for binomial or
  //     Poisson data.
  //   r_options: If any special options need to be passed in order to
  //     do the prediction, they should be included here.
  //   r_observed_data: In most cases, the prediction takes place
  //     starting with the time period immediately following the last
  //     observation in the training data.  If so then r_observed_data
  //     should be R_NilValue, and the observed data will be taken
  //     from r_bsts_object.  However, if more data have been added
  //     (or if some data should be omitted) from the training data, a
  //     new set of training data can be passed here.
  //
  // Returns:
  //   An R matrix, with rows corresponding to MCMC draws and columns
  //   to time, containing posterior predictive draws for the
  //   forecast.
  virtual Matrix Forecast(
      SEXP r_bsts_object,
      SEXP r_prediction_data,
      SEXP r_burn,
      SEXP r_observed_data);

  // Returns a HoldoutErrorSampler that holds a family specific implementation
  // pointer that samples one-step prediction errors for data in r_bsts_object
  // beyond observation number 'cutpoint'.  This object can be submitted to a
  // ThreadPool for parallel processing.
  //
  // Args:
  //   r_bsts_object: A bsts object fit to full data, for which one-step
  //     prediction errors are desired.
  //   cutpoint: An integer giving the index of the last data point in
  //     r_bsts_object to be considered training data.  Observations after
  //     'cutpoint' are considered holdout data.
  //   prediction_error_output: A reference to a Matrix, with rows corresponding
  //     to MCMC iterations, and columns to observations in the holdout data
  //     set.  The matrix will be resized to appropriate dimensions by this
  //     call.
  //
  // Note that one step prediction errors are only supported for Gaussian
  // models.
  virtual HoldoutErrorSampler CreateHoldoutSampler(
      SEXP r_bsts_object,
      int cutpoint,
      BOOM::Matrix *prediction_error_output) = 0;

  // Time stamps are considered trivial if either (a) no time stamp information
  // was provided by the user, or (b) each time stamp contains one observation
  // and there are no gaps in the time series of observations.
  bool TimestampsAreTrivial() const {
    return timestamps_are_trivial_;
  }

  int NumberOfTimePoints() const {
    return number_of_time_points_;
  }

  // Returns the timestamp number of observation i.  Converts from R's unit
  // based indexing system to C's 0-based system.
  int TimestampMapping(int i) const {
    return timestamps_are_trivial_ ? i : timestamp_mapping_[i] - 1;
  }

  RNG & rng() {return rng_;}

 protected:
  // Checks to see if r_data_list has a field named timestamp.info, and use it
  // to populate the follwoing fields: number_of_time_points_,
  // timestamps_are_trivial_, and timestamp_mapping_.
  void UnpackTimestampInfo(SEXP r_data_list);

 private:
  // Create the specific StateSpaceModel suitable for the given model
  // family.  The posterior sampler for the model is set, and entries
  // for its model parameters are created in io_manager.  This
  // function does not add state to the the model.  It is primarily
  // intended to aid the implementation of CreateModel.
  //
  // The arguments are documented in the comment to CreateModel.
  //
  // Returns:
  //   A pointer to the created model.  The pointer is owned by a Ptr
  //   in the the child class, so working with the raw pointer
  //   privately is exception safe.
  virtual StateSpaceModelBase * CreateObservationModel(
      SEXP r_data_list,
      SEXP r_prior,
      SEXP r_options,
      RListIoManager *io_manager) = 0;

  // Add data to the model object managed by the child classes.  The
  // data can come either from a previous bsts object, or from an R
  // list containing appropriately formatted data.
  virtual void AddDataFromBstsObject(SEXP r_bsts_object) = 0;
  virtual void AddDataFromList(SEXP r_data_list) = 0;

  // Allocates and fills the appropriate data structures needed for
  // forecasting, held by the child classes.
  // Args:
  //    r_prediction_data: An R list containing data needed for
  //      prediction.
  //
  // Returns:
  //    The number of periods to be forecast.
  virtual int UnpackForecastData(SEXP r_prediction_data) = 0;

  // Unpacks forecast data for the dynamic regression state component,
  // if one is present in the model.
  // Args:
  //   model:  The model to be forecast.
  //   r_state_specification: The R list of state specfication
  //     elements, used to determine the position of the dynamic
  //     regression component.
  //   r_prediction_data: A list.  If a dynamic regression component
  //     is present this list must contain an element named
  //     "dynamic.regression.predictors", which is an R matrix
  //     containing the forecast predictors for the dynamic regression
  //     component.
  void UnpackDynamicRegressionForecastData(
      StateSpaceModelBase *model,
      SEXP r_state_specification,
      SEXP r_prediction_data);

  // This function must not be called before UnpackForecastData.  It
  // takes the current state of the model held by the child classes,
  // along with the data obtained by UnpackForecastData(), and
  // simulates one draw from the posterior predictive forecast
  // distribution.
  virtual Vector SimulateForecast(const Vector &final_state) = 0;

  RNG rng_;

  //----------------------------------------------------------------------
  // Time stamps are trivial the timestamp information was NULL, or if there is
  // at most one observation at each time point.
  bool timestamps_are_trivial_;

  // The number of distinct time points.
  int number_of_time_points_;

  // Indicates the time point (in R's 1-based counting system) to which each
  // observation belongs.
  std::vector<int> timestamp_mapping_;
};

}  // namespace bsts
}  // namespace BOOM

#endif  // ANALYSIS_COMMON_R_BSTS_SRC_MODEL_MANAGER_H_
