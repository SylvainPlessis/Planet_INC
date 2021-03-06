//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// Planet - An atmospheric code for planetary bodies, adapted to Titan
//
// Copyright (C) 2013 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-

#ifndef PLANET_PHYSICS_FACTORY_H
#define PLANET_PHYSICS_FACTORY_H

#include "grins/physics_factory.h"

namespace Planet
{
  //! Build PlanetPhysics
  /*!
   * Extends the underlying GRINS::PhysicsFactory to build
   * PlanetPhysics, but still play nice with other GRINS::Physics.
   */
  class PhysicsFactory : public GRINS::PhysicsFactory
  {
  public:

    PhysicsFactory();

    virtual ~PhysicsFactory();

  protected:

    virtual void add_physics( const GetPot& input,
			      const std::string& physics_to_add,
			      GRINS::PhysicsList& physics_list );
  };

} // end namespace Planet

#endif // PLANET_PHYSICS_FACTORY_H
