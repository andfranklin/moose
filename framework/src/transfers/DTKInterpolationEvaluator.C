//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "libmesh/libmesh_config.h"

#ifdef LIBMESH_TRILINOS_HAVE_DTK

// DTK includes
#include "DTKInterpolationEvaluator.h"
#include "DTKInterpolationHelper.h"

#include "libmesh/dof_map.h"
#include "libmesh/fe_interface.h"
#include "libmesh/fe_compute_data.h"
#include "libmesh/numeric_vector.h"
#include "libmesh/equation_systems.h"
#include "libmesh/mesh.h"
#include "libmesh/system.h"

namespace libMesh
{

DTKInterpolationEvaluator::DTKInterpolationEvaluator(System & in_sys,
                                                     std::string var_name,
                                                     const Point & offset)
  : sys(in_sys),
    _offset(offset),
    current_local_solution(*sys.current_local_solution),
    es(in_sys.get_equation_systems()),
    mesh(sys.get_mesh()),
    dim(mesh.mesh_dimension()),
    dof_map(sys.get_dof_map()),
    var_num(sys.variable_number(var_name)),
    fe_type(dof_map.variable_type(var_num))
{
}

DTKInterpolationEvaluator::FieldContainerType
DTKInterpolationEvaluator::evaluate(const Teuchos::ArrayRCP<GlobalOrdinal> & elements,
                                    const Teuchos::ArrayRCP<double> & coords)
{
  GlobalOrdinal num_values = elements.size();

  Teuchos::ArrayRCP<Number> values(num_values);
  DataTransferKit::FieldContainer<Number> evaluations(values, 1);

  for (GlobalOrdinal i = 0; i < num_values; i++)
  {
    Elem * elem = mesh.elem_ptr(elements[i]);

    Point p;

    for (unsigned int j = 0; j < dim; j++)
      p(j) = coords[(j * num_values) + i] - _offset(j);

    const Point mapped_point(FEInterface::inverse_map(dim, dof_map.variable_type(0), elem, p));

    FEComputeData data(es, mapped_point);
    FEInterface::compute_data(dim, fe_type, elem, data);

    std::vector<dof_id_type> dof_indices;
    dof_map.dof_indices(elem, dof_indices, var_num);

    Number value = 0;

    for (unsigned int j = 0; j < dof_indices.size(); j++)
      value += current_local_solution(dof_indices[j]) * data.shape[j];

    values[i] = value;
  }

  return evaluations;
}

} // namespace libMesh

#endif // LIBMESH_TRILINOS_HAVE_DTK
