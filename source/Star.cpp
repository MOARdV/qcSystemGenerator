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
#include <qcSysGen/Star.h>

#include <qcSysGen/Config.h>
#include <qcSysGen/Consts.h>
#include <qcSysGen/Equations.h>
#include <qcSysGen/Generator.h>

#include <algorithm>
#include <math.h>

namespace
{

//----------------------------------------------------------------------------
__inline int32_t GetBaseStellarInfoIndex(qc::SystemGenerator::StarClassification s)
{
    return static_cast<int32_t>(s) * 10;
}

// All values below assume Sol.

constexpr double EcosphereScalar = 1.0; //!< Radius of the ecosphere, in AU.

constexpr double SnowLineScalar = 5.0; //!< The snow line scalar, where water ice can exist in a vacuum, in AU.

static constexpr double InnerHabitableZoneScalar = 0.95; //!< Inner limit of the habitable zone, in AU.  Kasting 1993
static constexpr double OuterHabitableZoneScalar = 1.37; //!< Outer limit of the habitable zone, in AU.  Kasting 1993

// vvv These may belong in the Generator, instead, since that's the only place where we use them ... ?

/// @brief Inner limit of planetisimal formation, in AU.
static constexpr double InnerPlanetScalar = 0.3;
/// @brief Outer limit of planetisimal formation, in AU.
static constexpr double OuterPlanetScalar = 50.0;

/// @brief Inner limit of dust formation, in AU.
static constexpr double InnerDustScalar = 0.0;
/// @brief Outer limit of dust formation, in AU.
static constexpr double OuterDustScalar = 200.0;

static constexpr double InnerZone1Scalar = 0.0; //!< Inner limit of dust Zone 1, in AU
/// @brief Outer limit of dust Zone 1, in AU.
static constexpr double OuterZone1Scalar = 5.0;

/// @brief Inner limit of dust Zone 2, in AU.
static constexpr double InnerZone2Scalar = 4.0;
/// @brief Outer limit of dust Zone 2, in AU.
static constexpr double OuterZone2Scalar = 16.0;

/// @brief Inner limit of dust Zone 3, in AU.
static constexpr double InnerZone3Scalar = 14.0;
/// @brief Outer limit of dust Zone 3, in AU.
static constexpr double OuterZone3Scalar = 200.0;

// Stats related to the star type.
//
// Mass and radius are normalized so Sol = 1.0.
//
// Luminance is normalized so Sol = 1.0; since this table
// contains logL, logL Sol = 0.0.

#include "StellarInfo.h"

}


namespace qc
{

namespace SystemGenerator
{

//----------------------------------------------------------------------------
StarType_t GetStarType(double mass)
{
    // Start at the heaviest, move to the lightest.

    const float m = float(mass);
    // Start on index 3 (O3V), not 0, since the first 3 entries are bogus
    for (uint32_t i = 3; i < stellarInfoCount; ++i)
    {
        if (stellarInfo[i].mass <= m)
        {
            const uint32_t c = i / 10u;
            const uint32_t idx = i % 10u;

            return std::make_pair(StarClassification(c), int32_t(idx));
        }
    }

    // Hard-coded fallback: star is too small.
    return std::make_pair(StarClassification::M_V, 9);
}

//----------------------------------------------------------------------------
void Star::evaluate(Generator* generator)
{
    if (evaluated)
    {
        return; // Early.  We already evaluated this star.
    }

    //--- Set the primary derived values:

    const StellarInfo_t& si = stellarInfo[GetBaseStellarInfoIndex(type) + subtype];

    temperatureKelvin = powf(10.0f, si.logT);
    luminositySolar = pow(10.0, si.logL);
    radiusSolar = si.radius;
    massSolar = si.mass;

    const double maximumStellarAge = std::min(MaximumStellarAge, 1.0e10 * (massSolar / luminositySolar));

    if (ageYears == 0.0)
    {
        if (!generator)
        {
            ageYears = 0.5 * (maximumStellarAge + MinimumStellarAge);
        }
        else
        {
            const double ageRange = maximumStellarAge - MinimumStellarAge;
            ageYears = generator->randomUniform(MinimumStellarAge + 0.25 * ageRange, MinimumStellarAge + 0.75 * ageRange);
        }
    }

    ageYears = std::max(MinimumStellarAge, ageYears);
    ageYears = std::min(maximumStellarAge, ageYears);

    //--- Set the secondary derived values:
    const double sqrtLum = sqrt(luminositySolar);
    const double curtMass = pow(massSolar, (1.0 / 3.0));

    ecosphere = sqrtLum * EcosphereScalar;
    snowLine = sqrtLum * SnowLineScalar;

    habitableZone = std::make_pair(sqrtLum * InnerHabitableZoneScalar, sqrtLum * OuterHabitableZoneScalar);
    dustZone = std::make_pair(curtMass * InnerDustScalar, curtMass * OuterDustScalar);
    protoplanetZone = std::make_pair(curtMass * InnerPlanetScalar, curtMass * OuterPlanetScalar);

    zone1 = std::make_pair(sqrtLum * InnerZone1Scalar, sqrtLum * OuterZone1Scalar);
    zone2 = std::make_pair(sqrtLum * InnerZone2Scalar, sqrtLum * OuterZone2Scalar);
    zone3 = std::make_pair(sqrtLum * InnerZone3Scalar, sqrtLum * OuterZone3Scalar);

    evaluated = true;
}

//----------------------------------------------------------------------------
void Star::getStellarClass(char* output, size_t sizeofOutput) const
{
    if (sizeofOutput < 4)
    {
        return;
    }

    const char* fmt = nullptr;
    switch (type)
    {
        case StarClassification::O_V:
            fmt = "O%dV";
            break;
        case StarClassification::B_V:
            fmt = "B%dV";
            break;
        case StarClassification::A_V:
            fmt = "A%dV";
            break;
        case StarClassification::F_V:
            fmt = "F%dV";
            break;
        case StarClassification::K_V:
            fmt = "K%dV";
            break;
        case StarClassification::M_V:
            fmt = "M%dV";
            break;
        case StarClassification::G_V:
        default:
            fmt = "G%dV";
    }

    sprintf_s(output, sizeofOutput, fmt, subtype);
}

//----------------------------------------------------------------------------
void Star::getStellarClass(std::string& out) const
{
    // Yes, I'm taking the lazy way
    char outchar[4];
    getStellarClass(outchar, sizeof(outchar));

    out = std::string(outchar);
}

}
}
