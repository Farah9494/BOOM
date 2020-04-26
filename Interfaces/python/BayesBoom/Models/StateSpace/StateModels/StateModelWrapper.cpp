#include <pybind11/pybind11.h>

#include "Models/StateSpace/StateModels/LocalLevelStateModel.hpp"
#include "Models/StateSpace/StateModels/LocalLinearTrend.hpp"
#include "Models/StateSpace/StateModels/SeasonalStateModel.hpp"

#include "Models/PosteriorSamplers/ZeroMeanGaussianConjSampler.hpp"

#include "cpputil/Ptr.hpp"

namespace py = pybind11;
PYBIND11_DECLARE_HOLDER_TYPE(T, BOOM::Ptr<T>, true);

namespace BayesBoom {
  using namespace BOOM;

  void StateModel_def(py::module &boom) {

    py::class_<StateModel,
               BOOM::Ptr<StateModel>>(
                   boom, "StateModel", py::multiple_inheritance())
        ;

    // Base class
    py::class_<LocalLevelStateModel,
               StateModel,
               ZeroMeanGaussianModel,
               BOOM::Ptr<LocalLevelStateModel>>(boom, "LocalLevelStateModel")
        .def(py::init<double>(),
             py::arg("sigma") = 1.0,
             "Args:\n"
             "  sigma: Standard deviation of the innovation errors.")
        .def_property_readonly(
            "state_dimension", &LocalLevelStateModel::state_dimension,
            "Dimension of the state vector.")
        .def_property_readonly(
            "state_error_dimension",
            &LocalLevelStateModel::state_error_dimension,
            "Dimension of the innovation term.")
        .def("set_initial_state_mean",
             [] (Ptr<LocalLevelStateModel> model, double mean) {
               model->set_initial_state_mean(mean);
             },
             py::arg("mean"),
             "Set the mean of the initial state distribution to the "
             "specified value.")
        .def("set_initial_state_variance",
             [] (Ptr<LocalLevelStateModel> model, double variance) {
               model->set_initial_state_variance(variance);
             },
             py::arg("variance"),
             "Set the variance of the initial state distribution to the "
             "specified value.")
        .def("set_posterior_sampler",
             [] (LocalLevelStateModel &model,
                 const Ptr<GammaModelBase> &prior,
                 RNG &seeding_rng) {
               NEW(ZeroMeanGaussianConjSampler, sampler)(
                   &model, prior, seeding_rng);
               model.set_method(sampler);
               return sampler; },
             py::arg("prior"),
             py::arg("rng") = BOOM::GlobalRng::rng,
             "Args:\n"
             "  prior:  Prior distribution on the innovation precision.\n\n"
             "Returns:\n"
             "  The posterior sampler, which has already been assigned to \n"
             "  the model.  Assigning it again will cause duplicate MCMC moves."
             )
        ;

    py::class_<SeasonalStateModel,
               StateModel,
               ZeroMeanGaussianModel,
               BOOM::Ptr<SeasonalStateModel>>(boom, "SeasonalStateModel")
        .def(py::init<int, int>(),
             py::arg("nseasons"),
             py::arg("season_duration") = 1,
             "Args:\n"
             "  nseasons: Number of seasons in the the model.\n"
             "  season_duration: Number of time periods each season lasts.\n")
        .def_property_readonly("nseasons", &SeasonalStateModel::nseasons)
        .def_property_readonly("season_duration",
                               &SeasonalStateModel::season_duration)
        .def_property_readonly(
            "state_dimension",
            &SeasonalStateModel::state_dimension,
            "Dimension of the state vector.")
        .def_property_readonly(
            "state_error_dimension",
            &SeasonalStateModel::state_error_dimension,
            "Dimension of the error term for this state component.")
        .def("set_initial_state_mean",
             &SeasonalStateModel::set_initial_state_mean,
             py::arg("mu"),
             "Args: \n"
             "  mu: Vector of size 'nseasons' - 1 giving the mean of the state "
             "at time 0.\n")
        .def("set_initial_state_variance",
             [] (Ptr<SeasonalStateModel> seasonal,
                     const BOOM::SpdMatrix &variance) {
               seasonal->set_initial_state_variance(variance);
             },
             py::arg("variance"),
             "Args: \n"
             "  variance: SpdMatrix of size 'nseasons' - 1 giving the variance"
             " of the state at time 0.\n")
        .def("set_initial_state_variance",
             [] (Ptr<SeasonalStateModel> seasonal, double variance) {
               seasonal->set_initial_state_variance(variance);
             },
             py::arg("variance"),
             "Args: \n"
             "  variance: The variance matrix is this constant times the "
             "identity.\n")
             ;


  }  // StateSpaceModel_def

}  // namespace BayesBoom
