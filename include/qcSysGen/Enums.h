/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2024 Questionable Coding
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

namespace qc
{

namespace SystemGenerator
{

/// @brief Enumeration of gases of interest in planetary atmospheres
enum class Gas
{
    Hydrogen,
    Helium,
    Nitrogen,
    Oxygen,
    Neon,
    Argon,
    Krypton,
    Xenon,
    Ammonia,
    Water,
    CarbonDioxide,
    Ozone,
    Methane
};

/// @brief Return an English string corresponding to the name of the Gas
/// @note Returns "Unknown" for invalid values.
/// @param gas The Gas to stringify.
/// @return The name.
const char* GasName(Gas gas);

//----------------------------------------------------------------------------

/// @brief A broad classification of the zones of a planetary system, based on position relative to the
/// habitable zone and the snow line.
enum class OrbitalZone
{
    Inner, //!< Between the star and the inner edge of the habitable zone.
    Habitable, //!< Between the bounds of the habitable zone.
    Middle, //!< Outside the habitable zone, inside the snow line.
    Outer, //!< Outside the snow line.
};

/// @brief Return an English string corresponding to the orbital zone.
/// @note Returns "Unknown" for invalid values.
/// @param orbitalZone The OrbitalZone to stringify.
/// @return The name.
const char* OrbitalZoneName(OrbitalZone orbitalZone);

//----------------------------------------------------------------------------

/// @brief Enumeration of planet types
///
/// @note If there are changes here, don't forget to update Planet::PlanetTypeString().
enum class PlanetType
{
    Unknown, //!< Undetermined type

    //--- Rocky planet types
    Rocky, //!< Catch-all for non-gaseous planets.  Terrestrial planets.

    AsteroidBelt, //!< A collection of boulder-to-mountain sized rocky and metallic objects.
    DwarfPlanet, //!< A smaller body that is too small to establish orbital dominance.
    IcePlanet, //!< A colder terrestrial planet dominated by surface ices.
    Terrestrial, //!< A rocky planet with atmosphere and liquid hydrosphere.
    Ocean, //!< A terrestrial world characterized by near-total coverage in water.

    //--- Gaseous planet types
    Gaseous,    //!< Catch-all for a planet with a substantial volatile gas composition.  Used as an interim classification

    IceGiant,   //!< The smaller sized gaseous planets, also called Neptunian.  Uranus and Neptune fall into this category.  Roughly 2.0 (+/-0.7)x M(Earth) to 0.41 (+/-0.07)x M(Jupiter).
    GasGiant,   //!< Large planets primarily composed of hydrogen and helium.  Jupiter falls into this category; Saturn barely makes the cutoff, as well.  0.41x M(Jupiter) on up.
    BrownDwarf, //!< Large gas giants, but not large enough to sustain nuclear fusion of hydrogen.  13x to 80x M(Jupiter).
};

/// @brief Returns an English string corresponding to the name of the PlanetType.
/// @note Returns "Unknown" for invalid values.
/// @param type The PlanetType to stringify.
/// @return The name.
const char* PlanetTypeName(PlanetType type);


}
}
