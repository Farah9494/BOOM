#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/operators.h>
#include <memory>
#include "LinAlg/Vector.hpp"
#include "LinAlg/VectorView.hpp"
#include "LinAlg/Matrix.hpp"
#include "LinAlg/SpdMatrix.hpp"
#include "LinAlg/Cholesky.hpp"
#include "LinAlg/EigenMap.hpp"



namespace py = pybind11;

namespace BayesBoom {
  using namespace BOOM;

  void LinAlg_def(py::module &boom) {

    py::class_<Vector, std::unique_ptr<Vector>>(boom, "Vector")
        .def(py::init<int, double>(), py::arg("size"), py::arg("value") = 0.0,
             "Create a Vector of the requested size filled with a constant value.")
        .def(py::init( [] (Eigen::Ref<Eigen::VectorXd> numpy_array) {
              VectorView view(numpy_array.data(), numpy_array.size(), 1);
              return std::unique_ptr<Vector>(new Vector(view));
            }),
          "Create a Vector from a numpy array.")
        .def("all_finite", &Vector::all_finite,
             "Returns true iff all elements are finite.")
        .def_property_readonly("randomize", &Vector::randomize,
             "Fill the vector with U(0, 1) random deviates.")
        .def_property_readonly("stride", &Vector::stride,
             "The distance between consecutive elements.  "
             "For a dense vector this is always 1.")
        .def_property_readonly("length", &Vector::length,
                               "The number of elements in the vector.")
        .def_property_readonly("size", &Vector::length,
                               "The number of elements in the vector.")
        .def("to_numpy", [](const Vector &v) {return EigenMap(v);})
        .def("__getitem__", [](const Vector &v, int i) {return v[i];}, py::is_operator())
        .def("__setitem__", [](Vector &v, int i, double value) {return v[i] = value;},
             py::is_operator())
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * py::self)
        .def(py::self / py::self)
        .def(py::self += py::self)
        .def(py::self *= py::self)
        .def(py::self + float())
        .def(py::self - float())
        .def(py::self * float())
        .def(py::self / float())
        .def("__repr__",
             [](const Vector &v) {
               std::ostringstream out;
               out << v;
               return out.str();
             })
        ;
    py::implicitly_convertible<Eigen::VectorXd, Vector>();

    py::class_<Matrix>(boom, "Matrix")
        .def(py::init<int, int, double>(),
             py::arg("nrow") = 0,
             py::arg("ncol") = 0,
             py::arg("value") = 0.0,
             "Create a matrix with the specified number of rows and columns, "
             "with all elements set to the the given value")
        .def(py::init( [] (const Eigen::MatrixXd &numpy_array) {
              return std::unique_ptr<Matrix>(
                  new Matrix(numpy_array.rows(),
                             numpy_array.cols(),
                             numpy_array.data(),
                             false));   // byrow.  False means column-storage order.
            }),
          "Create a Matrix from a 2-D numpy array."
          )
        .def_property_readonly("nrow", &Matrix::nrow, "The number of rows in the matrix.")
        .def_property_readonly("ncol", &Matrix::ncol, "The number of columns in the matrix.")
        .def("to_numpy",
             [](const Matrix &m) {return EigenMap(m);},
             "Convert the matrix to a numpy array." )
        .def("to_numpy", [](const Vector &v) {return EigenMap(v);})
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * py::self)
        .def(py::self / py::self)
        .def(py::self += py::self)
        .def(py::self + float())
        .def(py::self - float())
        .def(py::self * float())
        .def(py::self / float())
        .def(py::self * Vector())
        .def("__repr__",
             [](const Matrix &m) {
               std::ostringstream out;
               out << m;
               return out.str();
             })
        ;

    py::class_<SpdMatrix, Matrix>(boom, "SpdMatrix")
        .def(py::init<int, double>(),
             py::arg("dim") = 0,
             py::arg("diagonal_value") = 1.0,
             "Create a symmetric positive definite matrix of the given dimension. "
             "The diagonal elements are constant and equal to 'digaonal_value'. "
             )
        .def(py::init( [] (const Eigen::Ref<Eigen::MatrixXd> &numpy_spd) {
              return std::unique_ptr<SpdMatrix>(
                  new SpdMatrix(Matrix(
                      numpy_spd.rows(),
                      numpy_spd.cols(),
                      numpy_spd.data(),
                      false)));
            }),
            "Create a symmetric positive definite matrix by copying data "
          "from a 2-D numpy array."
            )
        ;

    py::class_<Cholesky>(boom, "Cholesky")
        .def(py::init<const Matrix &>(),
             py::arg("A"),
             "The (lower) cholesky decomposition of the matrix A.")
        .def_property_readonly("nrow", &Cholesky::nrow)
        .def_property_readonly("ncol", &Cholesky::ncol)
        .def_property_readonly("dim", &Cholesky::dim)
        .def("getL", &Cholesky::getL, py::arg("check"),
             "Extract the lower triangular matrix.\n"
             "Args:\n"
             "  check:  If True, throw an exception if the original matrix "
             "was not positive definite.\n"
             )
        .def("getLT", &Cholesky::getLT, "The transpose of L.")
        .def("solve", [](const Cholesky &chol,
                         const Matrix &mat) {
               return chol.solve(mat);
             },
             "Return Ainv * mat, where Ainv is the inverse of the "
             "decomposed matrix.")
        .def("solve", [](const Cholesky &chol,
                         const Vector &vec) {
               return chol.solve(vec);
             },
             "Return Ainv * vec, where Ainv is the inverse of the "
             "decomposed matrix.")
        .def("inv", &Cholesky::inv, "Inverse of the decomposed matrix.")
        .def_property_readonly("original_matrix",
                               &Cholesky::original_matrix,
                               "The matrix that was decomposed.")
        .def("det", &Cholesky::det, "Determinant of the original matrix.")
        .def("logdet",  &Cholesky::logdet,
             "Log of the determinant of the original matrix.\n"
             "It is computationally more stable to call chol.logdet() "
             "than log(chol.det()).")
        .def_property_readonly("pos_def", &Cholesky::is_pos_def,
                               "Is the original matrix positive definite.")
        ;


  }  // ends the LinAlg_def function.

}  // namespace BayesBoom
