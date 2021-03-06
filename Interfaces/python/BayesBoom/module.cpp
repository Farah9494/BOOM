#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>

#include <cpputil/Ptr.hpp>
namespace py = pybind11;
using namespace BOOM;
PYBIND11_DECLARE_HOLDER_TYPE(T, BOOM::Ptr<T>, true);

namespace BayesBoom {
  // Forward definitions of all the class definitions to be added in other
  // files.  Each of these is defined in a local cpp file, but invoked here.
  // That way all the definitions occur within the same module.
  void distribution_def(py::module &);
  void LinAlg_def(py::module &);
  void stats_def(py::module &);

  void Model_def(py::module &);
  void Parameter_def(py::module &);
  void GaussianModel_def(py::module &);
  void GammaModel_def(py::module &);
  void MvnModel_def(py::module &);
  void GlmModel_def(py::module &);
  void StateSpaceModel_def(py::module &);
  void StateModel_def(py::module &);

  PYBIND11_MODULE(BayesBoom, boom) {
    boom.doc() = "A library for Bayesian modeling, and assorted "
        "other useful bits.";

    // Calling these functions here defines the classes in the module.
    distribution_def(boom);
    LinAlg_def(boom);
    stats_def(boom);

    Model_def(boom);
    Parameter_def(boom);
    GaussianModel_def(boom);
    GammaModel_def(boom);
    MvnModel_def(boom);

    GlmModel_def(boom);
    StateSpaceModel_def(boom);
    StateModel_def(boom);

  }  // Module BOOM

}  // namespace BayesBoom
