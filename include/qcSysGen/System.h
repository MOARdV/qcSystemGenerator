/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2021-2024 Questionable Coding
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
****************************************************************************/
#pragma once

#include "Config.h"
#include "Consts.h"
#include "Planet.h"
#include "Star.h"

#include <random>

namespace qc
{

namespace SystemGenerator
{

class Generator;

/// @brief Represents a single-star solar system (star + planets + moons)
///
/// Intended usage:
/// * Instantiate a Generator.  Optionally (strongly recommended) seed() the Generator's random number generator.
/// * Instantiate a SolarSystem.
/// * Instantiate a Star.  Optionally set the parameters for the star.
/// * SolarSystem::add() the Star.
///
/// For manually-created solar systems:
/// * SolarSystem::add() one or more Planet objects.
/// * Call SolarSystem::evaluate() to finalize planetary stats.
/// * todo: Implement support for manual planet adding
/// 
/// For automatically-generated solar systems:
/// * Instantiate and populate a Config structure.
/// * Optionally set Config::generateBodeSeeds to true to generate a more structured solar system.
/// * Call Generator::create() with the SolarSystem and the Config.
/// 
/// For manually-seeded solar systems:
/// * Instantiate and populate a Config structure.  Add one or more ProtoplanetSeed structures to Config::protoplanetSeeds
/// * Call Generator::create() with the SolarSystem and the Config.
class SolarSystem
{
    public:

    SolarSystem() { }
    ~SolarSystem() { }

    /// @brief Add/replace the central star in this solar system.
    /// 
    /// reset() is called implicitly by this method before the new star is added, so do
    /// not add a star after adding planets.  They will be deleted.
    /// @param star_ The star to use as the central star.  It will be evaluated after adding.
    void add(const Star& star_);

    /// @brief Evaluate the contents of the solar system.
    ///
    /// This method will evaluate all planets (and their moons) in the planet list that have not
    /// already been evaluated.  It is recommended to call this only once after adding
    /// any manual planets to the list (wait until there are no planets left to add).
    /// @param generator The Generator (used for random numbers).
    void evaluate(Generator& generator);

    /// @brief Get the name of the solar system.
    /// @return The name.  Will be empty if a name was not specified.
    const std::string& getName() const { return name; }

    /// @brief Access the planets of this solar system.
    /// @return The planets.
    PlanetVector& getPlanets() { return planet; }
    const PlanetVector& getPlanets() const { return planet; }

    /// @brief Access the central star.
    /// 
    /// Note that because changes to the star have significant effects on all of the
    /// planets, only the star's name may be changed once it's been attached to a
    /// solar system.
    /// @return A read-only instance of the central star.
    const Star& getStar() const { return star; }

    /// @brief Reset the SolarSystem.
    ///
    /// Any generated planets will be removed and the star will be reset
    /// to its generic default.  The solar system's name is /not/ changed.
    void reset();

    /// @brief Set the name of the solar system.
    /// 
    /// If the solar system has already been evaluated, changing the system name does not
    /// affect the Star or any Planet that has been created.
    /// @param name_ The new name of the solar system.
    void setName(const std::string& name_) { name = name_; }

    /// @brief Set/change the name of the central star.
    /// 
    /// Note that changing the star's name will not update the name of any planets in the solar
    /// system.
    /// @param name_ The new name of the star.
    void setStarName(const std::string& name_) { star.setName(name_); }

    private:
    friend class Generator;

    std::string name; //!< The name of the solar system.

    Star star; //!< The central (only) star of the solar system.

    PlanetVector planet;
};

}
}
