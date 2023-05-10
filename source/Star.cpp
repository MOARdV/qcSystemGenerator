/*****************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2021-2023 Questionable Coding
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
#include "Star.h"

#include "Config.h"
#include "Consts.h"
#include "Equations.h"
#include "GenerationState.h"

#include <algorithm>
#include <math.h>

/*
* Development notes
* 
* http://www.pas.rochester.edu/~emamajek/EEM_dwarf_UBVIJHK_colors_Teff.txt
*/

namespace
{

struct StellarClass
{
    double logLum; //!< The logarithm of stellar luminosity.
    char subclass; //!< The subclass, integer [0, 9].
    char stellarClass; //!< The stellar class.
};

static constexpr StellarClass stellarClass[] =
{
    { +1.58, '0', 'A' }, // M(solar) = 2.18
    { +1.49, '1', 'A' },
    { +1.38, '2', 'A' },
    { +1.23, '3', 'A' },
    { +1.13, '4', 'A' },
    { +1.09, '5', 'A' },
    { +1.05, '6', 'A' },
    { +1.00, '7', 'A' },
    { +0.96, '8', 'A' },
    { +0.92, '9', 'A' },
    { +0.86, '0', 'F' },
    { +0.79, '1', 'F' },
    { +0.71, '2', 'F' },
    { +0.67, '3', 'F' },
    { +0.62, '4', 'F' },
    { +0.56, '5', 'F' },
    { +0.43, '6', 'F' },
    { +0.39, '7', 'F' },
    { +0.29, '8', 'F' },
    { +0.22, '9', 'F' },
    { +0.13, '0', 'G' },
    { +0.08, '1', 'G' },
    { +0.01, '2', 'G' },
    { -0.01, '3', 'G' },
    { -0.04, '4', 'G' },
    { -0.05, '5', 'G' },
    { -0.10, '6', 'G' },
    { -0.13, '7', 'G' },
    { -0.17, '8', 'G' },
    { -0.26, '9', 'G' },
    { -0.34, '0', 'K' },
    { -0.39, '1', 'K' },
    { -0.43, '2', 'K' },
    { -0.55, '3', 'K' },
    { -0.69, '4', 'K' },
    { -0.76, '5', 'K' },
    { -0.86, '6', 'K' },
    { -1.00, '7', 'K' },
    { -1.06, '8', 'K' },
    { -1.10, '9', 'K' },
    { -1.16, '0', 'M' }, // M(solar) = 0.57
};

static constexpr int stellarClassCount = _countof(stellarClass);

}

//----------------------------------------------------------------------------
Star::Star(float mass, double random01) :
    stellarMass(mass),
    stellarLuminosity(Luminosity(mass)),
    lifespan(0.0),
    curtMass(0.0),
    sqrtLum(0.0),
    outerPlanetaryR(0.0)
{
    lifespan = 1.0e10 * (stellarMass / stellarLuminosity);
    stellarAge = Lerp(random01, MinimumStellarAge, std::min(MaximumStellarAge, lifespan));

    sqrtLum = sqrt(stellarLuminosity);
    curtMass = pow(stellarMass, (1.0 / 3.0));

    outerPlanetaryR = curtMass * OuterPlanetScalar;
}

//----------------------------------------------------------------------------
void Star::init(const Config* config, GenerationState* state)
{
    stellarMass = config->stellarMass;
    stellarLuminosity = Luminosity(stellarMass);

    lifespan = 1.0E10 * (stellarMass / stellarLuminosity);
    stellarAge = state->randomUniform(MinimumStellarAge, std::min(MaximumStellarAge, lifespan));

    sqrtLum = sqrt(stellarLuminosity);
    curtMass = pow(stellarMass, (1.0 / 3.0));

    outerPlanetaryR = curtMass * OuterPlanetScalar;
    if (config->overrideOuterPlanetLimit > 0.0)
    {
        outerPlanetaryR = std::min(outerPlanetaryR, config->overrideOuterPlanetLimit);
    }
}

//----------------------------------------------------------------------------
void Star::stellarClass(char* output, size_t sizeofOutput) const
{
    if (sizeofOutput < 4)
    {
        return;
    }

    const double logLum = log(stellarLuminosity);

    output[2] = 'V';
    output[3] = 0;

    if (logLum > ::stellarClass[0].logLum)
    {
        output[0] = ::stellarClass[0].stellarClass;
        output[1] = ::stellarClass[0].subclass;
        
        return;
    }

    auto result = std::adjacent_find(std::begin(::stellarClass), std::end(::stellarClass), [=](const StellarClass& left, const StellarClass& right)
        {
            return (left.logLum >= logLum && right.logLum < logLum);
        });

    if (result != std::end(::stellarClass))
    {
        output[0] = result->stellarClass;
        output[1] = result->subclass;
    }
    else
    {
        output[0] = ::stellarClass[stellarClassCount - 1].stellarClass;
        output[1] = ::stellarClass[stellarClassCount - 1].subclass;
    }
}

//----------------------------------------------------------------------------
std::string Star::stellarClass() const
{
    // Yes, I'm taking the lazy way
    char out[4];
    stellarClass(out, sizeof(out));

    return std::string(out);
}