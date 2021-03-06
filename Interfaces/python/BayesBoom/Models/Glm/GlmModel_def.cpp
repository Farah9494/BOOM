#include <pybind11/pybind11.h>

#include "Models/ModelTypes.hpp"
#include "Models/Glm/Glm.hpp"
#include "Models/Glm/GlmCoefs.hpp"
#include "Models/Glm/RegressionModel.hpp"
#include "Models/Glm/VariableSelectionPrior.hpp"

#include "Models/Glm/PosteriorSamplers/BregVsSampler.hpp"

#include "cpputil/Ptr.hpp"

namespace py = pybind11;
PYBIND11_DECLARE_HOLDER_TYPE(T, BOOM::Ptr<T>, true);

namespace BayesBoom {
  using namespace BOOM;

  void GlmModel_def(py::module &boom) {

    py::class_<GlmCoefs,
               VectorParams,
               Ptr<GlmCoefs>>(boom, "GlmCoefs")
        .def_property_readonly(
            "inc",
            [](const GlmCoefs &coefs) {return coefs.inc();},
            "The Selector object indicating which variables are included "
            "or excluded.")
        .def_property_readonly(
            "included_coefficients",
            &GlmCoefs::included_coefficients,
            "The vector of coefficients corresponding to the subset of "
            "included variables.")
        ;

    // Base class for generalized linear models: regression, logistic
    // regression, Poisson regression, etc.
    py::class_<GlmModel,
               Model,
               Ptr<GlmModel>>(boom, "GlmModel")
        .def_property_readonly(
            "coef",
            [] (GlmModel &model) {return model.coef();},
            "The model coefficient object.")
        .def_property_readonly("xdim", &GlmModel::xdim)
        .def("add_all", &GlmModel::add_all)
        .def("drop_all", &GlmModel::drop_all)
        .def("drop_all_but_intercept", &GlmModel::drop_all_but_intercept)
        .def("add", &GlmModel::add, "Add the variable in the specified position.")
        .def("drop", &GlmModel::add, "Drop the variable in the specified position.")
        .def("flip", &GlmModel::add, "Flip the variable in the specified position.")
        .def_property_readonly(
            "coef",
            [](const GlmModel &m) {return m.coef();},
            "The object containing the model coefficients.")
        ;

    py::class_<RegSuf,
               Ptr<RegSuf>>(
                   boom, "RegSuf", py::multiple_inheritance())
        .def_property_readonly(
            "sample_mean",
            [](const RegSuf &s) { return s.ybar(); },
            "The sample mean of the training data.")
        .def_property_readonly(
            "sample_variance",
            [](const RegSuf &s) {
              Vector beta(s.size());
              beta[0] = s.ybar();
              return s.relative_sse(beta) / (s.n() - 1.0);
            },
            "Sample variance of the training data.")
        ;

    py::class_<RegressionModel,
               GlmModel,
               PriorPolicy,
               Ptr<RegressionModel>>(
                   boom, "RegressionModel", py::multiple_inheritance())
        .def(py::init<Matrix, Vector, bool>(),
             py::arg("X"),
             py::arg("y"),
             py::arg("start_at_mle") = false,
             "Create a regression model from a data set.\n\n"
             "Args:"
             "  X:  boom.Matrix of predictor variables.\n"
             "  y:  boom.Vector of responses.\n"
             "  start_at_mle: If True then the model parameters will be initialized "
             "to the maximum likelihood estimate (which will be undefined if X is "
             "less than full rank).  If False then model parameters begin at "
             "default levels.")
        .def_property_readonly(
            "suf",
            [](const RegressionModel &m) {return m.suf();},
            "RegSuf object containing the sufficient statistics for the model.")
        .def_property_readonly(
            "Sigsq_prm",
            [](RegressionModel &m) {
              return m.Sigsq_prm();
            },
            "The parameter object representing the residual variance.  boom.UnivParams")
        .def_property_readonly(
            "coefficients",
            [](RegressionModel &m) {
              return m.coef();
            },
            "The parameter object representing the model coefficients.  boom.GlmCoefs")
        .def_property_readonly(
            "sigma",
            [](const RegressionModel &m){
              return m.sigma();
            },
            "The residual standard deviation.")
        .def_property_readonly(
            "suf",
            [](const RegressionModel &m) {
              return m.suf();
            },
            "The sufficient statistics for the regression model.")
        .def("log_likelihood",
             [](const RegressionModel& m) {
               return m.log_likelihood();
             })
        ;


    py::class_<VariableSelectionPrior,
               Ptr<VariableSelectionPrior>>(boom, "VariableSelectionPrior")
        .def(py::init<const Vector &>(),
             py::arg("prior_inclusion_probabilities"),
             "Create a VariableSelectionPrior from a vector of prior inclusion probabilities.\n\n"
             "Args:\n\n"
             "  prior_inclusion_probabilities: boom.Vector containing the prior "
             "probability that each variable is to be included. "
             )
        ;

    py::class_<BregVsSampler,
               PosteriorSampler,
               Ptr<BregVsSampler>>(boom, "BregVsSampler")
        .def(py::init(
            [](Ptr<RegressionModel> model,
               const Ptr<MvnGivenScalarSigma> &slab,
               const Ptr<GammaModelBase> &residual_precision_prior,
               const Ptr<VariableSelectionPrior> &spike,
               RNG &seeding_rng) {
              return new BregVsSampler(model.get(), slab, residual_precision_prior,
                                       spike, seeding_rng);
            }),
             py::arg("model"),
             py::arg("slab"),
             py::arg("residual_precision_prior"),
             py::arg("spike"),
             py::arg("seeding_rng") = BOOM::GlobalRng::rng,
             "Create a BregVsSampler -- a spike and slab sampler for regression "
             "models.\n\n"
             "Args:\n"
             "  model:  The model to be sampled.\n"
             "  slab:  The conditional normal prior on the regression "
             "coefficients.\n"
             "  residual_precision_prior:   Prior distribution on the residual "
             "precision (reciprocal of the residual variance).\n"
             "  spike:  Prior distribution over the vector of inclusion "
             "indicators.\n"
             "  seeding_rng:  The random number generator used to set the seed "
             "of the RNG owned by this sampler."
             )
        ;

  }

}  // namespace BayesBoom
