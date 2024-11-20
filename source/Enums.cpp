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
#include <qcSysGen/Enums.h>

#include <algorithm>
#include <cstdint>
#include <stdlib.h>

namespace qc
{

namespace SystemGenerator
{

//----------------------------------------------------------------------------
const char* GasName(Gas gas)
{
    static const char* gases[] =
    {
        "Hydrogen",
        "Helium",
        "Nitrogen",
        "Oxygen",
        "Neon",
        "Argon",
        "Krypton",
        "Xenon",
        "Ammonia",
        "Water Vapor",
        "Carbon Dioxide",
        "Ozone",
        "Methane",
        "Unknown" // <--- This is one past the end of the Gas enum size
    };

    const uint32_t idx = std::min(uint32_t(gas), static_cast<uint32_t>(_countof(gases) - 1u));

    return gases[idx];
}

//----------------------------------------------------------------------------
const char* OrbitalZoneName(OrbitalZone orbitalZone)
{
    static const char* orbitalZones[] =
    {
        "Inner",
        "Habitable",
        "Middle",
        "Outer",
        "Unknown" // <--- This is one past the end of the Gas enum size
    };

    const uint32_t idx = std::min(uint32_t(orbitalZone), static_cast<uint32_t>(_countof(orbitalZones) - 1u));

    return orbitalZones[idx];
}

//----------------------------------------------------------------------------
const char* PlanetTypeName(PlanetType type)
{
    static const char* planetType[] =
    {
        "Unknown",
        "Rocky Planet",
        "Asteroid Belt",
        "Dwarf Planet",
        "Ice Planet",
        "Terrestrial Planet",
        "Ocean Planet",
        "Gaseous",
        "Ice Giant",
        "Gas Giant",
        "Brown Dwarf",
        "Unknown" // <--- This is one past the end of the PlanetType enum size
    };

    const uint32_t idx = std::min(uint32_t(type), static_cast<uint32_t>(_countof(planetType) - 1u));

    return planetType[idx];
}

}
}